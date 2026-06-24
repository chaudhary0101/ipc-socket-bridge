#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace bridge {

enum class EndpointKind : uint8_t { Ipc = 1, Tcp = 2 };

struct MessageFrame {
    static constexpr uint32_t Magic = 0x49504342; // IPCB
    static constexpr uint16_t Version = 1;
    static constexpr std::size_t HeaderSize = 16;

    EndpointKind source{EndpointKind::Ipc};
    EndpointKind destination{EndpointKind::Tcp};
    uint32_t sequence{0};
    std::vector<uint8_t> payload;

    std::vector<uint8_t> serialize() const;
    static MessageFrame deserialize(const std::vector<uint8_t>& bytes, std::size_t max_payload);
    static bool try_extract(std::vector<uint8_t>& buffer, std::size_t max_payload, MessageFrame& out);
    std::string payload_as_string() const;
};

} // namespace bridge
