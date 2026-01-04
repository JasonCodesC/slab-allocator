#include "types.h"
#include <algorithm>

constexpr std::array<SizeClassId, NumClasses> sizes{16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
constexpr uint8_t page_size = 64; //KB
constexpr uint8_t blocks_per_bin = 128;