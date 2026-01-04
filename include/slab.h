#include "config.h"
#include "thread_cache.h"
#include <mutex>


class slab {

    inline int8_t get_bucket(SizeClassId size) {
        auto it = std::lower_bound(sizes.begin(), sizes.end(), size);
        return *it;
    }
    std::mutex registry_mutex;
    std::vector<std::unique_ptr<ThreadCache>> registry;

    public:

    void* alloc(SizeClassId size, int16_t align);
    void free(void* ptr);
};