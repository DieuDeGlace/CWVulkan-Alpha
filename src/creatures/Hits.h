#pragma once
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

namespace creatures {

enum HitKind : uint8_t { HIT_DAMAGE = 0, HIT_BLOCKED = 1, HIT_IMMUNE = 3, HIT_ABSORBED = 5 };

struct HitRecord {
    uint64_t attacker = 0, target = 0;
    float amount = 0;
    int32_t stun = 0, lockMs = 0;
    std::array<int64_t, 3> position{};
    glm::vec3 knockBack{0};
    uint8_t critical = 0, area = 0, kind = HIT_DAMAGE, beam = 0;
    std::array<uint8_t, 3> criticalPadding{};
    uint8_t trailingPadding = 0;

    std::array<uint8_t, 72> serialize() const;
    static HitRecord parse(const uint8_t *bytes); // requires a complete 72-byte record
};

// Local delivery policy is separate from the 72-byte payload. The producer's
// source/self gain is written directly and must not be applied a second time.
struct HitEmission {
    HitRecord record;
    bool applyLocally = true;
    HitEmission(const HitRecord &hit, bool apply = true) : record(hit), applyLocally(apply) {}
};

} // namespace creatures
