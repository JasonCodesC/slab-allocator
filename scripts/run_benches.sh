#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/out"

make -C "$ROOT" benches

declare -a benches=(
  "bench_basic"
  "bench_alignment"
  "bench_remote"
  "bench_four_thread"
  "bench_multialign"
  "bench_remote_six"
  "bench_remote_many_to_one"
)

for b in "${benches[@]}"; do
  echo "Running $b"
  "$ROOT/out/$b" | tee "$ROOT/out/${b}.txt"
done

make -C "$ROOT" clean
