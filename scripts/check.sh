#!/usr/bin/env bash
# Quick check of everything that runs without the ESP32.
# Usage: ./scripts/check.sh   (from anywhere)
set -euo pipefail
cd "$(dirname "$0")/.."

echo "== 1. host tests (PID, filter, faults, FSM, telemetry format)"
./host_tests/run_host_tests.sh

echo "== 2. python tool tests"
(cd tools && python3 -m unittest -q)

echo "== 3. fake telemetry run"
python3 tools/groundstation.py --fake 600 --out tools/out/check

echo "== 4. secrets"
if git ls-files --error-unmatch include/secrets.h >/dev/null 2>&1; then
  echo "include/secrets.h is tracked by git, remove it: git rm --cached include/secrets.h"
  exit 1
fi
[ -f include/secrets.h ] || echo "note: include/secrets.h missing, copy include/secrets.example.h before building"

echo "== 5. firmware build (only if PlatformIO is installed)"
if command -v pio >/dev/null 2>&1; then
  pio run -e esp32dev
else
  echo "pio not found, skipped"
fi

echo "check done"
