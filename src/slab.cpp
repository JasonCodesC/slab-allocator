#include "../include/slab.h"

namespace {
    thread_local ThreadCache* t_cache = nullptr;
    thread_local ThreadId t_id = 0; // MAY BE POINTLESS
}

void* slab::alloc(SizeClassId size, int16_t align) {
    SizeClassId bucket_size = get_bucket(size);
}

void slab::free(void* ptr) {

}