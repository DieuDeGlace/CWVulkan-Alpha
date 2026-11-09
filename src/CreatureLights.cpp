#include "CreatureLights.h"
#include "creatures/Combat.h"
#include "creatures/Spirits.h"
#include <bit>
#include <cmath>

namespace terrain_lights {

std::array<std::optional<Candidate>, 2> bodySources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const FixedPosition &renderedPosition) {
    std::array<std::optional<Candidate>, 2> sources{};
    if (!(creature.health > 0))
        return sources;
    if (creature.type == 117)
        sources[0] = Candidate{renderedPosition, {0, .4f, .8f}, 16, 0};
    else if (creature.type >= 135 && creature.type <= 138) {
        constexpr std::array<glm::vec3, 4> colours{{{.1f, .5f, .1f}, {.1f, .1f, .5f},
                                                   {.5f, .1f, .2f}, {.8f, .8f, .6f}}};
        FixedPosition position = mainPosition;
        position[2] = std::bit_cast<int64_t>(uint64_t(position[2]) + 98304u); // 1.5 blocks
        sources[0] = Candidate{position, colours[size_t(creature.type - 135)], 16, 0};
    }
    if (creature.appearanceFlags & 0x0200) {
        const glm::vec3 colour = creature.controller == 1 ? glm::vec3(.4f, 0, .25f) : glm::vec3(.5f);
        sources[1] = Candidate{renderedPosition, colour, 16, 0};
    }
    return sources;
}

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
