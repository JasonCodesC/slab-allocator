#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/out"

make -C "$ROOT" test
"$ROOT/out/test_runner" | tee "$ROOT/out/tests.txt"
make -C "$ROOT" clean
