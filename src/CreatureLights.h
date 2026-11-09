#pragma once
// Bounded creature source sites from the held light lifecycle contract. Callers
// supply displayed and weapon positions; their native producers and same-frame
// freshness remain separate dependencies.
#include "TerrainLights.h"

namespace creatures { struct Combatant; }

namespace terrain_lights {

// First three independent sites, each constructing its own yaw offset from the
// current state angle and supplied appearance height. No buff consumption.
std::array<std::optional<Candidate>, 3> yawSources(
    const creatures::Combatant &creature, const FixedPosition &renderedPosition,
    float stateYawDegrees, float height);

using SpeedQuery = float (*)(const creatures::Combatant &);
// Site ten uses a separate rendered angle. Conditional actions query speed once;
// unconditional/unsupported actions do not. Null selects the engine stat helper.
std::optional<Candidate> actionSource(const creatures::Combatant &creature,
    const FixedPosition &renderedPosition, float renderedYawDegrees, SpeedQuery speed = nullptr);

// The mutually exclusive kind-specific source precedes the appearance-flag
// source. Neither site depends on the equipment counter or a view-plane test.
std::array<std::optional<Candidate>, 2> bodySources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const FixedPosition &renderedPosition);

// Slots in the result retain main hand, off hand, shoulders, chest order. Empty
// optionals are rejected sites, not zero-colour replacements or merged sources.
std::array<std::optional<Candidate>, 4> equipmentSources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const std::array<FixedPosition, 2> &handPositions);

} // namespace terrain_lights
