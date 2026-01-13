#include "../include/slab.h"
#include <array>
#include <barrier>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

struct TestCase {
    const char* name;
    void (*fn)();
};

static void test_basic() {
    slab allocator;
    for (SizeClassId sz : sizes) {
        std::vector<void*> ptrs;
        ptrs.reserve(256);
        for (int i = 0; i < 256; ++i) {
            void* p = allocator.alloc(sz, 1);
            assert(p != nullptr);
            ptrs.push_back(p);
        }
        for (void* p : ptrs) {
            allocator.free(p);
        }
    }
}

static void test_alignment_single_thread() {
    slab allocator;
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    for (std::size_t align : aligns) {
        void* p = allocator.alloc(64, align);
        assert(p != nullptr);
        assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
        allocator.free(p);
    }
}

static void test_remote_free_two_threads() {
    slab allocator;
    constexpr int count = 1024;
    std::vector<void*> shared(count, nullptr);
    std::barrier sync(2);

    std::thread producer([&] {
        sync.arrive_and_wait();
        for (int i = 0; i < count; ++i) {
            shared[i] = allocator.alloc(64, 1);
            assert(shared[i] != nullptr);
        }
        sync.arrive_and_wait();
    });

    std::thread consumer([&] {
        sync.arrive_and_wait();
        sync.arrive_and_wait();
        for (int i = 0; i < count; ++i) {
            allocator.free(shared[i]);
        }
    });

    producer.join();
    consumer.join();
}

static void test_four_thread_stress() {
    slab allocator;
    constexpr int threads = 4;
    constexpr int ops = 4000;
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            std::mt19937 rng(static_cast<unsigned>(t + 1));
            std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
            std::vector<void*> bag;
            bag.reserve(256);
            sync.arrive_and_wait();
            for (int i = 0; i < ops; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(dist(rng));
                void* p = allocator.alloc(sizes[cls], 1);
                assert(p != nullptr);
                bag.push_back(p);
                if (bag.size() > 128) {
                    allocator.free(bag.back());
                    bag.pop_back();
                }
            }
            for (void* p : bag) {
                allocator.free(p);
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }
}

static void test_multithread_alignment() {
    slab allocator;
    const std::array<std::size_t, 3> aligns{1, 16, 64};
    const int threads = static_cast<int>(aligns.size());
    std::barrier sync(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);

    for (int idx = 0; idx < threads; ++idx) {
        workers.emplace_back([&, idx] {
            const std::size_t align = aligns[idx];
            std::mt19937 rng(static_cast<unsigned>(idx + 11));
            std::uniform_int_distribution<int> dist(0, static_cast<int>(NumClasses - 1));
            std::vector<void*> ptrs;
            ptrs.reserve(512);
            sync.arrive_and_wait();
            for (int i = 0; i < 1024; ++i) {
                SizeClassId cls = static_cast<SizeClassId>(dist(rng));
                void* p = allocator.alloc(sizes[cls], align);
                assert(p != nullptr);
                assert(reinterpret_cast<std::uintptr_t>(p) % align == 0);
                ptrs.push_back(p);
                if (ptrs.size() > 256) {
                    allocator.free(ptrs.back());
                    ptrs.pop_back();
                }
            }
            for (void* p : ptrs) {
                allocator.free(p);
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }
}

int main() {
    const std::array<TestCase, 5> tests{{
        {"basic", test_basic},
        {"alignment_single_thread", test_alignment_single_thread},
        {"remote_free_two_threads", test_remote_free_two_threads},
        {"four_thread_stress", test_four_thread_stress},
        {"multithread_alignment", test_multithread_alignment},
    }};

    int failures = 0;
    for (const auto& t : tests) {
        try {
            t.fn();
            std::cout << "[PASS] " << t.name << '\n';
        } catch (...) {
            ++failures;
            std::cout << "[FAIL] " << t.name << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
