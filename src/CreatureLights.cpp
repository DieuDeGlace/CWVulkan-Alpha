#include "CreatureLights.h"
#include "creatures/Combat.h"
#include "creatures/Pose.h"
#include "creatures/Spirits.h"
#include "creatures/Strike.h"
#include <bit>
#include <cmath>

namespace terrain_lights {
namespace {
std::optional<FixedPosition> yawPosition(const FixedPosition &base, float degrees, float height, bool point) {
    const float radians = degrees * std::bit_cast<float>(0x3c8efa35u);
    const float cosine = float(std::cos(double(radians)));
    const float sine = float(std::sin(double(radians)));
    glm::mat4 rotation(1);
    rotation[0] = {cosine, sine, 0, 0};
    rotation[1] = {-sine, cosine, 0, 0};
    const glm::vec3 local = point ? glm::vec3(.5f, 1, height * .3f) : glm::vec3(0, 1, 0);
    float reciprocal = 1;
    if (point) {
        const float xy = rotation[0][3] * local.x + rotation[1][3] * local.y;
        const float xyz = xy + rotation[2][3] * local.z;
        const float denominator = xyz + rotation[3][3];
        reciprocal = 1.f / denominator;
    }
    FixedPosition position = base;
    for (int axis = 0; axis < 3; ++axis) {
        const float x = rotation[0][axis] * local.x, y = rotation[1][axis] * local.y;
        const float xy = axis == 0 ? y + x : x + y;
        const float xyz = xy + rotation[2][axis] * local.z;
        const float offset = point ? reciprocal * (xyz + rotation[3][axis]) : xyz;
        const float fixed = offset * 65536.f;
        // Exceptional/out-of-range conversion has no ordinary native contract.
        if (!(double(fixed) >= -0x1p63 && double(fixed) < 0x1p63))
            return std::nullopt;
        position[axis] = std::bit_cast<int64_t>(uint64_t(base[axis]) + uint64_t(int64_t(fixed)));
    }
    return position;
}
bool admitsAction(const creatures::Combatant &creature, SpeedQuery query) {
    float numerator;
    switch (creature.action) {
    case 28: case 36: case 94: case 95:
        return true;
    case 37: case 43: case 89:
        numerator = creature.mainHand().subtype == 12 ? 600.f : 1200.f;
        break;
    case 38: case 44: numerator = 500; break;
    case 87: numerator = 5000; break;
    case 88: case 92: numerator = 400; break;
    default: return false;
    }
    const float speed = (query ? query : creatures::attackSpeed)(creature);
    const float denominator = speed * creature.multipliers[1];
    const float threshold = numerator / denominator;
    if (!(double(threshold) >= -0x1p31 && double(threshold) < 0x1p31))
        return false;
    return creature.actionMs < int32_t(threshold);
}
} // namespace

std::array<std::optional<Candidate>, 3> yawSources(
    const creatures::Combatant &creature, const FixedPosition &renderedPosition,
    float stateYawDegrees, float height) {
    std::array<std::optional<Candidate>, 3> sources{};
    if (!(creature.health > 0))
        return sources;
    if (creature.mainHand().subtype == 20 || creature.offHand().subtype == 20)
        if (const auto position = yawPosition(renderedPosition, stateYawDegrees, height, true))
            sources[0] = Candidate{*position, {1.2f, 1, .5f}, 32, 0};
    const auto &lantern = creature.equipment[10];
    if ((creature.mode & 0x0200) && lantern.type == 24)
        if (const auto position = yawPosition(renderedPosition, stateYawDegrees, height, true)) {
            const float radius = float(10 + 8 * int(lantern.rarity));
            const float quarter = float(lantern.rarity) * .25f;
            const float strength = quarter + .75f;
            sources[1] = Candidate{*position, glm::vec3(.6f, .5f, .4f) * strength, radius, 0};
        }
    if (creature.hasBuff(9))
        if (const auto position = yawPosition(renderedPosition, stateYawDegrees, height, true))
            sources[2] = Candidate{*position, {.6f, .4f, 0}, 16, 0};
    return sources;
}

std::optional<Candidate> actionSource(const creatures::Combatant &creature,
    const FixedPosition &renderedPosition, float renderedYawDegrees, SpeedQuery speed) {
    if (!(creature.health > 0) || !admitsAction(creature, speed))
        return std::nullopt;
    const auto position = yawPosition(renderedPosition, renderedYawDegrees, 0, false);
    if (!position)
        return std::nullopt;
    const float angle = float(creature.actionMs) * .01f;
    const float cosine = float(std::cos(double(angle)));
    const float variation = cosine * .1f;
    const float strength = variation + 1.f;
    return Candidate{*position, glm::vec3(.3f, .2f, .1f) * strength, 20, 0};
}

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

std::array<std::optional<FixedPosition>, 2> weaponPoints(const creatures::PoseState &pose, glm::dvec3 displayed) {
    // Trail histories 0 and 2 hold the main and off tips, x/y relative to the
    // displayed position and z relative to the refresh height.
    const auto at = [&](const glm::vec3 &p) {
        return fromBlocks({displayed.x + double(p.x), displayed.y + double(p.y), double(pose.trailHeight) + double(p.z)});
    };
    return {at(pose.trail[0][0]), at(pose.trail[2][0])};
}

std::array<std::optional<Candidate>, 4> equipmentSources(
    const creatures::Combatant &creature, const FixedPosition &mainPosition,
    const std::array<FixedPosition, 2> &handPositions) {
    std::array<std::optional<Candidate>, 4> sources{};
    if (!(creature.health > 0))
        return sources;
    // One binary32 division, no clamp, shared by all four blocks. Only an ordered
    // equality with zero (either sign) skips them; negative and NaN ratios proceed.
    const float ratio = float(creature.combo) / creatures::comboMax(creature);
    if (ratio == 0.f)
        return sources;
    struct Site {
        int slot;
        bool weapon; // type byte 3, else any nonzero type byte
        float factor;
    };
    static constexpr std::array<Site, 4> sites{{{creatures::MAIN_HAND, true, .3f},
                                                {creatures::OFF_HAND, true, .5f},
                                                {creatures::SHOULDERS_SLOT, false, .1f},
                                                {creatures::CHEST_SLOT, false, .1f}}};
    for (size_t index = 0; index < sites.size(); ++index) {
        const Site &site = sites[index];
        const creatures::Item &item = creature.equipment[site.slot];
        if (site.weapon ? item.type != 3 : item.type == 0)
            continue;
        const glm::vec4 average = creatures::averageSpiritColour(item);
        if (!(average.a > 0)) // a NaN alpha skips too
            continue;
        // (A.rgb x ratio) x factor: two binary32 products per channel, ratio first.
        const glm::vec3 scaled = glm::vec3(average) * ratio;
        const glm::vec3 colour = scaled * site.factor;
        // The ruling leaves downstream use of a non-finite colour (zero-divisor
        // ratios) unspecified; the engine omits such a source rather than upload it.
        if (!std::isfinite(colour.r) || !std::isfinite(colour.g) || !std::isfinite(colour.b))
            continue;
        sources[index] = Candidate{site.weapon ? handPositions[index] : mainPosition, colour, 10, 0};
    }
    return sources;
}

std::array<std::optional<glm::vec3>, 2> criticalFlashColours(const creatures::Combatant &attacker) {
    const creatures::Item *weapons[2];
    creatures::strikingWeapons(attacker, weapons[0], weapons[1]);
    std::array<std::optional<glm::vec3>, 2> colours{};
    for (size_t k = 0; k < colours.size(); ++k)
        if (weapons[k])
            if (const glm::vec4 average = creatures::averageSpiritColour(*weapons[k]); average.a > 0)
                colours[k] = glm::vec3(average);
    return colours;
}

} // namespace terrain_lights
