#include "../include/page_pool.h"
#include <sys/mman.h>

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

    

}



