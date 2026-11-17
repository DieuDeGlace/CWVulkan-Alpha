#include "CreatureShading.h"
#include <algorithm>

namespace creature_shading {

int standingLight(uint8_t type, uint8_t light) {
    if (type == 13)
        return 255;
    if (type == 0 || type == 2)
        return std::max<int>(light, 5);
    return 0;
}

glm::vec4 material(int standingLight, const creatures::Combatant &c, bool deferred) {
    glm::vec4 m(1, 1, 1, float(standingLight) / 255.f);
    if (c.slowMs > 0)
        m *= glm::vec4(.5f, .5f, 1.5f, 1);
    for (const creatures::Buff &effect : c.buffs) {
        if (effect.kind == 1 || (!deferred && effect.kind == 2))
            m *= glm::vec4(1, .5f, .5f, 1);
        else if (!deferred && effect.kind == 4)
            m *= glm::vec4(.5f, 1.5f, .5f, 1);
    }
    return m;
}

bool hasMotionTrail(const creatures::Combatant &c) {
    const int32_t a = c.action;
    if (c.characterClass == 4 && c.specialization == 1 && c.actionMs < creatures::duration(c, a) &&
        (a == 0x11 || a == 0x05 || a == 0x14))
        return true;
    return a == 0x30 || a == 0x5D || c.hasBuff(12) || c.hasBuff(3);
}

Sequence drawSequence(bool trail, float stealth) {
    Sequence s;
    if (trail)
        for (int k = 0; k < 5; k++) {
            const float share = float(k) / 5.f;
            s.draws[s.count++] = {Draw::Ghost, .5f * share, -.07f * (1.f - share)};
        }
    s.draws[s.count++] = stealth > 0.f ? Draw{Draw::DeferredBody, 1.f - .75f * stealth, 0}
                                       : Draw{Draw::Body, 1, 0};
    return s;
}

} // namespace creature_shading
