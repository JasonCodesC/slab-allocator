#pragma once
#include "config.h"
#include "remote_free.h"


class ThreadCache { //represents memory that is free to be used.
    public:

    void* pop(SizeClassId size_class) noexcept; // alloc
    void push(SizeClassId size_class, void* ptr) noexcept; // free
    void push_remote(void* ptr) noexcept;
    void drain_remote() noexcept;

    private:

    struct Node {
        Node* next;
    };

    std::atomic<Node*> incoming_head{};

    std::array<Node*, NumClasses> heads{};
    std::array<std::uint32_t, NumClasses> counts{};
};