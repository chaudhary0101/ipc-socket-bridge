#include "bridge/MessageFrame.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {
int connect_with_backoff(const std::string& host, uint16_t port) {
    auto delay = std::chrono::milliseconds(100);
    while (true) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            return fd;
        }
        close(fd);
        std::cerr << "connect failed, retrying in " << delay.count() << " ms\n";
        std::this_thread::sleep_for(delay);
        delay = std::min(delay * 2, std::chrono::milliseconds(5000));
    }
}
} // namespace

int main(int argc, char** argv) {
    const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t port = argc > 2 ? static_cast<uint16_t>(std::stoul(argv[2])) : 9090;
    std::vector<uint8_t> buffer;
    while (true) {
        int fd = connect_with_backoff(host, port);
        std::cout << "connected to " << host << ':' << port << '\n';
        uint8_t chunk[4096];
        while (true) {
            const ssize_t got = recv(fd, chunk, sizeof(chunk), 0);
            if (got <= 0) {
                close(fd);
                std::cerr << "connection lost, reconnecting\n";
                break;
            }
            buffer.insert(buffer.end(), chunk, chunk + got);
            bridge::MessageFrame frame;
            while (bridge::MessageFrame::try_extract(buffer, 65536, frame)) {
                std::cout << "frame[" << frame.sequence << "] " << frame.payload_as_string() << '\n';
            }
        }
    }
}
