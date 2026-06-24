#include "bridge/TCPServer.hpp"

#include "bridge/Logger.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

namespace bridge {
namespace {
void add_epoll(int epoll_fd, int fd) {
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error("epoll_ctl add failed");
    }
}

void close_fd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}
} // namespace

TCPServer::TCPServer(const BridgeConfig& config, ThreadSafeQueue<RoutedMessage>& inbound, Metrics& metrics)
    : config_(config), inbound_(inbound), metrics_(metrics) {}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread(&TCPServer::run, this);
}

void TCPServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    close_fd(listen_fd_);
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
}

bool TCPServer::running() const {
    return running_.load();
}

bool TCPServer::send_to_clients(const MessageFrame& frame) {
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
            delivered = delivered || sent == static_cast<ssize_t>(bytes.size());
            ++it;
        }
    }
    return delivered;
}

void TCPServer::run() {
    std::unordered_map<int, std::vector<uint8_t>> buffers;
    try {
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) {
            throw std::runtime_error("epoll_create1 failed");
        }
        setup_listener();
        std::vector<epoll_event> events(64);
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
                if (fd == listen_fd_) {
                    while (true) {
                        const int client = accept4(listen_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
                        metrics_.record_tcp_connection();
                    }
                } else {
                    std::array<uint8_t, 4096> chunk{};
                    while (true) {
                        const ssize_t got = recv(fd, chunk.data(), chunk.size(), 0);
                        if (got > 0) {
                            auto& buffer = buffers[fd];
                            buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + got);
                            MessageFrame frame;
                            while (MessageFrame::try_extract(buffer, config_.max_payload_bytes, frame)) {
                                frame.source = EndpointKind::Tcp;
                                frame.destination = EndpointKind::Ipc;
                                inbound_.push({EndpointKind::Tcp, frame});
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
        Logger::instance().log(LogLevel::Error, "TCPServer", ex.what());
    }
}

void TCPServer::setup_listener() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listen_fd_ == -1) {
        throw std::runtime_error("socket(AF_INET) failed");
    }
    int yes = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.tcp_port);
    if (inet_pton(AF_INET, config_.tcp_bind_address.c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("invalid tcp_bind_address");
    }
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw std::runtime_error("bind tcp failed: " + std::string(std::strerror(errno)));
    }
    if (listen(listen_fd_, SOMAXCONN) == -1) {
        throw std::runtime_error("listen tcp failed");
    }
    add_epoll(epoll_fd_, listen_fd_);
}

void TCPServer::close_client(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), fd), clients_.end());
}

} // namespace bridge
