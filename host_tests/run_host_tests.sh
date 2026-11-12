#!/usr/bin/env bash
# Build and run the PC tests for the plain C++ code in lib/gs_core.
# No ESP32 needed. Uses g++ and plain assert().
set -euo pipefail

cd "$(dirname "$0")/.."
OUT=host_tests/build
mkdir -p "$OUT"

FLAGS="-std=c++17 -Wall -Wextra -Werror -O1 -g -Ilib/gs_core"
CORE="lib/gs_core/*.cpp"

for t in test_pid test_filter test_faults test_fsm; do
  echo "== $t"
  # shellcheck disable=SC2086
  g++ $FLAGS host_tests/$t.cpp $CORE -o "$OUT/$t"
  "$OUT/$t"
done

echo "all host tests passed"
