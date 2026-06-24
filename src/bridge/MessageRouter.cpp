#include "bridge/MessageRouter.hpp"

#include "bridge/Logger.hpp"

namespace bridge {

MessageRouter::MessageRouter(ThreadSafeQueue<RoutedMessage>& inbound, IPCListener& ipc, TCPServer& tcp, Metrics& metrics)
    : inbound_(inbound), ipc_(ipc), tcp_(tcp), metrics_(metrics) {}

MessageRouter::~MessageRouter() {
    stop();
}

void MessageRouter::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread(&MessageRouter::run, this);
}

void MessageRouter::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    inbound_.close();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool MessageRouter::running() const {
    return running_.load();
}

void MessageRouter::run() {
    while (running_) {
        auto item = inbound_.wait_and_pop();
        if (!item) {
            break;
        }
        bool delivered = false;
        if (item->source == EndpointKind::Ipc) {
            item->frame.destination = EndpointKind::Tcp;
            delivered = tcp_.send_to_clients(item->frame);
        } else {
            item->frame.destination = EndpointKind::Ipc;
            delivered = ipc_.send_to_clients(item->frame);
        }
        if (delivered) {
            metrics_.record_routed(item->frame.payload.size());
        } else {
            metrics_.record_error();
            Logger::instance().log(LogLevel::Warn, "MessageRouter", "message dropped because no destination client was writable");
        }
    }
}

} // namespace bridge
