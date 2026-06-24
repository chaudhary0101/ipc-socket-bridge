#pragma once

#include "bridge/Logger.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace bridge {

struct BridgeConfig {
    std::string unix_socket_path{"/tmp/ipc_socket_bridge.sock"};
    std::string fifo_path{"/tmp/ipc_socket_bridge.fifo"};
    std::string tcp_bind_address{"0.0.0.0"};
    uint16_t tcp_port{9090};
    std::size_t queue_capacity{1024};
    std::size_t max_payload_bytes{65536};
    std::chrono::milliseconds epoll_timeout{500};
    std::chrono::milliseconds reconnect_initial_delay{100};
    std::chrono::milliseconds reconnect_max_delay{5000};
    std::string log_file{"bridge.log"};
    LogLevel log_level{LogLevel::Info};
    bool log_console{true};
    bool enable_fifo{true};
    bool enable_unix_socket{true};

    static BridgeConfig from_file(const std::string& path);
    void validate() const;
};

LogLevel parse_log_level(const std::string& text);

} // namespace bridge
