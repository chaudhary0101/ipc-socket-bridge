find_package(GTest REQUIRED)

add_executable(bridge_tests
    test_thread_safe_queue.cpp
    test_message_frame.cpp
    test_ipc_listener.cpp
    test_tcp_server.cpp
    test_message_router.cpp
    test_metrics.cpp
    test_integration.cpp
)

target_link_libraries(bridge_tests PRIVATE bridge_core GTest::gtest_main)
target_compile_options(bridge_tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(bridge_tests)
