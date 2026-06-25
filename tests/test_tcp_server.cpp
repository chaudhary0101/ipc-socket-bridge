#include "bridge/TCPServer.hpp"
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
namespace {
int connect_tcp(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}
} // namespace
TEST(TCPServer, StartsAndAcceptsClient) {
    bridge::BridgeConfig cfg;
    cfg.tcp_bind_address = "127.0.0.1";
    cfg.tcp_port = 19101;
    cfg.epoll_timeout = std::chrono::milliseconds(50);
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(8);
    bridge::Metrics metrics;
    bridge::TCPServer server(cfg, queue, metrics);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    int fd = connect_tcp(cfg.tcp_port);
    ASSERT_GE(fd, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    close(fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    EXPECT_GE(metrics.snapshot().tcp_connections, 1U);
}
TEST(TCPServer, RecordsErrorForInvalidBindAddress) {
    bridge::BridgeConfig cfg;
    cfg.tcp_bind_address = "not-an-ip";
    cfg.tcp_port = 19102;
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(8);
    bridge::Metrics metrics;
    bridge::TCPServer server(cfg, queue, metrics);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
    EXPECT_GE(metrics.snapshot().errors, 1U);
}
