#pragma once
#include "../include/slab.h"
#include <iostream>

namespace {
    thread_local ThreadCache* t_cache = nullptr;
    thread_local ThreadId t_id = 0; // MAY BE POINTLESS
    thread_local std::unique_ptr<ThreadCache> t_local_cache;

    // Canonicalize alignment to the three supported choices.
    inline std::size_t normalize_align(std::size_t align) noexcept {
        if (align >= 64) { return 64; }
        if (align >= 16) { return 16; }
        return 1;
    }
}

ThreadCache* slab::ensure_registered(slab* self) noexcept {
    if (t_cache) {return t_cache;}

    if (!t_local_cache) {
        t_local_cache = std::make_unique<ThreadCache>();
    }

    std::lock_guard<std::mutex> lock(self->registry_mutex);
    t_id = static_cast<ThreadId>(self->registry.size());
    self->registry.push_back(std::move(t_local_cache));
    t_cache = self->registry.back().get();
    return t_cache;
}

void* slab::alloc(SizeClassId size, size_t align) noexcept {
    ThreadCache* cache = ensure_registered(this);
    const std::size_t block_align = normalize_align(align);

    SizeClassId size_class = get_bucket(size);
    if (size_class >= NumClasses) {return nullptr;}


    void* ptr = cache->pop(size_class);
    if (ptr) {return ptr;}

    // fallback

    cache->drain_remote();
    void* drain_ptr = cache->pop(size_class);
    if (drain_ptr) {return drain_ptr;}

    // another fallback but slower

    std::vector<void*> batch;
    batch.reserve(blocks_per_bin);
    pool.get_batch(size_class, t_id, blocks_per_bin, batch, block_align);

    for (void* page : batch) {
        cache->push(size_class, page); // stock the shelves
    }

    return cache->pop(size_class);
}

void slab::free(void* ptr) noexcept {
    if (!ptr) {std::cerr << "bad free ptr"; return;}

    BlockHeader* header = header_from_user_ptr(ptr);
    const ThreadId owner = header->owner_id;
    const SizeClassId size_class = header->size_id;

    if (t_cache && owner == t_id) {
        t_cache->push(size_class, ptr); return;
    }

    //remote free
    ThreadCache* owner_cache = nullptr;
    {
        std::lock_guard<std::mutex> lock(registry_mutex);
        if (owner < registry.size()) {
            owner_cache = registry[owner].get();
        }
    }

    if (!owner_cache) {return;}
    owner_cache->push_remote(ptr);
}
