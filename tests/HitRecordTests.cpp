#include "creatures/Hits.h"
#include "creatures/Combat.h"
#include "net/Protocol.h"
#include <bit>
#include <cstdio>
#include <cstring>

int main() {
    std::array<uint8_t, 72> bytes;
    for (size_t index = 0; index < bytes.size(); index++)
        bytes[index] = (uint8_t)(index * 37 + 11);
    const uint64_t source = 0, target = 0xfedcba9876543210;
    const uint32_t amountBits = 0xffc12345;
    const int32_t stun = INT32_MIN + 17, lock = 16777217;
    const std::array<int64_t, 3> position{INT64_MIN + 7, -1, INT64_MAX - 11};
    std::memcpy(bytes.data(), &source, 8);
    std::memcpy(bytes.data() + 8, &target, 8);
    std::memcpy(bytes.data() + 0x10, &amountBits, 4);
    std::memcpy(bytes.data() + 0x18, &stun, 4);
    std::memcpy(bytes.data() + 0x1c, &lock, 4);
    std::memcpy(bytes.data() + 0x20, position.data(), 24);
    bytes[0x14] = 0xff;
    bytes[0x44] = 0x81;
    bytes[0x45] = 5;
    bytes[0x46] = 0xfe;
    const auto hit = creatures::HitRecord::parse(bytes.data());
    bool okay = hit.attacker == source && hit.target == target && hit.stun == stun && hit.lockMs == lock &&
        hit.position == position && std::bit_cast<uint32_t>(hit.amount) == amountBits &&
        hit.critical == 0xff && hit.area == 0x81 && hit.kind == 5 && hit.beam == 0xfe && hit.serialize() == bytes;
    const auto packet = net::Hit::from(hit);
    okay &= packet.bytes == bytes && packet.lockMs() == lock && packet.record().serialize() == bytes;
    for (uint8_t kind : {uint8_t(1), uint8_t(3), uint8_t(5)}) {
        creatures::Combatant source, target;
        source.id = 41;
        target.id = 72;
        target.controller = 1;
        if (kind == 1) {
            target.action = 8;
            target.blockGauge = 1;
        } else if (kind == 3) {
            target.mode = 0x80;
        } else {
            target.buffs.push_back({6, 100, 1000});
        }
        creatures::HitInput input;
        input.amount = 10;
        input.critical = input.area = true;
        input.knockBack = {2, 4, 6};
        input.targetRecordPosition = position;
        glm::vec3 velocity(0);
        int draws = 0;
        const auto result = creatures::resolveHit(target, velocity, &source, input, [&] { draws++; return 32767; });
        if (result.records.empty()) {
            okay = false;
            continue;
        }
        const auto &record = result.records.front();
        const glm::vec3 vector = kind == 3 ? input.knockBack : input.knockBack * .5f;
        okay &= result.records.size() == 1 && record.kind == kind && record.position == position &&
            record.critical == 0 && record.area == 0 && record.beam == 0 && record.lockMs == 0 &&
            record.knockBack == vector && result.hitAccepted == (kind == 5) && draws == 1 &&
            result.sounds.size() == 1 && result.sounds[0].pitch == 1.3f;
    }
    {
        creatures::Combatant source, target;
        source.action = 28;
        target.controller = 6;
        creatures::HitInput input;
        input.amount = 10;
        input.critical = input.area = true;
        input.blockable = false;
        input.targetRecordPosition = position;
        glm::vec3 velocity(0);
        const auto result = creatures::resolveHit(target, velocity, &source, input, [] { return 32767; });
        okay &= result.hitAccepted && result.records.size() == 1 && result.records[0].critical == 1 &&
            result.records[0].area == 1 && result.records[0].beam == 1 && result.records[0].position == position;
    }
    if (!okay)
        std::puts("FAIL full hit payload, flags, signed timers, fixed-point positions or opaque bytes changed");
    else
        std::puts("all passed");
    return okay ? 0 : 1;
}
