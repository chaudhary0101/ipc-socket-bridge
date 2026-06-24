#include "bridge/MessageFrame.hpp"

#include <gtest/gtest.h>

TEST(MessageFrame, SerializesAndDeserializesPayload) {
    bridge::MessageFrame frame;
    frame.source = bridge::EndpointKind::Ipc;
    frame.destination = bridge::EndpointKind::Tcp;
    frame.sequence = 42;
    frame.payload = {'h', 'v', 'a', 'c'};
    auto decoded = bridge::MessageFrame::deserialize(frame.serialize(), 1024);
    EXPECT_EQ(decoded.sequence, 42U);
    EXPECT_EQ(decoded.payload_as_string(), "hvac");
}

TEST(MessageFrame, RejectsMalformedAndOversizedFrames) {
    EXPECT_THROW(bridge::MessageFrame::deserialize({1, 2, 3}, 1024), std::runtime_error);
    bridge::MessageFrame frame;
    frame.payload.resize(8, 'x');
    EXPECT_THROW(bridge::MessageFrame::deserialize(frame.serialize(), 4), std::runtime_error);
}

TEST(MessageFrame, ExtractsPartialFramesFromBuffer) {
    bridge::MessageFrame frame;
    frame.sequence = 9;
    frame.payload = {'o', 'k'};
    auto bytes = frame.serialize();
    std::vector<uint8_t> buffer(bytes.begin(), bytes.begin() + 5);
    bridge::MessageFrame out;
    EXPECT_FALSE(bridge::MessageFrame::try_extract(buffer, 1024, out));
    buffer.insert(buffer.end(), bytes.begin() + 5, bytes.end());
    EXPECT_TRUE(bridge::MessageFrame::try_extract(buffer, 1024, out));
    EXPECT_EQ(out.sequence, 9U);
    EXPECT_TRUE(buffer.empty());
}
