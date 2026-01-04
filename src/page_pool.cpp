#pragma once
#include "../include/page_pool.h"
#include <sys/mman.h>
#include <iostream>

namespace {

static inline std::size_t align_up(std::size_t x, std::size_t a) noexcept {
    return (x + (a - 1)) & ~(a - 1);
}

static inline std::byte* align_up_ptr(std::byte* p, std::size_t a) noexcept {
    auto v = reinterpret_cast<std::uintptr_t>(p);
    v = (v + (a - 1)) & ~(static_cast<std::uintptr_t>(a) - 1);
    return reinterpret_cast<std::byte*>(v);
}

}

PagePool::~PagePool() noexcept {
    for (std::size_t size_class = 0; size_class < NumClasses; ++size_class) {
        for (void* page : pages[size_class]) { free_page(page, page_size); }
    }
}

void* __attribute__((noinline)) PagePool::alloc_page(std::size_t bytes) noexcept {
    void* p = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? nullptr : p;
}

void __attribute__((noinline)) PagePool::free_page(void* ptr, std::size_t bytes) noexcept {
    ::munmap(ptr, bytes);
}

void __attribute__((noinline)) PagePool::get_batch(SizeClassId size_class, ThreadId owner,
        std::size_t batch, std::vector<void*>& out) noexcept {

    
    const std::size_t payload = sizes[size_class];
    const std::size_t align  = alignof(std::max_align_t);

    std::lock_guard<std::mutex> lk(mu_);

    out.reserve(out.size() + batch);

    auto ensure_page = [&]() noexcept {
        if (curr[size_class] != nullptr && remaining[size_class] > 0) {
            return;
        }

        void* page = alloc_page(page_size);
        if (page == nullptr) {
            std::cerr << "alloc failed"; std::exit(1);
        }

        pages[size_class].push_back(page);
        curr[size_class] = static_cast<std::byte*>(page);
        remaining[size_class] = page_size;
    };

    ensure_page();

    for (std::size_t made = 0; made < batch;) {

        std::byte* base = curr[size_class];                                 // curr pointer
        std::byte* user = align_up_ptr(base + sizeof(BlockHeader), align);  // start of users usable mem
        std::byte* header_ptr = user - sizeof(BlockHeader);                 // ptr to header
        std::byte* end = user + payload;                                    // end
        std::size_t used = static_cast<std::size_t>(end - base);            // total used
        std::size_t stride = align_up(used, align);                         // next block ptr

        // not enough space
        if (stride > remaining[size_class]) {
            curr[size_class] = nullptr;
            remaining[size_class] = 0;
            ensure_page();
            continue;
        }

        // write header
        auto* hdr = reinterpret_cast<BlockHeader*>(header_ptr);
        hdr->owner_id = owner; hdr->size_id = size_class;

        out.push_back(static_cast<void*>(user));

        curr[size_class] += stride;
        remaining[size_class] -= stride;

        ++made;
    }
}



