#include "config.h"


class slab {

    inline int8_t get_bucket(SizeClassId size) {
        auto it = std::lower_bound(sizes.begin(), sizes.end(), size);
        return *it;
    }

    public:

    void* alloc(SizeClassId size, int16_t align);
    void free(void* ptr);
};