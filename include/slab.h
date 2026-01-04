#pragma once
#include "config.h"
#include "thread_cache.h"
#include <mutex>
#include <vector>
#include <memory>


class slab {
    // Maybe switch to linear scan cuz only 9 els so big O is neglible
    // so test this
    inline int8_t get_bucket(SizeClassId size) noexcept { // Get correct bucket size quickly
        auto it = std::lower_bound(sizes.begin(), sizes.end(), size);
        return *it;
    }
    std::mutex registry_mutex;
    std::vector<std::unique_ptr<ThreadCache>> registry;

    public:

    void* alloc(SizeClassId size, int16_t align) noexcept;
    void free(void* ptr) noexcept;
};