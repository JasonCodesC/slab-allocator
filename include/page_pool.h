#pragma once
#include "config.h"
#include <vector>
#include <array>
#include <mutex>
#include <cstdlib>
#include <cstdint>

class PagePool {
    std::array<std::vector<void*>, NumClasses> pages{};
    std::mutex mu_;
    std::array<std::byte*, NumClasses> curr{}; //curr page per class, nullptr means none
    std::array<std::uint32_t, NumClasses> remaining{}; // remainng pages per class

    void* alloc_page(std::size_t bytes) noexcept;
    void free_page(void* ptr, std::size_t bytes) noexcept;

    public:

    ~PagePool() noexcept;
    void get_batch(SizeClassId size_class, ThreadId owner, std::size_t batch, 
        std::vector<void*>& out, std::size_t align) noexcept;
    
    
};
