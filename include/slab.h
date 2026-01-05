#pragma once
#include "config.h"
#include "thread_cache.h"
#include <mutex>
#include <vector>
#include <memory>
#include "page_pool.h"
#include <limits>


class slab {

    static ThreadCache* slab::ensure_registered(slab* self) noexcept;

    // Maybe switch to linear scan cuz only 9 els so big O is neglible
    // so test this
    inline SizeClassId get_bucket(SizeClassId size) noexcept {
        auto it = std::lower_bound(sizes.begin(), sizes.end(), size);
        if (it == sizes.end()) {
            return static_cast<SizeClassId>(NumClasses);
        }
        return static_cast<SizeClassId>(it - sizes.begin());
    }
    std::mutex registry_mutex;
    std::vector<std::unique_ptr<ThreadCache>> registry;
    PagePool pool;

    public:

    void* alloc(SizeClassId size, int16_t align) noexcept;
    void free(void* ptr) noexcept;
};