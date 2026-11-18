#include "creatures/RayActions.h"
#include "creatures/Npcs.h"
#include "creatures/Strike.h"
#include "SoundRules.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace creatures {
namespace {

int32_t addAge(int32_t age, int32_t piece) {
    return std::bit_cast<int32_t>(uint32_t(age) + uint32_t(piece));
}

int32_t relativeAge(int32_t age, int32_t threshold) {
    return std::bit_cast<int32_t>(uint32_t(age) - uint32_t(threshold));
}

glm::vec3 normalized(glm::vec3 value) {
    const float squared = (value.x * value.x + value.y * value.y) + value.z * value.z;
    const float inverse = 1.f / float(std::sqrt(double(squared)));
    return {value.x * inverse, value.y * inverse, value.z * inverse};
}

glm::dvec3 blocks(const std::array<int64_t, 3> &raw) {
    return glm::dvec3(double(raw[0]), double(raw[1]), double(raw[2])) / 65536.0;
}

} // namespace

void stepRayAction(Combatant &source, const Body &body, int32_t milliseconds,
                   const RayActionWorld &world, RayActionEvents events,
                   const std::function<int()> &draw) {
    if (!isRayAction(source.action))
        return;
    if ((world.connected || source.controller == 0) && &source != world.controlled)
        return;
    const auto threshold = [&] { return phases(source, source.action).windUp; };
    const auto crosses = [&] {
        const int32_t oldBound = threshold();
        if (source.actionMs > oldBound)
            return false;
        const int32_t newBound = threshold();
        return addAge(source.actionMs, milliseconds) > newBound;
    };
    if (source.action == 28) {
        source.mana = source.mana - float(milliseconds) * std::bit_cast<float>(0x3951B717u);
    } else if (crosses()) {
        source.mana = source.mana - manaCost(source, source.action);
        if (auto consumed = consumeBuff(source, 9))
            events.buffs.push_back(*consumed);
    }
    if (source.mana < 0.f) {
        source.mana = 0.f;
        return;
    }
    const int32_t clock = source.actionMs;
    const int32_t after = addAge(clock, milliseconds);
    if (after < threshold() || clock >= addAge(threshold(), phases(source, source.action).active))
        return;
    const bool repeat = source.action != 94 && after > threshold() &&
        relativeAge(after, threshold()) / 200 != relativeAge(clock, threshold()) / 200;
    if (!crosses() && !repeat)
        return;

    const glm::vec3 aim = source.aim;
    const float squared = (aim.x * aim.x + aim.y * aim.y) + aim.z * aim.z;
    const glm::vec3 direction = squared > 0.f ? normalized(aim) : aim;
    const auto origin = attackOrigin(body);
    // Zero aim remains an engine-defined zero-length ray.
    glm::vec3 unit(0);
    float reach = 0.f;
    if (squared > 0.f) {
        unit = normalized(aim);
        reach = visibility::trace(origin, {unit.x, unit.y, unit.z}, 200.f, 0, 1, world.sight);
    }
    const auto endpoint = [&](std::array<int64_t, 3> point) {
        for (size_t axis = 0; axis < point.size(); axis++) {
            const float scaled = unit[int(axis)] * reach;
            const int64_t displacement = int64_t(scaled * 65536.f);
            point[axis] = std::bit_cast<int64_t>(uint64_t(point[axis]) + uint64_t(displacement));
        }
        return point;
    };
    const auto launch = [&](const std::array<int64_t, 3> &center) {
        if (source.characterClass == 3 && source.specialization == 1 && crosses())
            world.launch(rayCloud(source, center));
    };
    source.struck.clear();
    source.deferred.clear();
    launch(endpoint(origin));

    bool admitted = false;
    if (reach > 0.f) {
        std::vector<ShotTarget> targets(world.targets.begin(), world.targets.end());
        std::erase_if(targets, [](const ShotTarget &target) { return !target.fighter || !target.body; });
        std::stable_sort(targets.begin(), targets.end(), [](const ShotTarget &a, const ShotTarget &b) {
            return std::bit_cast<int64_t>(a.fighter->id) < std::bit_cast<int64_t>(b.fighter->id);
        });
        const std::array<int64_t, 3> advance{int64_t(direction.x * 65536.f), int64_t(direction.y * 65536.f),
                                             int64_t(direction.z * 65536.f)};
        std::array<int64_t, 3> point = origin;
        for (int32_t sample = 0; reach > float(sample); sample++) {
            for (const ShotTarget &candidate : targets) {
                Combatant &target = *candidate.fighter;
                const Body &targetBody = *candidate.body;
                if (&target == &source || target.id == source.ordinaryLink || !(target.health > 0.f) ||
                    std::find(source.struck.begin(), source.struck.end(), target.id) != source.struck.end() ||
                    target.dodgeMs != 0)
                    continue;
                const float radius = targetBody.size.x * .5f + 1.f;
                int64_t horizontal = 0;
                for (size_t axis = 0; axis < 2; axis++) {
                    const uint64_t delta = uint64_t(point[axis]) - uint64_t(targetBody.position[axis]);
                    horizontal = std::bit_cast<int64_t>(uint64_t(horizontal) +
                        uint64_t(std::bit_cast<int64_t>(delta * delta) / 65536));
                }
                if (!(horizontal < int64_t((radius * radius) * 65536.f)))
                    continue;
                const float dz = float(std::bit_cast<int64_t>(uint64_t(point[2]) - uint64_t(targetBody.position[2]))) *
                    (1.f / 65536.f);
                if (!(std::fabs(double(dz)) < double(targetBody.size.z * .5f + 1.f)))
                    continue;
                source.struck.push_back(target.id);
                const bool critical = float(draw()) / 32767.f < criticalChance(source);
                HitInput input = beamInput(source, critical, draw);
                if (!hostilityTest(source, target))
                    continue;
                input.targetOnGround = (targetBody.contact & ON_GROUND) != 0;
                input.targetPosition = targetBody.blocks();
                input.attackerPosition = body.blocks();
                std::copy_n(targetBody.position, 3, input.targetRecordPosition.begin());
                std::copy_n(body.position, 3, input.attackerRecordPosition.begin());
                const bool accepted = world.hit(target, targetBody, input);
                if (accepted && source.specialization == 2) {
                    HitRecord drain;
                    drain.attacker = drain.target = source.id;
                    std::copy_n(body.position, 3, drain.position.begin());
                    drain.critical = input.critical;
                    drain.amount = std::bit_cast<float>(std::bit_cast<uint32_t>(input.amount) ^ 0x80000000u);
                    events.hits.push_back(drain);
                    if (!world.connected) {
                        source.health = source.health - drain.amount;
                        if (source.health > maxHealth(source))
                            source.health = maxHealth(source);
                    }
                }
                launch(point);
                admitted = true;
            }
            for (size_t axis = 0; axis < point.size(); axis++)
                point[axis] = std::bit_cast<int64_t>(uint64_t(point[axis]) + uint64_t(advance[axis]));
        }
    }
    if (source.struck.empty()) {
        events.bursts.push_back(Burst::at(blocks(endpoint({body.position[0], body.position[1], body.position[2]})),
                                         -direction, {1, 0.2f, 0.5f, 1}, .1f, 5, FIRE));
        const float before = attackSpeed(source);
        source.combo = 0;
        const float afterReset = attackSpeed(source);
        const float scaled = (before / afterReset) * float(source.actionMs);
        source.actionMs = scaled >= -2147483648.f && scaled < 2147483648.f ? int32_t(scaled) : INT32_MIN;
    } else if (admitted) {
        source.combo = addAge(source.combo, 1);
        source.comboElapsedMs = 0;
    }
    if (source.action == 94) {
        if (crosses())
            events.sounds.push_back({body.blocks(), 40, sound::randomPitch(draw(), .1f, 1.f), 1});
    } else {
        const int32_t soundAge = source.actionMs;
        const int32_t newBucket = addAge(soundAge, milliseconds) / 200;
        const int32_t oldBucket = soundAge / 200;
        if (newBucket != oldBucket)
            events.sounds.push_back({blocks(origin), admitted ? 37 : 36, sound::randomPitch(draw(), .25f, 1.f), 1});
    }
}

} // namespace creatures
