cmake_minimum_required(VERSION 3.16)
project(ipc_socket_bridge VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "ipc-socket-bridge is Linux-only because it uses epoll and POSIX IPC APIs")
endif()

option(BRIDGE_BUILD_TESTS "Build unit and integration tests" ON)
option(BRIDGE_BUILD_EXAMPLES "Build example clients" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bridge_core
    src/bridge/Logger.cpp
    src/bridge/SignalHandler.cpp
    src/bridge/BridgeConfig.cpp
    src/bridge/MessageFrame.cpp
    src/bridge/IPCListener.cpp
    src/bridge/TCPServer.cpp
    src/bridge/MessageRouter.cpp
    src/bridge/Metrics.cpp
    src/bridge/Bridge.cpp
)

target_include_directories(bridge_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(bridge_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(bridge_core PUBLIC pthread)

add_executable(ipc-socket-bridge src/bridge/main.cpp)
target_link_libraries(ipc-socket-bridge PRIVATE bridge_core)

if(BRIDGE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BRIDGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
