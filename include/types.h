#include <cstdint>
#include <array>

using SizeClassId = std::uint16_t;
using ThreadId = std::uint16_t;
static constexpr std::size_t NumClasses = 9;

struct BlockHeader {
    ThreadId owner_id;
    SizeClassId size_id;
};