#pragma once

#include "bridge/BridgeConfig.hpp"
#include "bridge/MessageFrame.hpp"
#include "bridge/Metrics.hpp"
#include "bridge/ThreadSafeQueue.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace bridge {

struct RoutedMessage {
    EndpointKind source;
    MessageFrame frame;
};

class IPCListener {
public:
    IPCListener(const BridgeConfig& config, ThreadSafeQueue<RoutedMessage>& inbound, Metrics& metrics);
    ~IPCListener();

    IPCListener(const IPCListener&) = delete;
    IPCListener& operator=(const IPCListener&) = delete;

    void start();
    void stop();
    bool running() const;
    bool send_to_clients(const MessageFrame& frame);

private:
    void run();
    void setup_unix_socket();
    void setup_fifo();
    void close_client(int fd);

    BridgeConfig config_;
    ThreadSafeQueue<RoutedMessage>& inbound_;
    Metrics& metrics_;
    std::atomic_bool running_{false};
    std::thread worker_;
    int epoll_fd_{-1};
    int unix_fd_{-1};
    int fifo_fd_{-1};
    std::mutex clients_mutex_;
    std::vector<int> clients_;
};

} // namespace bridge
