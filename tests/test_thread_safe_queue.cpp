#include "bridge/ThreadSafeQueue.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <thread>

TEST(ThreadSafeQueue, PushAndPopPreservesOrder) {
    bridge::ThreadSafeQueue<int> queue(2);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_EQ(queue.size(), 2U);
    EXPECT_EQ(queue.wait_and_pop().value(), 1);
    EXPECT_EQ(queue.wait_and_pop().value(), 2);
}

TEST(ThreadSafeQueue, CloseUnblocksConsumerAndRejectsPush) {
    bridge::ThreadSafeQueue<int> queue(1);
    std::optional<int> result;
    std::thread consumer([&] { result = queue.wait_and_pop(); });
    queue.close();
    consumer.join();
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(queue.push(7));
    EXPECT_TRUE(queue.closed());
}
