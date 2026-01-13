#pragma once
#include "config.h"
#include "remote_free.h"


class ThreadCache { //represents memory that is free to be used.
    public:

    [[gnu::always_inline]] inline void* pop(SizeClassId size_class) noexcept {
        Node* head = heads[size_class];
        if (!head) { return nullptr; }
        heads[size_class] = head->next;
        --counts[size_class];
        return static_cast<void*>(head);
    }

    [[gnu::always_inline]] inline void push(SizeClassId size_class, void* ptr) noexcept {
        Node* node = static_cast<Node*>(ptr);
        node->next = heads[size_class];
        heads[size_class] = node;
        ++counts[size_class];
    }
    [[gnu::noinline]] void push_remote(void* ptr) noexcept;
    [[gnu::noinline]] void drain_remote() noexcept;

    private:

    struct Node {
        Node* next;
    };

    std::atomic<Node*> incoming_head{};

    std::array<Node*, NumClasses> heads{};
    std::array<std::uint16_t, NumClasses> counts{};
};
