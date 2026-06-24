#include "bridge/Bridge.hpp"

#include "bridge/Logger.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace bridge {

Bridge::Bridge(BridgeConfig config) : config_(std::move(config)), queue_(config_.queue_capacity) {
    config_.validate();
    Logger::instance().configure(config_.log_level, config_.log_file, config_.log_console);
    ipc_ = std::make_unique<IPCListener>(config_, queue_, metrics_);
    tcp_ = std::make_unique<TCPServer>(config_, queue_, metrics_);
    router_ = std::make_unique<MessageRouter>(queue_, *ipc_, *tcp_, metrics_);
}

Bridge::~Bridge() {
    stop();
}

void Bridge::start() {
    Logger::instance().log(LogLevel::Info, "Bridge", "starting ipc-socket-bridge");
    ipc_->start();
    tcp_->start();
    router_->start();
}

void Bridge::stop() {
    if (router_) router_->stop();
    if (ipc_) ipc_->stop();
    if (tcp_) tcp_->stop();
    Logger::instance().log(LogLevel::Info, "Bridge", "stopped with metrics: " + metrics_.to_string());
}

int Bridge::run_until_shutdown() {
    start();
    while (!SignalHandler::shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    stop();
    return 0;
}

MetricsSnapshot Bridge::metrics() const {
    return metrics_.snapshot();
}

} // namespace bridge
