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

    #pragma unroll
   [[gnu::always_inline]] inline constexpr SizeClassId get_bucket(SizeClassId size) noexcept {
        for (SizeClassId i = 0; i < NumClasses; ++i) {
            if (size <= sizes[i]) { return i; }
        }
        return static_cast<SizeClassId>(NumClasses);
    }
    std::mutex registry_mutex;
    std::vector<std::unique_ptr<ThreadCache>> registry;
    PagePool pool;

    public:

    void* alloc(SizeClassId size, std::size_t align) noexcept;
    void free(void* ptr) noexcept;
};
