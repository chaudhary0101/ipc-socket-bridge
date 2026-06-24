#include "bridge/Bridge.hpp"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

TEST(Integration, BridgeStartsAndStopsCleanly) {
    bridge::BridgeConfig cfg;
    cfg.tcp_bind_address = "127.0.0.1";
    cfg.tcp_port = 19110;
    cfg.unix_socket_path = "/tmp/ipc_bridge_integration.sock";
    cfg.fifo_path = "/tmp/ipc_bridge_integration.fifo";
    cfg.enable_fifo = false;
    cfg.epoll_timeout = std::chrono::milliseconds(50);
    cfg.log_console = false;
    bridge::Bridge app(cfg);
    app.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    app.stop();
    EXPECT_GE(app.metrics().uptime_seconds, 0U);
}

TEST(Integration, InvalidConfigurationThrows) {
    bridge::BridgeConfig cfg;
    cfg.enable_fifo = false;
    cfg.enable_unix_socket = false;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}
