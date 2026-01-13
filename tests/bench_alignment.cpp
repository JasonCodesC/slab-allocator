#include "../include/slab.h"
#include "bench_util.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::chrono::nanoseconds run_slab(std::size_t iters, std::size_t align, std::vector<uint64_t>& samples) {
    slab allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(iters);
    std::mt19937 rng{static_cast<std::mt19937::result_type>(align * 17u)};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            void* p = allocator.alloc(sizes[cls], align);
            assert(p != nullptr);
            assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
            allocator.free(p);
        }
    };
    warm();
    auto start = clock_type::now();
    for (std::size_t i = 0; i < iters; ++i) {
        SizeClassId cls = static_cast<SizeClassId>(dist(rng));
        auto t0 = clock_type::now();
        void* p = allocator.alloc(sizes[cls], align);
        assert(p != nullptr);
        assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
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

static std::chrono::nanoseconds run_malloc(std::size_t iters, std::size_t align, std::vector<uint64_t>& samples) {
    std::vector<void*> ptrs;
    ptrs.reserve(iters);
    std::mt19937 rng{static_cast<std::mt19937::result_type>(align * 31u)};
    std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
    auto warm = [&]() {
        for (std::size_t i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            void* p = nullptr;
            if (align <= alignof(std::max_align_t)) {
                p = std::malloc(sizes[cls]);
            } else {
                if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
            }
            assert(p != nullptr);
            assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
            std::free(p);
        }
    };
    warm();
    auto start = clock_type::now();
    for (std::size_t i = 0; i < iters; ++i) {
        SizeClassId cls = static_cast<SizeClassId>(dist(rng));
        auto t0 = clock_type::now();
        void* p = nullptr;
        if (align <= alignof(std::max_align_t)) {
            p = std::malloc(sizes[cls]);
        } else {
            if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
        }
        assert(p != nullptr);
        assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
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
    const std::array<std::size_t, 2> aligns{16, 64};

    std::cout << "alignment iters=" << iters << "\n";
    for (std::size_t align : aligns) {
        std::vector<uint64_t> slab_samples;
        slab_samples.reserve(iters);
        std::vector<uint64_t> malloc_samples;
        malloc_samples.reserve(iters);

        auto t_slab = run_slab(iters, align, slab_samples);
        auto t_malloc = run_malloc(iters, align, malloc_samples);
        print_latency_report("slab", t_slab, (iters * 1e9 / t_slab.count()), slab_samples);
        print_latency_report("malloc", t_malloc, (iters * 1e9 / t_malloc.count()), malloc_samples);
    }
}
