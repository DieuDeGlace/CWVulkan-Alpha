#pragma once
// The four equipment-dependent source sites from the held light lifecycle
// contract. Callers supply the two weapon attachments; their native producers
// and same-frame freshness remain separate dependencies.
#include "TerrainLights.h"

namespace creatures { struct Combatant; }

namespace terrain_lights {

// Slots in the result retain main hand, off hand, shoulders, chest order. Empty
// optionals are rejected sites, not zero-colour replacements or merged sources.
std::array<std::optional<Candidate>, 4> equipmentSources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const std::array<FixedPosition, 2> &handPositions);

} // namespace terrain_lights
