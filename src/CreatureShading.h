#pragma once
// How a creature joins the frame (alpha-rendering.md §11.2, alpha-rulings-rendering-clouds-
// lights.md §16): its material tints, its hit flash, the five-ghost motion trail and the
// translucent body that waits until after the water. Pure rules; CreatureView turns them
// into draw items.
#include "creatures/Combat.h"
#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace creature_shading {

// §11.2 step 1 (0x4718b0 on the block's first byte): type 13 counts 255, empty or water
// max(light, 5), anything else 0.
int standingLight(uint8_t blockType, uint8_t lightValue);

// §11.2 steps 1-3: (1, 1, 1, l / 255), x (0.5, 0.5, 1.5, 1) while the slow countdown
// (creature +0x134) is positive, then one tint per resident effect in list order:
// kinds 1 and 2 (1, 0.5, 0.5, 1), kind 4 (0.5, 1.5, 0.5, 1). The deferred translucent
// path tints kind 1 only. Step 4 (the frame material) is applied by the shader; step 5
// (the 0x200 highlight) needs a colour source that is not specified, so it is not modelled.
glm::vec4 material(int standingLight, const creatures::Combatant &c, bool deferred);

// §11.2 trail: the five ghosts are drawn when (class 4, specialization 1, the action clock
// below the action's total duration and action 0x11, 0x05 or 0x14), or action 0x30 or
// 0x5D, or a resident effect of kind 12 or 3.
bool hasMotionTrail(const creatures::Combatant &c);

// Translucency (+0x1190, the stealth value): a positive value defers the body.
inline bool translucent(const creatures::Combatant &c) { return c.stealth > 0.f; }

// One draw of a creature in frame order. Ghosts precede the body; `offset` scales the
// creature's velocity (+0x34) for the ghost's displacement.
struct Draw {
    enum Kind : uint8_t { Ghost, Body, DeferredBody } kind;
    float alpha;
    float offset;
};
struct Sequence {
    std::array<Draw, 6> draws{};
    uint32_t count = 0;
    const Draw *begin() const { return draws.data(); }
    const Draw *end() const { return draws.data() + count; }
};
// k = 0..4: alpha 0.5 (k / 5), offset -0.07 (1 - k / 5); then the body, opaque, or for a
// translucent creature deferred with alpha 1 - 0.75 s.
Sequence drawSequence(bool trail, float stealth);

} // namespace creature_shading
