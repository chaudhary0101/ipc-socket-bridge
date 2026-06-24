#include "bridge/MessageRouter.hpp"

#include <gtest/gtest.h>

TEST(MessageRouter, QueueCanCarryRoutedMessages) {
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(4);
    bridge::MessageFrame frame;
    frame.payload = {'a'};
    EXPECT_TRUE(queue.push({bridge::EndpointKind::Ipc, frame}));
    auto item = queue.wait_and_pop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->source, bridge::EndpointKind::Ipc);
    EXPECT_EQ(item->frame.payload_as_string(), "a");
}

TEST(MessageRouter, ClosedQueueReturnsNoMessage) {
    bridge::ThreadSafeQueue<bridge::RoutedMessage> queue(1);
    queue.close();
    EXPECT_FALSE(queue.wait_and_pop().has_value());
}
