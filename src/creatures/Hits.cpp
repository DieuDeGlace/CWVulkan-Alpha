#include "creatures/Hits.h"
#include <bit>
#include <cstring>

namespace creatures {

static_assert(std::endian::native == std::endian::little);

std::array<uint8_t, 72> HitRecord::serialize() const {
    std::array<uint8_t, 72> bytes{};
    const auto put = [&](size_t offset, const auto &value) {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    };
    put(0x00, attacker);
    put(0x08, target);
    put(0x10, amount);
    bytes[0x14] = critical;
    std::memcpy(bytes.data() + 0x15, criticalPadding.data(), criticalPadding.size());
    put(0x18, stun);
    put(0x1c, lockMs);
    std::memcpy(bytes.data() + 0x20, position.data(), sizeof(int64_t) * position.size());
    put(0x38, knockBack.x);
    put(0x3c, knockBack.y);
    put(0x40, knockBack.z);
    bytes[0x44] = area;
    bytes[0x45] = kind;
    bytes[0x46] = beam;
    bytes[0x47] = trailingPadding;
    return bytes;
}

HitRecord HitRecord::parse(const uint8_t *bytes) {
    HitRecord record;
    const auto get = [&](size_t offset, auto &value) {
        std::memcpy(&value, bytes + offset, sizeof(value));
    };
    get(0x00, record.attacker);
    get(0x08, record.target);
    get(0x10, record.amount);
    record.critical = bytes[0x14];
    std::memcpy(record.criticalPadding.data(), bytes + 0x15, record.criticalPadding.size());
    get(0x18, record.stun);
    get(0x1c, record.lockMs);
    std::memcpy(record.position.data(), bytes + 0x20, sizeof(int64_t) * record.position.size());
    get(0x38, record.knockBack.x);
    get(0x3c, record.knockBack.y);
    get(0x40, record.knockBack.z);
    record.area = bytes[0x44];
    record.kind = bytes[0x45];
    record.beam = bytes[0x46];
    record.trailingPadding = bytes[0x47];
    return record;
}

} // namespace creatures
