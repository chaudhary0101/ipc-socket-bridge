#include "bridge/MessageFrame.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char** argv) {
    const std::string socket_path = argc > 1 ? argv[1] : "/tmp/ipc_socket_bridge.sock";
    const std::string text = argc > 2 ? argv[2] : "hvac:set_temp=22";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path.c_str());
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        perror("connect");
        close(fd);
        return 1;
    }
    bridge::MessageFrame frame;
    frame.source = bridge::EndpointKind::Ipc;
    frame.destination = bridge::EndpointKind::Tcp;
    frame.sequence = 1;
    frame.payload.assign(text.begin(), text.end());
    auto bytes = frame.serialize();
    const ssize_t sent = send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL);
    close(fd);
    if (sent != static_cast<ssize_t>(bytes.size())) {
        std::cerr << "short write\n";
        return 1;
    }
    std::cout << "sent IPC frame: " << text << '\n';
    return 0;
}
