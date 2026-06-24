#pragma once

#include "bridge/BridgeConfig.hpp"
#include "bridge/IPCListener.hpp"
#include "bridge/MessageRouter.hpp"
#include "bridge/Metrics.hpp"
#include "bridge/SignalHandler.hpp"
#include "bridge/TCPServer.hpp"

#include <memory>

namespace bridge {

class Bridge {
public:
    explicit Bridge(BridgeConfig config);
    ~Bridge();

    void start();
    void stop();
    int run_until_shutdown();
    MetricsSnapshot metrics() const;

private:
    BridgeConfig config_;
    Metrics metrics_;
    SignalHandler signals_;
    ThreadSafeQueue<RoutedMessage> queue_;
    std::unique_ptr<IPCListener> ipc_;
    std::unique_ptr<TCPServer> tcp_;
    std::unique_ptr<MessageRouter> router_;
};

} // namespace bridge
