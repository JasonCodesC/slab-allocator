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
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    const int threads = static_cast<int>(aligns.size());
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    slab allocator;
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    auto warm = [&]() {
        std::mt19937 rng{555};
        std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
        for (int i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            std::size_t align = aligns[i % aligns.size()];
            void* p = allocator.alloc(sizes[cls], align);
            assert(p != nullptr);
            allocator.free(p);
        }
    };
    warm();

    auto start = clock_type::now();
    for (int idx = 0; idx < threads; ++idx) {
        workers.emplace_back([&, idx] {
            const std::size_t align = aligns[idx];
            std::mt19937 rng(static_cast<unsigned>(idx + 21));
            std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
            std::vector<void*> bag;
            bag.reserve(256);
            auto& ts = thread_samples[idx];
            ts.reserve(iters_per_thread);
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_thread; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(dist(rng));
                auto t0 = clock_type::now();
                void* p = allocator.alloc(sizes[cls], align);
                assert(p != nullptr);
                bag.push_back(p);
                if (bag.size() > 128) {
                    allocator.free(bag.back());
                    bag.pop_back();
                }
                auto t1 = clock_type::now();
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
            for (void* p : bag) {
                allocator.free(p);
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

static std::chrono::nanoseconds run_malloc(std::size_t iters_per_thread, std::vector<uint64_t>& samples) {
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    const int threads = static_cast<int>(aligns.size());
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);
    std::vector<std::vector<uint64_t>> thread_samples(threads);
    auto warm = [&]() {
        std::mt19937 rng{777};
        std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
        for (int i = 0; i < 1000; ++i) {
            SizeClassId cls = static_cast<SizeClassId>(dist(rng));
            std::size_t align = aligns[i % aligns.size()];
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
    for (int idx = 0; idx < threads; ++idx) {
        workers.emplace_back([&, idx] {
            const std::size_t align = aligns[idx];
            std::mt19937 rng(static_cast<unsigned>(idx + 31));
            std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
            std::vector<void*> bag;
            bag.reserve(256);
            auto& ts = thread_samples[idx];
            ts.reserve(iters_per_thread);
            sync.arrive_and_wait();
            for (std::size_t i = 0; i < iters_per_thread; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(dist(rng));
                auto t0 = clock_type::now();
                void* p = nullptr;
                if (align <= alignof(std::max_align_t)) {
                    p = std::malloc(sizes[cls]);
                } else {
                    if (posix_memalign(&p, align, sizes[cls]) != 0) { p = nullptr; }
                }
                assert(p != nullptr);
                bag.push_back(p);
                if (bag.size() > 128) {
                    std::free(bag.back());
                    bag.pop_back();
                }
                auto t1 = clock_type::now();
                ts.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
            for (void* p : bag) {
                std::free(p);
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
    constexpr std::size_t iters_per_thread = 100000;
    const double total_ops = static_cast<double>(iters_per_thread) * 3.0;
    std::vector<uint64_t> slab_samples;
    slab_samples.reserve(static_cast<std::size_t>(total_ops));
    std::vector<uint64_t> malloc_samples;
    malloc_samples.reserve(static_cast<std::size_t>(total_ops));
    auto t_slab = run_slab(iters_per_thread, slab_samples);
    auto t_malloc = run_malloc(iters_per_thread, malloc_samples);

    std::cout << "multialign iters/thread=" << iters_per_thread << "\n";
    print_latency_report("slab", t_slab, (total_ops * 1e9 / t_slab.count()), slab_samples);
    print_latency_report("malloc", t_malloc, (total_ops * 1e9 / t_malloc.count()), malloc_samples);
}
