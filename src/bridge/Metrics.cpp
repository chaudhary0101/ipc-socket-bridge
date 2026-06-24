#include "bridge/Metrics.hpp"

#include <sstream>

namespace bridge {

Metrics::Metrics() : started_(std::chrono::steady_clock::now()) {}

void Metrics::record_routed(std::size_t bytes) {
    messages_routed_.fetch_add(1);
    bytes_transferred_.fetch_add(bytes);
}

void Metrics::record_ipc_connection() {
    ipc_connections_.fetch_add(1);
}

void Metrics::record_tcp_connection() {
    tcp_connections_.fetch_add(1);
}

void Metrics::record_error() {
    errors_.fetch_add(1);
}

MetricsSnapshot Metrics::snapshot() const {
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started_);
    return {messages_routed_.load(), bytes_transferred_.load(), ipc_connections_.load(),
            tcp_connections_.load(), errors_.load(), static_cast<uint64_t>(uptime.count())};
}

std::string Metrics::to_string() const {
    const auto s = snapshot();
    std::ostringstream out;
    out << "messages=" << s.messages_routed << " bytes=" << s.bytes_transferred
        << " ipc_connections=" << s.ipc_connections << " tcp_connections=" << s.tcp_connections
        << " errors=" << s.errors << " uptime_seconds=" << s.uptime_seconds;
    return out.str();
}

} // namespace bridge
