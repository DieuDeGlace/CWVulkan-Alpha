#include "creatures/Buffs.h"
#include "creatures/Combat.h"
#include <algorithm>
#include <bit>
#include <cstring>

namespace creatures {
std::array<uint8_t, 24> Buff::serialize() const {
    std::array<uint8_t, 24> bytes{};
    bytes[0] = kind;
    std::copy(typePadding.begin(), typePadding.end(), bytes.begin() + 1);
    std::memcpy(bytes.data() + 4, &value, 4);
    std::memcpy(bytes.data() + 8, &timeMs, 4);
    std::copy(timePadding.begin(), timePadding.end(), bytes.begin() + 12);
    std::memcpy(bytes.data() + 16, &source, 8);
    return bytes;
}

Buff Buff::parse(const uint8_t *bytes) {
    Buff record;
    record.kind = bytes[0];
    std::copy_n(bytes + 1, 3, record.typePadding.begin());
    std::memcpy(&record.value, bytes + 4, 4);
    std::memcpy(&record.timeMs, bytes + 8, 4);
    std::copy_n(bytes + 12, 4, record.timePadding.begin());
    std::memcpy(&record.source, bytes + 16, 8);
    return record;
}

std::array<uint8_t, 40> BuffEvent::serialize() const {
    std::array<uint8_t, 40> bytes{};
    std::memcpy(bytes.data(), &source, 8);
    std::memcpy(bytes.data() + 8, &target, 8);
    const auto payload = record.serialize();
    std::copy(payload.begin(), payload.end(), bytes.begin() + 16);
    return bytes;
}

BuffEvent BuffEvent::parse(const uint8_t *bytes) {
    BuffEvent event;
    std::memcpy(&event.source, bytes, 8);
    std::memcpy(&event.target, bytes + 8, 8);
    event.record = Buff::parse(bytes + 16);
    return event;
}

void applyBuff(Combatant &target, const Buff &record) {
    switch (record.kind) {
    case 1: case 2: case 3: case 6: case 7: case 9: case 10: case 11:
        if (auto first = std::find_if(target.buffs.begin(), target.buffs.end(),
                [&](const Buff &present) { return present.kind == record.kind; }); first != target.buffs.end()) {
            *first = record;
            return;
        }
        break;
    default: break;
    }
    target.buffs.push_back(record);
}

std::optional<BuffEvent> consumeBuff(Combatant &target, uint8_t kind) {
    const auto first = std::find_if(target.buffs.begin(), target.buffs.end(),
        [&](const Buff &record) { return record.kind == kind; });
    if (first == target.buffs.end()) return std::nullopt;
    const Buff cleared{kind, 0, 0};
    *first = cleared;
    return BuffEvent{target.id, target.id, cleared};
}

void tickBuffs(Combatant &target, int32_t dtMs, bool authoritative,
               const std::function<void(const Buff &)> &periodic) {
    std::vector<size_t> expired;
    for (size_t index = 0; index < target.buffs.size(); index++) {
        Buff &buff = target.buffs[index];
        const int32_t prior = buff.timeMs;
        buff.timeMs = std::bit_cast<int32_t>((uint32_t)prior - (uint32_t)dtMs);
        if (buff.timeMs <= 0) {
            expired.push_back(index);
            continue;
        }
        if (buff.kind == 1 && target.hitStun > 0)
            target.hitStun = 0;
        if (buff.kind == 4 && authoritative && prior / 400 != buff.timeMs / 400 && periodic)
            periodic(buff);
    }
    // Only the nodes collected during their own visit expire now. A previously
    // visited shield depleted by later periodic work stays until the next tick.
    if (!expired.empty()) {
        size_t kept = 0, nextExpired = 0;
        for (size_t index = 0; index < target.buffs.size(); index++) {
            if (nextExpired < expired.size() && expired[nextExpired] == index)
                nextExpired++;
            else {
                if (kept != index) target.buffs[kept] = std::move(target.buffs[index]);
                kept++;
            }
        }
        target.buffs.resize(kept);
    }
}
}
