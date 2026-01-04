#pragma once
#include "../include/thread_cache.h"


inline void* ThreadCache::pop(SizeClassId size_class) noexcept {
    Node* head = heads[size_class];
    if (!head) {return nullptr;};

    heads[size_class] = head->next;
    --counts[size_class];

    return static_cast<void*>(head);
}

inline void ThreadCache::push(SizeClassId size_class, void* ptr) noexcept {
    Node* node = static_cast<Node*>(ptr);
    node->next = heads[size_class];

    heads[size_class] = node;
    ++counts[size_class];
}

inline void ThreadCache::push_remote(void* ptr) noexcept{
    Node* node = static_cast<Node*>(ptr);
    RemoteFree::push_MPSC(incoming_head, node);
}

inline void ThreadCache::drain_remote() noexcept {
    Node* list = RemoteFree::steal_all(incoming_head);
    while (list) {
        Node* next = list->next;
        BlockHeader* header = reinterpret_cast<BlockHeader*>(reinterpret_cast<std::byte*>(list)
                                            - sizeof(BlockHeader));

        const SizeClassId size_class = header->size_id;
        list->next = heads[size_class];
        heads[size_class] = list;
        ++counts[size_class];
        list = next;
    }
}