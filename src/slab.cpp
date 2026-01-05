#pragma once
#include "../include/slab.h"

namespace {
    thread_local ThreadCache* t_cache = nullptr;
    thread_local ThreadId t_id = 0; // MAY BE POINTLESS
}

ThreadCache* slab::ensure_registered(slab* self) noexcept {
    if (t_cache) {return t_cache;}

    std::lock_guard<std::mutex> lock(self->registry_mutex);
    t_id = static_cast<ThreadId>(self->registry.size());
    self->registry.push_back(std::make_unique<ThreadCache>());
    t_cache = self->registry.back().get();
    return t_cache;
}

void* slab::alloc(SizeClassId size, int16_t align) noexcept {
    ThreadCache* cache = ensure_registered(this);

    SizeClassId size_class = get_bucket(size);
    if (size_class >= NumClasses) {return nullptr;}

    void* ptr = cache->pop(size_class);
    if (ptr) {return ptr;}

    // fallback

    std::vector<void*> batch;
    batch.reserve(blocks_per_bin);
    pool.get_batch(size_class, t_id, blocks_per_bin, batch);

    for (void* page : batch) {
        cache->push(size_class, page);
    }

}

void slab::free(void* ptr) noexcept {

}