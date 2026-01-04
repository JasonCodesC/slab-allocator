#include "config.h"


class ThreadCache { //represents memory that is free to be used.

    void* pop(SizeClassId size_class) noexcept; // alloc
    void push(SizeClassId size_class, void* ptr) noexcept; // free

    private:

    struct Node {
        Node* next;
    };

    std::array<Node*, NumClasses> heads{};
    std::array<std::uint32_t, NumClasses> counts{};
};