#include "net/RemoteCreatures.h"
#include "creatures/AngleStep.h"
#include <bit>

namespace net {

void RemoteCreatures::receive(uint64_t id, const CreatureRecord &record, bool snap) {
    const auto [it, created] = list.try_emplace(id);
    Remote &r = it->second;
    // §1.6: every received field lands on the live state (the whole-record copy).
    creatures::Body &b = r.live;
    for (int axis = 0; axis < 3; axis++)
        b.position[axis] = record.position(axis);
    b.size = record.appearance().box;
    b.velocity = record.velocity();
    b.intent = record.intent();
    b.push = record.response();
    b.controller = record.controller();
    b.type = record.fighter().type;
    b.appearanceFlags = record.appearance().flags;
    b.swimmer = (b.appearanceFlags & 0x0100) != 0;
    b.flyer = (b.appearanceFlags & 0x0002) != 0;
    b.replica = true;
    r.rotation = record.orientation();
    r.health = record.health();
    // A first receipt always snaps (the drawn pose of a new creature starts at the record).
    if (snap || created) {
        r.smoothing = false;
        std::copy(b.position, b.position + 3, r.drawn.begin());
        r.drawnRotation = r.rotation;
    } else {
        r.smoothing = true;
    }
}

void RemoteCreatures::retain(const std::map<uint64_t, CreatureRecord> &listed) {
    std::erase_if(list, [&](const auto &entry) { return !listed.contains(entry.first); });
}

void RemoteCreatures::step(int32_t dtMs, const creatures::BlockType &block, const creatures::ObjectTest &objects) {
    // §2.2: the fraction f approaches 1 at 0.015 once per millisecond of the step.
    float f = 0.f;
    for (int32_t ms = 0; ms < dtMs; ms++)
        f = (1.f - f) * 0.015f + f;
    const int64_t k = int64_t(f * 65536.f);
    for (auto &[id, r] : list) {
        if (!(r.health > 0.f))
            continue;
        creatures::worldStep(r.live, dtMs, block, objects);
        const int64_t *live = r.live.position;
        if (!r.smoothing) { // §2.1: the drawn pose is a copy of the live pose
            std::copy(live, live + 3, r.drawn.begin());
            r.drawnRotation = r.rotation;
            continue;
        }
        // §2.2: each axis closes (d x T(f)) / 65536 of its gap, the vertical axis a second
        // time by the binary32 gap x f.
        for (size_t axis = 0; axis < 3; axis++) {
            const uint64_t d = uint64_t(live[axis]) - uint64_t(r.drawn[axis]);
            const int64_t e = std::bit_cast<int64_t>(d * uint64_t(k)) / 65536;
            r.drawn[axis] = std::bit_cast<int64_t>(uint64_t(r.drawn[axis]) + uint64_t(e));
        }
        const float gap = float(std::bit_cast<int64_t>(uint64_t(live[2]) - uint64_t(r.drawn[2])));
        r.drawn[2] = std::bit_cast<int64_t>(uint64_t(r.drawn[2]) + uint64_t(int64_t(gap * f)));
        // §2.3: each rotation component by the angle step.
        for (int c = 0; c < 3; c++)
            r.drawnRotation[c] = creatures::angleStep(r.drawnRotation[c], r.rotation[c], f);
    }
}

const RemoteCreatures::Remote *RemoteCreatures::find(uint64_t id) const {
    const auto it = list.find(id);
    return it == list.end() ? nullptr : &it->second;
}

} // namespace net
