#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace bridge {

struct MetricsSnapshot {
    uint64_t messages_routed;
    uint64_t bytes_transferred;
    uint64_t ipc_connections;
    uint64_t tcp_connections;
    uint64_t errors;
    uint64_t uptime_seconds;
};

class Metrics {
public:
    Metrics();

    void record_routed(std::size_t bytes);
    void record_ipc_connection();
    void record_tcp_connection();
    void record_error();
    MetricsSnapshot snapshot() const;
    std::string to_string() const;

private:
    std::chrono::steady_clock::time_point started_;
    std::atomic_uint64_t messages_routed_{0};
    std::atomic_uint64_t bytes_transferred_{0};
    std::atomic_uint64_t ipc_connections_{0};
    std::atomic_uint64_t tcp_connections_{0};
    std::atomic_uint64_t errors_{0};
};

} // namespace bridge
