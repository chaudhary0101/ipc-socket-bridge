#include "bridge/SignalHandler.hpp"

#include <cstring>

namespace bridge {

std::atomic_bool SignalHandler::shutdown_requested_{false};

SignalHandler::SignalHandler() {
    reset();
    struct sigaction action {};
    action.sa_handler = &SignalHandler::handle;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, &old_int_);
    sigaction(SIGTERM, &action, &old_term_);

    struct sigaction ignore {};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGPIPE, &ignore, &old_pipe_);
}

SignalHandler::~SignalHandler() {
    sigaction(SIGINT, &old_int_, nullptr);
    sigaction(SIGTERM, &old_term_, nullptr);
    sigaction(SIGPIPE, &old_pipe_, nullptr);
}

bool SignalHandler::shutdown_requested() {
    return shutdown_requested_.load();
}

void SignalHandler::reset() {
    shutdown_requested_.store(false);
}

void SignalHandler::handle(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        shutdown_requested_.store(true);
    }
}

} // namespace bridge
