#!/usr/bin/env bash
set -euo pipefail

SOCKET_PATH="${1:-/tmp/ipc_socket_bridge.sock}"
MESSAGE="${2:-hvac:fan_speed=3}"

cmake --build build --target ipc_client_sender
./build/examples/ipc_client_sender "$SOCKET_PATH" "$MESSAGE"
