#pragma once
#include "config.h"
#include "remote_free.h"


class ThreadCache { //represents memory that is free to be used.

    inline void* pop(SizeClassId size_class) noexcept; // alloc
    inline void push(SizeClassId size_class, void* ptr) noexcept; // free
    inline void push_remote(void* ptr) noexcept;
    inline void drain_remote() noexcept;

    private:

    struct Node {
        Node* next;
    };

    std::atomic<Node*> incoming_head{};

    std::array<Node*, NumClasses> heads{};
    std::array<std::uint32_t, NumClasses> counts{};
};