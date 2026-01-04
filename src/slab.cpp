#include "../include/slab.h"

void* slab::alloc(SizeClassId size, int16_t align) {
    SizeClassId bucket_size = get_bucket(size);
}

void slab::free(void* ptr) {

}