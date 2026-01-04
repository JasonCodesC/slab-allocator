#pragma once
#include "types.h"
#include <algorithm>

inline constexpr std::size_t NumClasses = 9;
inline constexpr std::array<SizeClassId, NumClasses> sizes{16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
inline constexpr std::size_t page_size = 64 * 1024; //64KB
inline constexpr uint8_t blocks_per_bin = 128;
