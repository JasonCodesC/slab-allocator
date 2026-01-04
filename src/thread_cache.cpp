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