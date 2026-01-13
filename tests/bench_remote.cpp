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

static std::chrono::nanoseconds run_slab(std::size_t iters, std::vector<uint64_t>& samples) {
    slab allocator;
    std::vector<void*> shared(iters);
    std::barrier sync(2);
    std::mt19937 rng{123};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    std::uniform_int_distribution<int> align_dist(0, 2);
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<uint64_t> prod_samples;
    prod_samples.reserve(iters);
    std::vector<uint64_t> cons_samples;
    cons_samples.reserve(iters);
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            std::size_t align = aligns[align_dist(rng)];
            void* p = allocator.alloc(sizes[cls], align);
            assert(p != nullptr);
            allocator.free(p);
        }
    };
    warm();

    auto start = clock_type::now();
    std::thread producer([&] {
        sync.arrive_and_wait();
        for (std::size_t i = 0; i < iters; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            std::size_t align = aligns[align_dist(rng)];
            auto t0 = clock_type::now();
            shared[i] = allocator.alloc(sizes[cls], align);
            auto t1 = clock_type::now();
            assert(shared[i] != nullptr);
            prod_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
        sync.arrive_and_wait();
    });

    std::thread consumer([&] {
        sync.arrive_and_wait();
        sync.arrive_and_wait();
        for (std::size_t i = 0; i < iters; ++i) {
            auto t0 = clock_type::now();
            allocator.free(shared[i]);
            auto t1 = clock_type::now();
            cons_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
    });

    producer.join();
    consumer.join();
    samples.insert(samples.end(), prod_samples.begin(), prod_samples.end());
    samples.insert(samples.end(), cons_samples.begin(), cons_samples.end());
    auto end = clock_type::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

static std::chrono::nanoseconds run_malloc(std::size_t iters, std::vector<uint64_t>& samples) {
    std::vector<void*> shared(iters);
    std::barrier sync(2);
    std::mt19937 rng{321};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    std::uniform_int_distribution<int> align_dist(0, 2);
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    std::vector<uint64_t> prod_samples;
    prod_samples.reserve(iters);
    std::vector<uint64_t> cons_samples;
    cons_samples.reserve(iters);
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
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
    std::thread producer([&] {
        sync.arrive_and_wait();
        for (std::size_t i = 0; i < iters; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            std::size_t align = aligns[align_dist(rng)];
            auto t0 = clock_type::now();
            void* p = nullptr;
            if (align <= alignof(std::max_align_t)) {
                p = std::malloc(sizes[cls]);
            } else {
                if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
            }
            assert(p != nullptr);
            shared[i] = p;
            auto t1 = clock_type::now();
            prod_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
        sync.arrive_and_wait();
    });

    std::thread consumer([&] {
        sync.arrive_and_wait();
        sync.arrive_and_wait();
        for (std::size_t i = 0; i < iters; ++i) {
            auto t0 = clock_type::now();
            std::free(shared[i]);
            auto t1 = clock_type::now();
            cons_samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
    });

    producer.join();
    consumer.join();
    samples.insert(samples.end(), prod_samples.begin(), prod_samples.end());
    samples.insert(samples.end(), cons_samples.begin(), cons_samples.end());
    auto end = clock_type::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

int main() {
    constexpr std::size_t iters = 100000;
    std::vector<uint64_t> slab_samples;
    slab_samples.reserve(iters * 2);
    std::vector<uint64_t> malloc_samples;
    malloc_samples.reserve(iters * 2);

    auto t_slab = run_slab(iters, slab_samples);
    auto t_malloc = run_malloc(iters, malloc_samples);

    std::cout << "remote iters=" << iters << "\n";
    print_latency_report("slab", t_slab, (iters * 1e9 / t_slab.count()), slab_samples);
    print_latency_report("malloc", t_malloc, (iters * 1e9 / t_malloc.count()), malloc_samples);
}
