#include "../include/slab.h"
#include "bench_util.h"
#include <array>
#include <barrier>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::chrono::nanoseconds run_slab(std::size_t iters_per_thread, std::vector<uint64_t>& samples) {
    slab allocator;
    constexpr int threads = 6;
    const int owner_idx = 0;
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::vector<void*> inbox;
    inbox.resize(iters_per_thread * (threads - 1));
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    thread_samples[owner_idx].reserve(iters_per_thread * (threads - 1));

    auto start = clock_type::now();
    for (int t = 0; t < threads; ++t) {
        if (t == owner_idx) {
            workers.emplace_back([&, t] {
                sync.arrive_and_wait();
                sync.arrive_and_wait();
                for (void* p : inbox) {
                    auto t0 = clock_type::now();
                    allocator.free(p);
                    auto t1 = clock_type::now();
                    thread_samples[t].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                }
            });
        } else {
            workers.emplace_back([&, t] {
                std::mt19937 rng(static_cast<unsigned>(t + 500));
                std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
                std::uniform_int_distribution<int> align_dist(0, 2);
                auto& ts = thread_samples[t];
                ts.reserve(iters_per_thread);
                sync.arrive_and_wait();
                for (std::size_t i = 0; i < iters_per_thread; ++i) {
                    SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
                    std::size_t align = aligns[align_dist(rng)];
                    auto t0 = clock_type::now();
                    void* p = allocator.alloc(sizes[cls], align);
                    auto t1 = clock_type::now();
                    assert(p != nullptr);
                    inbox[(t - 1) * iters_per_thread + i] = p;
                    ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                }
                sync.arrive_and_wait();
            });
        }
    }
    for (auto& th : workers) th.join();
    auto end = clock_type::now();
    for (auto& v : thread_samples) {
        samples.insert(samples.end(), v.begin(), v.end());
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

static std::chrono::nanoseconds run_malloc(std::size_t iters_per_thread, std::vector<uint64_t>& samples) {
    constexpr int threads = 6;
    const int owner_idx = 0;
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::vector<void*> inbox;
    inbox.resize(iters_per_thread * (threads - 1));
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    thread_samples[owner_idx].reserve(iters_per_thread * (threads - 1));

    auto start = clock_type::now();
    for (int t = 0; t < threads; ++t) {
        if (t == owner_idx) {
            workers.emplace_back([&, t] {
                sync.arrive_and_wait();
                sync.arrive_and_wait();
                for (void* p : inbox) {
                    auto t0 = clock_type::now();
                    std::free(p);
                    auto t1 = clock_type::now();
                    thread_samples[t].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                }
            });
        } else {
            workers.emplace_back([&, t] {
                std::mt19937 rng(static_cast<unsigned>(t + 600));
                std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
                std::uniform_int_distribution<int> align_dist(0, 2);
                auto& ts = thread_samples[t];
                ts.reserve(iters_per_thread);
                sync.arrive_and_wait();
                for (std::size_t i = 0; i < iters_per_thread; ++i) {
                    SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
                    std::size_t align = aligns[align_dist(rng)];
                    auto t0 = clock_type::now();
                    void* p = nullptr;
                    if (align <= alignof(std::max_align_t)) {
                        p = std::malloc(sizes[cls]);
                    } else {
                        if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
                    }
                    assert(p != nullptr);
                    inbox[(t - 1) * iters_per_thread + i] = p;
                    auto t1 = clock_type::now();
                    ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                }
                sync.arrive_and_wait();
            });
        }
    }
    for (auto& th : workers) th.join();
    auto end = clock_type::now();
    for (auto& v : thread_samples) {
        samples.insert(samples.end(), v.begin(), v.end());
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

int main() {
    constexpr std::size_t iters_per_thread = 100000;
    const double producer_ops = static_cast<double>(iters_per_thread) * 5.0;
    std::vector<uint64_t> slab_samples;
    slab_samples.reserve(static_cast<std::size_t>(producer_ops * 2));
    std::vector<uint64_t> malloc_samples;
    malloc_samples.reserve(static_cast<std::size_t>(producer_ops * 2));
    auto t_slab = run_slab(iters_per_thread, slab_samples);
    auto t_malloc = run_malloc(iters_per_thread, malloc_samples);

    std::cout << "remote_many_to_one iters/producer=" << iters_per_thread << "\n";
    print_latency_report("slab", t_slab, (producer_ops * 1e9 / t_slab.count()), slab_samples);
    print_latency_report("malloc", t_malloc, (producer_ops * 1e9 / t_malloc.count()), malloc_samples);
}
