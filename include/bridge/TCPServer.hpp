#pragma once

#include "bridge/BridgeConfig.hpp"
#include "bridge/IPCListener.hpp"
#include "bridge/MessageFrame.hpp"
#include "bridge/Metrics.hpp"
#include "bridge/ThreadSafeQueue.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace bridge {

class TCPServer {
public:
    TCPServer(const BridgeConfig& config, ThreadSafeQueue<RoutedMessage>& inbound, Metrics& metrics);
    ~TCPServer();

    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    void start();
    void stop();
    bool running() const;
    bool send_to_clients(const MessageFrame& frame);

private:
    void run();
    void setup_listener();
    void close_client(int fd);

    BridgeConfig config_;
    ThreadSafeQueue<RoutedMessage>& inbound_;
    Metrics& metrics_;
    std::atomic_bool running_{false};
    std::thread worker_;
    int epoll_fd_{-1};
    int listen_fd_{-1};
    std::mutex clients_mutex_;
    std::vector<int> clients_;
};

} // namespace bridge
