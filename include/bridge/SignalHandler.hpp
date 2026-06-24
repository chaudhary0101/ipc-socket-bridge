#pragma once

#include <atomic>
#include <csignal>

namespace bridge {

class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler();

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    static bool shutdown_requested();
    static void reset();

private:
    static void handle(int signo);
    struct sigaction old_int_{};
    struct sigaction old_term_{};
    struct sigaction old_pipe_{};
    static std::atomic_bool shutdown_requested_;
};

} // namespace bridge
