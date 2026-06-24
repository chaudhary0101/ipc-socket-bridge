#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace bridge {

enum class LogLevel { Debug = 0, Info, Warn, Error };

class Logger {
public:
    static Logger& instance();

    void configure(LogLevel min_level, const std::string& file_path, bool console_enabled);
    void log(LogLevel level, const std::string& component, const std::string& message);

private:
    Logger() = default;
    static const char* to_string(LogLevel level);
    static std::string timestamp();

    std::mutex mutex_;
    LogLevel min_level_{LogLevel::Info};
    std::ofstream file_;
    bool console_enabled_{true};
};

} // namespace bridge
