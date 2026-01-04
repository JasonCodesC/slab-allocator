#pragma once
#include "config.h"

namespace RemoteFree {

    template <class NodeType>
    inline void push_MPSC(std::atomic<NodeType*>& head, NodeType* n) noexcept {
        NodeType* old = head.load(std::memory_order_relaxed);
        n->next = old;
        while (!head.compare_exchange_weak(old, n, std::memory_order_release, std::memory_order_relaxed)) {
            n->next = old;
        }
    }

    template <class NodeType>
    inline NodeType* steal_all(std::atomic<NodeType*>& head) noexcept {
        return head.exchange(nullptr, std::memory_order_acquire);
    }
}