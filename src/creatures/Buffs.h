#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace creatures {
enum BuffKind : uint8_t { DAMAGE_TAKEN = 1, DAMAGE_OVER_TIME = 4, ABSORB = 6, SURE_CRITICAL = 11, HASTE = 12 };

struct Buff {
    uint8_t kind = 0;
    float value = 0;
    int32_t timeMs = 0;
    uint64_t source = 0;
    // Opaque bytes survive received-record copies; newly made records use zero.
    std::array<uint8_t, 3> typePadding{};
    std::array<uint8_t, 4> timePadding{};
    std::array<uint8_t, 24> serialize() const;
    static Buff parse(const uint8_t *record); // requires a complete 24-byte record
};

struct BuffEvent {
    uint64_t source = 0, target = 0;
    Buff record; // its source is independent of the two outer event IDs
    std::array<uint8_t, 40> serialize() const;
    static BuffEvent parse(const uint8_t *event); // requires a complete 40-byte event
};

struct Combatant;
// Replace the first selected-type match in place, or append. Nonpositive time
// remains resident until the subsequent countdown/expiration phase.
void applyBuff(Combatant &target, const Buff &record);
// Consume the first matching node through a zero-time replacement, leaving it
// visible until countdown. Returns the copied self/self event when found.
std::optional<BuffEvent> consumeBuff(Combatant &target, uint8_t kind);
// Periodic work may change payloads, but must not mutate the buff collection.
void tickBuffs(Combatant &target, int32_t dtMs, bool authoritative,
               const std::function<void(const Buff &)> &periodic = {});
}
