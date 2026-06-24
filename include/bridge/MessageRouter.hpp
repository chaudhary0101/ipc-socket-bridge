#pragma once

#include "bridge/IPCListener.hpp"
#include "bridge/Metrics.hpp"
#include "bridge/TCPServer.hpp"
#include "bridge/ThreadSafeQueue.hpp"

#include <atomic>
#include <thread>

namespace bridge {

class MessageRouter {
public:
    MessageRouter(ThreadSafeQueue<RoutedMessage>& inbound, IPCListener& ipc, TCPServer& tcp, Metrics& metrics);
    ~MessageRouter();

    void start();
    void stop();
    bool running() const;

private:
    void run();

    ThreadSafeQueue<RoutedMessage>& inbound_;
    IPCListener& ipc_;
    TCPServer& tcp_;
    Metrics& metrics_;
    std::atomic_bool running_{false};
    std::thread worker_;
};

} // namespace bridge
