#pragma once
#include <cstdint>
#include <array>
#include <cstddef>

using SizeClassId = std::uint16_t;
using ThreadId = std::uint16_t;


struct BlockHeader {
    ThreadId owner_id;
    SizeClassId size_id;
};

inline BlockHeader* header_from_user_ptr(void* ptr) noexcept {
    return reinterpret_cast<BlockHeader*>(static_cast<std::byte*>(ptr) - sizeof(BlockHeader));
}