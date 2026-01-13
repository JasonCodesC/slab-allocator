# Slab Allocator

A multithreaded slab allocator with per-thread caches, remote-free inboxes, and a simple page pool. Alignment is normalized to 1/16/64 and size classes are fixed (16..4096). Remote frees use an MPSC inbox per thread cache; page refills are batched.

## Implementation Highlights
- Thread-local fast path: `ThreadCache::pop/push` are inline, lock-free for the owner thread.
- Remote frees: MPSC inbox (`push_remote`/`drain_remote`) to batch cross-thread frees.
- Page pool: locked only on refill; slices 64KB pages into aligned blocks.
- Alignment: normalized to 1/16/64 with bitmasking for stride/headers.
- Registration: thread-local cache constructed outside the registry mutex; registry only owns pointers.

## File Structure
```
include/
  config.h, types.h, slab.h, thread_cache.h, remote_free.h, page_pool.h
src/
  slab.cpp, thread_cache.cpp, page_pool.cpp
tests/
  test_runner.cpp
  bench_basic.cpp
  bench_alignment.cpp
  bench_remote.cpp
  bench_four_thread.cpp
  bench_multialign.cpp
  bench_remote_six.cpp
  bench_remote_many_to_one.cpp
  bench_util.h
scripts/
  run_tests.sh
  run_benches.sh
out/
  bench_*.txt, tests.txt
Makefile
README.md
```

## Benchmarks (100k iterations each, latest run)
All benches randomize size classes; alignment bench covers 16, 64. Each reports total time and ops/sec.

- **basic**: slab 5.26M ops/s; malloc 6.33M.
- **alignment**
  - align 16: slab 5.80M; malloc 7.39M.
  - align 64: slab 6.35M; malloc 1.30M.
- **remote (2 threads)**: slab 3.02M; malloc 3.76M.
- **four_thread**: slab 40.8M; malloc 19.6M.
- **multialign (3 threads)**: slab 35.4M; malloc 14.0M.
- **remote_six (3 producer/consumer pairs)**: slab 2.57M; malloc 11.8M (remote contention heavy).
- **remote_many_to_one (5 producers → 1 consumer)**: slab 3.89M; malloc 6.97M (many-to-one hot spot).

### Interpretation
- Slab shines under multi-threaded and higher-align workloads (align=64, multialign, four_thread).
- Slab lags malloc in single-thread/basic and remote-heavy contention (remote_six, many-to-one), and at align=16.
- Remote contention is the biggest gap; a sharded remote-free path or reduced mutex use would help. Single-thread basic is also behind malloc.

## How to run
```bash
bash scripts/run_tests.sh      # correctness tests
bash scripts/run_benches.sh    # builds benches, writes outputs to out/bench_*.txt
```
