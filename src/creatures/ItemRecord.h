#pragma once
#include <array>
#include <cstdint>

namespace creatures {
// Complete little-endian item value used by the existing content/wire codecs.
using ItemRecord = std::array<uint8_t, 280>;
}
