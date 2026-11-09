#include "CreatureLights.h"
#include "creatures/Combat.h"
#include "creatures/Spirits.h"
#include <cmath>

namespace terrain_lights {

std::array<std::optional<Candidate>, 4> equipmentSources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const std::array<FixedPosition, 2> &handPositions) {
    std::array<std::optional<Candidate>, 4> sources{};
    if (!(creature.health > 0))
        return sources;
    const float ratio = float(creature.combo) / creatures::comboMax(creature);
    // The held ordinary contract admits finite negative ratios and has no cap.
    // Exceptional ratios have no native contract; the engine omits those sources.
    if (ratio == 0 || !std::isfinite(ratio))
        return sources;
    constexpr int slots[] = {creatures::MAIN_HAND, creatures::OFF_HAND,
                             creatures::SHOULDERS_SLOT, creatures::CHEST_SLOT};
    for (size_t site = 0; site < sources.size(); ++site) {
        const creatures::Item &item = creature.equipment[slots[site]];
        const bool weapon = site < 2;
        if (weapon ? item.type != 3 : item.type == 0)
            continue;
        glm::vec3 average;
        if (!creatures::averageSpiritColour(item, average))
            continue;
        const glm::vec3 scaled = average * ratio;
        sources[site] = Candidate{weapon ? handPositions[site] : mainPosition,
                                  scaled * (weapon ? .3f : .1f), 10, 0};
    }
    return sources;
}

} // namespace terrain_lights
