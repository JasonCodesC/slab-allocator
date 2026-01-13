#include "../include/thread_cache.h"

[[gnu::noinline]] void ThreadCache::push_remote(void* ptr) noexcept {
    Node* node = static_cast<Node*>(ptr);
    RemoteFree::push_MPSC(incoming_head, node);
}

[[gnu::noinline]] void ThreadCache::drain_remote() noexcept {
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
