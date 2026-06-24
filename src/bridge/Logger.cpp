#include "bridge/Logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace bridge {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::configure(LogLevel min_level, const std::string& file_path, bool console_enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min_level;
    console_enabled_ = console_enabled;
    if (!file_path.empty()) {
        file_.open(file_path, std::ios::app);
    }
}

void Logger::log(LogLevel level, const std::string& component, const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(min_level_)) {
        return;
    }
    std::ostringstream line;
    line << timestamp() << " [" << to_string(level) << "] [" << component << "] " << message << '\n';
    std::lock_guard<std::mutex> lock(mutex_);
    if (console_enabled_) {
        std::clog << line.str();
    }
    if (file_.is_open()) {
        file_ << line.str();
        file_.flush();
    }
}

const char* Logger::to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

} // namespace bridge
