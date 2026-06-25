#include "bridge/MessageFrame.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
namespace bridge {
std::vector<uint8_t> MessageFrame::serialize() const {
    std::vector<uint8_t> bytes(HeaderSize + payload.size());
    const uint32_t magic = htonl(Magic);
    const uint16_t version = htons(Version);
    const uint32_t seq = htonl(sequence);
    const uint32_t size = htonl(static_cast<uint32_t>(payload.size()));
    std::memcpy(bytes.data(), &magic, 4);
    std::memcpy(bytes.data() + 4, &version, 2);
    bytes[6] = static_cast<uint8_t>(source);
    bytes[7] = static_cast<uint8_t>(destination);
    std::memcpy(bytes.data() + 8, &seq, 4);
    std::memcpy(bytes.data() + 12, &size, 4);
    std::copy(payload.begin(), payload.end(), bytes.begin() + static_cast<long>(HeaderSize));
    return bytes;
}
MessageFrame MessageFrame::deserialize(const std::vector<uint8_t>& bytes, std::size_t max_payload) {
    if (bytes.size() < HeaderSize) {
        throw std::runtime_error("frame too small");
    }
    uint32_t magic = 0;
    uint16_t version = 0;
    uint32_t seq = 0;
    uint32_t payload_size = 0;
    std::memcpy(&magic, bytes.data(), 4);
    std::memcpy(&version, bytes.data() + 4, 2);
    std::memcpy(&seq, bytes.data() + 8, 4);
    std::memcpy(&payload_size, bytes.data() + 12, 4);
    magic = ntohl(magic);
    version = ntohs(version);
    seq = ntohl(seq);
    payload_size = ntohl(payload_size);
    if (magic != Magic || version != Version) {
        throw std::runtime_error("invalid frame header");
    }
    if (payload_size > max_payload || bytes.size() != HeaderSize + payload_size) {
        throw std::runtime_error("invalid payload size");
    }
    MessageFrame frame;
    frame.source = static_cast<EndpointKind>(bytes[6]);
    frame.destination = static_cast<EndpointKind>(bytes[7]);
    frame.sequence = seq;
    frame.payload.assign(bytes.begin() + static_cast<long>(HeaderSize), bytes.end());
    return frame;
}
bool MessageFrame::try_extract(std::vector<uint8_t>& buffer, std::size_t max_payload, MessageFrame& out) {
    if (buffer.size() < HeaderSize) {
        return false;
    }
    uint32_t payload_size = 0;
    std::memcpy(&payload_size, buffer.data() + 12, 4);
    payload_size = ntohl(payload_size);
    if (payload_size > max_payload) {
        throw std::runtime_error("payload exceeds configured maximum");
    }
    const std::size_t total = HeaderSize + payload_size;
    if (buffer.size() < total) {
        return false;
    }
    std::vector<uint8_t> frame_bytes(buffer.begin(), buffer.begin() + static_cast<long>(total));
    out = deserialize(frame_bytes, max_payload);
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<long>(total));
    return true;
}
std::string MessageFrame::payload_as_string() const {
    return std::string(payload.begin(), payload.end());
}
} // namespace bridge
