#include "bridge/IPCListener.hpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
namespace {
bridge::BridgeConfig ipc_config(const std::string& suffix) {
    bridge::BridgeConfig cfg;
    cfg.unix_socket_path = "/tmp/ipc_bridge_test_" + suffix + ".sock";
    cfg.fifo_path = "/tmp/ipc_bridge_test_" + suffix + ".fifo";
    cfg.enable_fifo = false;
    cfg.tcp_port = 19090;
    cfg.epoll_timeout = std::chrono::milliseconds(50);
    return cfg;
}
} // namespace
TEST(IPCListener, StartsAndAcceptsUnixSocketClient) {
    auto cfg = ipc_config("accept");
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(8);
    bridge::Metrics metrics;
    bridge::IPCListener listener(cfg, queue, metrics);
    listener.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", cfg.unix_socket_path.c_str());
    EXPECT_EQ(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    close(fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    listener.stop();
    EXPECT_GE(metrics.snapshot().ipc_connections, 1U);
}
TEST(IPCListener, RejectsInvalidUnixPathAtStartup) {
    auto cfg = ipc_config("bad");
    cfg.unix_socket_path = "/tmp/missing_dir/ipc.sock";
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(8);
    bridge::Metrics metrics;
    bridge::IPCListener listener(cfg, queue, metrics);
    listener.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    listener.stop();
    EXPECT_GE(metrics.snapshot().errors, 1U);
}
