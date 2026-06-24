#include "bridge/IPCListener.hpp"

#include "bridge/Logger.hpp"

#include <array>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

namespace bridge {
namespace {
void add_epoll(int epoll_fd, int fd) {
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error("epoll_ctl add failed: " + std::string(std::strerror(errno)));
    }
}

void close_fd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}
} // namespace

IPCListener::IPCListener(const BridgeConfig& config, ThreadSafeQueue<RoutedMessage>& inbound, Metrics& metrics)
    : config_(config), inbound_(inbound), metrics_(metrics) {}

IPCListener::~IPCListener() {
    stop();
}

void IPCListener::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread(&IPCListener::run, this);
}

void IPCListener::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    close_fd(unix_fd_);
    close_fd(fifo_fd_);
    close_fd(epoll_fd_);
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (int& fd : clients_) {
            close_fd(fd);
        }
        clients_.clear();
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    unlink(config_.unix_socket_path.c_str());
}

bool IPCListener::running() const {
    return running_.load();
}

bool IPCListener::send_to_clients(const MessageFrame& frame) {
    auto bytes = frame.serialize();
    bool delivered = false;
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto it = clients_.begin(); it != clients_.end();) {
        const ssize_t sent = send(*it, bytes.data(), bytes.size(), MSG_NOSIGNAL);
        if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            close(*it);
            it = clients_.erase(it);
            metrics_.record_error();
        } else {
            delivered = sent == static_cast<ssize_t>(bytes.size()) || delivered;
            ++it;
        }
    }
    return delivered;
}

void IPCListener::run() {
    std::unordered_map<int, std::vector<uint8_t>> buffers;
    try {
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) {
            throw std::runtime_error("epoll_create1 failed");
        }
        if (config_.enable_unix_socket) {
            setup_unix_socket();
        }
        if (config_.enable_fifo) {
            setup_fifo();
        }
        std::vector<epoll_event> events(32);
        while (running_) {
            const int n = epoll_wait(epoll_fd_, events.data(), static_cast<int>(events.size()),
                                     static_cast<int>(config_.epoll_timeout.count()));
            if (n < 0) {
                if (errno == EINTR) continue;
                metrics_.record_error();
                break;
            }
            for (int i = 0; i < n; ++i) {
                const int fd = events[i].data.fd;
                if (fd == unix_fd_) {
                    while (true) {
                        const int client = accept4(unix_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (client < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            metrics_.record_error();
                            break;
                        }
                        add_epoll(epoll_fd_, client);
                        {
                            std::lock_guard<std::mutex> lock(clients_mutex_);
                            clients_.push_back(client);
                        }
                        metrics_.record_ipc_connection();
                    }
                } else {
                    std::array<uint8_t, 4096> chunk{};
                    while (true) {
                        const ssize_t got = read(fd, chunk.data(), chunk.size());
                        if (got > 0) {
                            auto& buffer = buffers[fd];
                            buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + got);
                            MessageFrame frame;
                            while (MessageFrame::try_extract(buffer, config_.max_payload_bytes, frame)) {
                                frame.source = EndpointKind::Ipc;
                                frame.destination = EndpointKind::Tcp;
                                inbound_.push({EndpointKind::Ipc, frame});
                            }
                        } else if (got == 0) {
                            close_client(fd);
                            buffers.erase(fd);
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            metrics_.record_error();
                            close_client(fd);
                            buffers.erase(fd);
                            break;
                        }
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        metrics_.record_error();
        Logger::instance().log(LogLevel::Error, "IPCListener", ex.what());
    }
}

void IPCListener::setup_unix_socket() {
    unlink(config_.unix_socket_path.c_str());
    unix_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (unix_fd_ == -1) {
        throw std::runtime_error("socket(AF_UNIX) failed");
    }
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", config_.unix_socket_path.c_str());
    if (bind(unix_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw std::runtime_error("bind unix socket failed: " + std::string(std::strerror(errno)));
    }
    if (listen(unix_fd_, SOMAXCONN) == -1) {
        throw std::runtime_error("listen unix socket failed");
    }
    add_epoll(epoll_fd_, unix_fd_);
}

void IPCListener::setup_fifo() {
    if (mkfifo(config_.fifo_path.c_str(), 0660) == -1 && errno != EEXIST) {
        throw std::runtime_error("mkfifo failed: " + std::string(std::strerror(errno)));
    }
    fifo_fd_ = open(config_.fifo_path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fifo_fd_ == -1) {
        throw std::runtime_error("open fifo failed: " + std::string(std::strerror(errno)));
    }
    add_epoll(epoll_fd_, fifo_fd_);
}

void IPCListener::close_client(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), fd), clients_.end());
}

} // namespace bridge
