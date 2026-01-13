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

static std::chrono::nanoseconds run_slab(std::size_t iters_per_pair, std::vector<uint64_t>& samples) {
    slab allocator;
    constexpr int threads = 6;
    constexpr int pairs = threads / 2;
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::array<std::vector<void*>, pairs> shared{};
    for (auto& v : shared) { v.resize(iters_per_pair, nullptr); }
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    auto warm = [&]() {
        std::mt19937 rng{888};
        std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
        std::uniform_int_distribution<int> align_dist(0, 2);
        for (int i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
            std::size_t align = aligns[align_dist(rng)];
            void* p = allocator.alloc(sizes[cls], align);
            assert(p != nullptr);
            allocator.free(p);
        }
    };
    warm();

    auto start = clock_type::now();
    for (int p = 0; p < pairs; ++p) {
        workers.emplace_back([&, p] {
            std::mt19937 rng(static_cast<unsigned>(p + 101));
            std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
            std::uniform_int_distribution<int> align_dist(0, 2);
            auto& ts = thread_samples[p * 2];
            ts.reserve(iters_per_pair);
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_pair; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
                std::size_t align = aligns[align_dist(rng)];
                auto t0 = clock_type::now();
                shared[p][i] = allocator.alloc(sizes[cls], align);
                auto t1 = clock_type::now();
                assert(shared[p][i] != nullptr);
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
            sync.arrive_and_wait();
        });
        workers.emplace_back([&, p] {
            std::mt19937 rng(static_cast<unsigned>(p + 201));
            (void)rng;
            auto& ts = thread_samples[p * 2 + 1];
            ts.reserve(iters_per_pair);
            sync.arrive_and_wait();
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_pair; ++i) {
                auto t0 = clock_type::now();
                allocator.free(shared[p][i]);
                auto t1 = clock_type::now();
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
        });
    }
    for (auto& th : workers) th.join();
    auto end = clock_type::now();
    for (auto& v : thread_samples) {
        samples.insert(samples.end(), v.begin(), v.end());
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

static std::chrono::nanoseconds run_malloc(std::size_t iters_per_pair, std::vector<uint64_t>& samples) {
    constexpr int threads = 6;
    constexpr int pairs = threads / 2;
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::array<std::vector<void*>, pairs> shared{};
    for (auto& v : shared) { v.resize(iters_per_pair, nullptr); }
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    auto warm = [&]() {
        std::mt19937 rng{9999};
        std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
        std::uniform_int_distribution<int> align_dist(0, 2);
        for (int i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
            std::size_t align = aligns[align_dist(rng)];
            void* p = nullptr;
            if (align <= alignof(std::max_align_t)) {
                p = std::malloc(sizes[cls]);
            } else {
                if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
            }
            assert(p != nullptr);
            std::free(p);
        }
    };
    warm();

    auto start = clock_type::now();
    for (int p = 0; p < pairs; ++p) {
        workers.emplace_back([&, p] {
            std::mt19937 rng(static_cast<unsigned>(p + 301));
            std::uniform_int_distribution<int> size_dist(0, static_cast<int>(NumClasses - 1));
            std::uniform_int_distribution<int> align_dist(0, 2);
            auto& ts = thread_samples[p * 2];
            ts.reserve(iters_per_pair);
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_pair; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(size_dist(rng));
                std::size_t align = aligns[align_dist(rng)];
                auto t0 = clock_type::now();
                void* pptr = nullptr;
                if (align <= alignof(std::max_align_t)) {
                    pptr = std::malloc(sizes[cls]);
                } else {
                    if (posix_memalign(&pptr, align, sizes[cls]) != 0) { pptr = nullptr; }
                }
                shared[p][i] = pptr;
                auto t1 = clock_type::now();
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
            sync.arrive_and_wait();
        });
        workers.emplace_back([&, p] {
            std::mt19937 rng(static_cast<unsigned>(p + 401));
            (void)rng;
            auto& ts = thread_samples[p * 2 + 1];
            ts.reserve(iters_per_pair);
            sync.arrive_and_wait();
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_pair; ++i) {
                auto t0 = clock_type::now();
                std::free(shared[p][i]);
                auto t1 = clock_type::now();
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
        });
    }
    for (auto& th : workers) th.join();
    auto end = clock_type::now();
    for (auto& v : thread_samples) {
        samples.insert(samples.end(), v.begin(), v.end());
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

int main() {
    constexpr std::size_t iters_per_pair = 100000;
    const double total_ops = static_cast<double>(iters_per_pair) * 3.0; // producers only
    std::vector<uint64_t> slab_samples;
    slab_samples.reserve(static_cast<std::size_t>(total_ops * 2)); // include frees
    std::vector<uint64_t> malloc_samples;
    malloc_samples.reserve(static_cast<std::size_t>(total_ops * 2));
    auto t_slab = run_slab(iters_per_pair, slab_samples);
    auto t_malloc = run_malloc(iters_per_pair, malloc_samples);

    std::cout << "remote_six iters/pair=" << iters_per_pair << "\n";
    print_latency_report("slab", t_slab, (total_ops * 1e9 / t_slab.count()), slab_samples);
    print_latency_report("malloc", t_malloc, (total_ops * 1e9 / t_malloc.count()), malloc_samples);
}
