#include "bridge/Metrics.hpp"

#include <gtest/gtest.h>

TEST(Metrics, RecordsCountersAndSnapshot) {
    bridge::Metrics metrics;
    metrics.record_ipc_connection();
    metrics.record_tcp_connection();
    metrics.record_routed(128);
    metrics.record_error();
    auto snapshot = metrics.snapshot();
    EXPECT_EQ(snapshot.ipc_connections, 1U);
    EXPECT_EQ(snapshot.tcp_connections, 1U);
    EXPECT_EQ(snapshot.messages_routed, 1U);
    EXPECT_EQ(snapshot.bytes_transferred, 128U);
    EXPECT_EQ(snapshot.errors, 1U);
    EXPECT_NE(metrics.to_string().find("messages=1"), std::string::npos);
}
