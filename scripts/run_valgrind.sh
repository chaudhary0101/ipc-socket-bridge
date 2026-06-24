#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBRIDGE_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=7 ./build/tests/bridge_tests
