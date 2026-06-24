#include "bridge/BridgeConfig.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace bridge {

namespace {
std::string trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool parse_bool(const std::string& value) {
    return value == "true" || value == "1" || value == "yes" || value == "on";
}
} // namespace

LogLevel parse_log_level(const std::string& text) {
    if (text == "debug") {
        return LogLevel::Debug;
    }
    if (text == "info") {
        return LogLevel::Info;
    }
    if (text == "warn") {
        return LogLevel::Warn;
    }
    if (text == "error") {
        return LogLevel::Error;
    }
    throw std::invalid_argument("invalid log_level: " + text);
}

BridgeConfig BridgeConfig::from_file(const std::string& path) {
    BridgeConfig cfg;
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open config file: " + path);
    }
    std::string line;
    while (std::getline(in, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error("invalid config line: " + line);
        }
        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));
        if (key == "unix_socket_path") cfg.unix_socket_path = value;
        else if (key == "fifo_path") cfg.fifo_path = value;
        else if (key == "tcp_bind_address") cfg.tcp_bind_address = value;
        else if (key == "tcp_port") cfg.tcp_port = static_cast<uint16_t>(std::stoul(value));
        else if (key == "queue_capacity") cfg.queue_capacity = std::stoul(value);
        else if (key == "max_payload_bytes") cfg.max_payload_bytes = std::stoul(value);
        else if (key == "epoll_timeout_ms") cfg.epoll_timeout = std::chrono::milliseconds(std::stoul(value));
        else if (key == "reconnect_initial_delay_ms") cfg.reconnect_initial_delay = std::chrono::milliseconds(std::stoul(value));
        else if (key == "reconnect_max_delay_ms") cfg.reconnect_max_delay = std::chrono::milliseconds(std::stoul(value));
        else if (key == "log_file") cfg.log_file = value;
        else if (key == "log_level") cfg.log_level = parse_log_level(value);
        else if (key == "log_console") cfg.log_console = parse_bool(value);
        else if (key == "enable_fifo") cfg.enable_fifo = parse_bool(value);
        else if (key == "enable_unix_socket") cfg.enable_unix_socket = parse_bool(value);
        else throw std::runtime_error("unknown config key: " + key);
    }
    cfg.validate();
    return cfg;
}

void BridgeConfig::validate() const {
    if (!enable_fifo && !enable_unix_socket) {
        throw std::invalid_argument("at least one IPC transport must be enabled");
    }
    if (tcp_port == 0) {
        throw std::invalid_argument("tcp_port must be non-zero");
    }
    if (queue_capacity == 0 || max_payload_bytes == 0) {
        throw std::invalid_argument("queue_capacity and max_payload_bytes must be non-zero");
    }
    if (reconnect_initial_delay > reconnect_max_delay) {
        throw std::invalid_argument("initial reconnect delay must not exceed max delay");
    }
}

} // namespace bridge
