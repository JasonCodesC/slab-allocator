#include "../include/slab.h"
#include "bench_util.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::chrono::nanoseconds run_slab(std::size_t iters, std::vector<uint64_t>& samples) {
    slab allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(iters);
    std::mt19937 rng{12345};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            void* p = allocator.alloc(sizes[cls], 1);
            assert(p != nullptr);
            allocator.free(p);
        }
    };
    warm();
    auto start = clock_type::now();
    for (std::size_t i = 0; i < iters; ++i) {
        SizeClassId cls = static_cast<SizeClassId>(dist(rng));
        auto t0 = clock_type::now();
        void* p = allocator.alloc(sizes[cls], 1);
        assert(p != nullptr);
        ptrs.push_back(p);
        auto t1 = clock_type::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    for (void* p : ptrs) {
        allocator.free(p);
    }
    auto end = clock_type::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

static std::chrono::nanoseconds run_malloc(std::size_t iters, std::vector<uint64_t>& samples) {
    std::vector<void*> ptrs;
    ptrs.reserve(iters);
    std::mt19937 rng{67890};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            void* p = std::malloc(sizes[cls]);
            assert(p != nullptr);
            std::free(p);
        }
    };
    warm();
    auto start = clock_type::now();
    for (std::size_t i = 0; i < iters; ++i) {
        SizeClassId cls = static_cast<SizeClassId>(dist(rng));
        auto t0 = clock_type::now();
        void* p = std::malloc(sizes[cls]);
        assert(p != nullptr);
        ptrs.push_back(p);
        auto t1 = clock_type::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    for (void* p : ptrs) {
        std::free(p);
    }
    auto end = clock_type::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

int main() {
    constexpr std::size_t iters = 100000;
    std::vector<uint64_t> slab_samples;
    slab_samples.reserve(iters);
    std::vector<uint64_t> malloc_samples;
    malloc_samples.reserve(iters);

    auto t_slab = run_slab(iters, slab_samples);
    auto t_malloc = run_malloc(iters, malloc_samples);

    std::cout << "basic iters=" << iters << "\n";
    print_latency_report("slab", t_slab, (iters * 1e9 / t_slab.count()), slab_samples);
    print_latency_report("malloc", t_malloc, (iters * 1e9 / t_malloc.count()), malloc_samples);
}
