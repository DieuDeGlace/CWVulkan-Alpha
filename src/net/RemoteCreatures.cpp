#include "net/RemoteCreatures.h"
#include "SoundRules.h"
#include "creatures/Abilities.h"
#include "creatures/AngleStep.h"
#include "creatures/Npcs.h"
#include "creatures/Projectiles.h"
#include "creatures/Strike.h"
#include <algorithm>
#include <bit>
#include <utility>

namespace net {

namespace {

glm::vec3 rotationOf(const creatures::Body &body) {
    return {body.swimPitch, body.bank, body.yaw};
}

glm::dvec3 soundPosition(const int64_t *position) {
    const glm::vec3 blocks{float(position[0]), float(position[1]), float(position[2])};
    return glm::dvec3(blocks * (1.f / 65536.f));
}

void prepareReceivedAction(creatures::Combatant &fighter, const creatures::Body &body, int32_t dt,
                           const RemoteCreatures::StepEffects *effects) {
    if (effects)
        creatures::receivedActionEffects(fighter, body, dt, soundPosition(body.position), effects->draw,
                                         effects->frame.sounds, effects->frame.bursts);
    if (!creatures::meleeAction(fighter.action))
        return;

    const int32_t before = fighter.actionMs;
    // Action 31 tests the live clock after preparation against bounds saved
    // from the earlier signed repeat window.
    int32_t lower = 0, upper = 0;
    if (fighter.action == 31) {
        const auto timing = creatures::phases(fighter, fighter.action);
        const int32_t start = timing.windUp, period = timing.active / 12;
        lower = upper = start;
        if (period > 0 && before >= start && before < start + timing.active) {
            lower = start + ((before - start) / period) * period;
            upper = lower + 100;
        }
    }
    creatures::updateSwingCount(fighter, before, dt);
    creatures::prepareCommonAction(fighter, dt, effects ? &effects->frame.buffs : nullptr);
    if (fighter.action == 31 && lower < fighter.actionMs && fighter.actionMs <= upper &&
        fighter.actionMs >= creatures::phases(fighter, fighter.action).windUp) {
        if (const auto consumed = creatures::consumeBuff(fighter, 9); consumed && effects)
            effects->frame.buffs.push_back(*consumed);
    }
}

void appendMovementEffects(creatures::Body &body, const creatures::BlockType &block,
                          bool hasControlledActor, const RemoteCreatures::StepEffects *effects) {
    const bool thudded = std::exchange(body.thudded, false);
    const bool splashed = std::exchange(body.splashed, false);
    const bool footfall = std::exchange(body.footfall, false);
    if (!effects)
        return;
    auto &frame = effects->frame;
    if (thudded && hasControlledActor)
        frame.sounds.push_back({soundPosition(body.thudPosition.data()), 32,
                                sound::randomPitch(effects->draw(), 0.2f, 1.f), 0.5f});
    if (splashed) {
        const auto at = soundPosition(body.position);
        auto burst = creatures::splashBurst(glm::dvec3(0));
        std::copy_n(body.position, 3, burst.position);
        frame.bursts.push_back(burst);
        frame.sounds.push_back({at, 31, sound::randomPitch(effects->draw(), 1.f, 0.9f), 1.f});
    }
    if (footfall && hasControlledActor) {
        const auto below = creatures::footstepCell(body.position, body.size.z);
        const bool softGround = block(below.x, below.y, below.z) == 3;
        const int32_t draw = effects->draw();
        frame.sounds.push_back({soundPosition(body.position), softGround ? 33 + draw % 3 : 32,
                                softGround ? 1.f : sound::randomPitch(draw, 0.2f, 1.f), 1.f});
    }
}

const creatures::Combatant *fighterAt(uint64_t id, const creatures::ActorMap<RemoteFighter> &fighters,
                                    const RemoteCreatures::Controlled *controlled) {
    if (controlled && controlled->fighter.id == id)
        return &controlled->fighter;
    const auto found = fighters.find(id);
    return found == fighters.end() ? nullptr : &found->second.fighter;
}

} // namespace

void RemoteCreatures::receive(const CreatureReceipt &receipt) {
    auto &r = list[receipt.id];
    if (receipt.initialAim) {
        r = Remote{};
        r.drawnLook = *receipt.initialAim; // engine's starting drawn aim
    }
    r.smoothing = !receipt.snap;
    if (receipt.snap) {
        std::copy_n(receipt.snap->position.begin(), 3, r.live.follow);
        r.drawnRotation = receipt.snap->rotation;
    }
    r.needsLiveSnapshot = true;
}

void RemoteCreatures::updateLive(uint64_t id, const CreatureRecord &record, RemoteFighter &state) {
    const auto found = list.find(id);
    if (found == list.end() || !found->second.needsLiveSnapshot)
        return;
    auto &r = found->second;
    r.needsLiveSnapshot = false;
    // §1.6: every received field lands on the live state (the whole-record copy).
    state.receive(id, record);
    const auto &fighter = state.fighter;
    creatures::Body &b = r.live;
    for (int axis = 0; axis < 3; axis++)
        b.position[axis] = record.position(axis);
    b.key = id;
    b.size = record.appearance().box;
    b.velocity = record.velocity();
    b.intent = record.intent();
    b.push = record.response();
    b.controller = fighter.controller;
    b.type = fighter.type;
    b.appearanceFlags = fighter.appearanceFlags;
    b.swimmer = (b.appearanceFlags & 0x0100) != 0;
    b.flyer = (b.appearanceFlags & 0x0002) != 0;
    b.replica = true;
    b.skills = fighter.skills;
    b.skillFallback = fighter.level / 2 + fighter.powerTier + 1;
    b.gliderEquipped = fighter.equipment[11].type == 23 && fighter.equipment[11].subtype == 0;
    b.lanternEquipped = fighter.equipment[10].type == 24;
    b.look = record.look();
    const glm::vec3 rotation = record.orientation();
    b.swimPitch = rotation.x;
    b.bank = rotation.y;
    b.yaw = rotation.z;
    r.headPitch = record.headPitch();
}

void RemoteCreatures::writeLive(uint64_t id, CreatureRecord &record, const RemoteFighter &state) const {
    const auto *remote = find(id);
    if (!remote || remote->needsLiveSnapshot)
        return;
    const auto &body = remote->live;
    for (int axis = 0; axis < 3; axis++)
        record.setPosition(axis, body.position[axis]);
    record.setMotion(body.velocity, rotationOf(body), body.push, body.intent);
    record.setHeadPitch(remote->headPitch);
    record.setContactFlags(state.fighter.contact);
    record.setActionTimers(state.fighter);
    record.setHealth(state.fighter.health);
    record.setMovementModes(state.fighter);
    record.setCombatResources(state.fighter);
}

void RemoteCreatures::retain(const std::map<uint64_t, CreatureRecord> &listed) {
    std::erase_if(list, [&](const auto &entry) { return !listed.contains(entry.first); });
}

bool RemoteCreatures::attach(Remote &pet, const creatures::Combatant &fighter,
                             const creatures::ActorMap<RemoteFighter> &fighters, const Controlled *controlled) {
    if (fighter.controller != 5)
        return false;
    const auto *owner = fighterAt(fighter.ownerId, fighters, controlled);
    if (!owner || owner->action != 106)
        return false;
    if (controlled && owner == &controlled->fighter) {
        const auto &rider = controlled->body;
        creatures::copyRidingPose(pet.live, rider, rider.yaw);
        pet.drawnRotation = rotationOf(rider);
    } else {
        const auto *rider = find(fighter.ownerId);
        if (!rider)
            return false;
        creatures::copyRidingPose(pet.live, rider->live, rider->live.yaw);
        pet.drawnRotation = rider->drawnRotation;
    }
    pet.attached = true;
    return true;
}

void RemoteCreatures::step(creatures::ActorMap<RemoteFighter> &fighters, int32_t dtMs,
                           const creatures::BlockType &block, const creatures::ObjectTest &objects,
                           const Controlled *controlled, const std::function<void()> &advanceControlled,
                           const StepEffects *effects) {
    // §2.2: the fraction f approaches 1 at 0.015 once per millisecond of the step.
    float f = 0.f;
    for (int32_t ms = 0; ms < dtMs; ms++)
        f = (1.f - f) * 0.015f + f;
    const int64_t k = int64_t(f * 65536.f);
    const auto advance = [&](uint64_t id, RemoteFighter &state) {
        auto &fighter = state.fighter;
        const auto found = list.find(id);
        if (found == list.end())
            return;
        Remote &r = found->second;
        r.attached = false;
        if (!creatures::admitCreatureUpdate(fighter))
            return;
        // The common timers run inside each received actor's living turn.
        // Replica actors skip combo timeout and world-authoritative periodic hits.
        creatures::advanceActionTimers(fighter, dtMs);
        prepareReceivedAction(fighter, r.live, dtMs, effects);
        creatures::advanceActionClock(fighter, dtMs, false);
        auto &body = r.live;
        body.mode = fighter.mode;
        body.action = fighter.action;
        body.actionMs = fighter.actionMs;
        const auto timing = creatures::phases(fighter, fighter.action);
        body.windUpMs = timing.windUp;
        body.activeMs = timing.active;
        body.durationMs = timing.total();
        body.useLimitMs = fighter.action == 81 ? creatures::consumableDuration(fighter.held) : -1;
        body.contact = fighter.contact;
        body.dodgeMs = fighter.dodgeMs;
        body.stunMs = fighter.hitStun;
        body.slowed = fighter.slowMs > 0;
        body.hastened = fighter.hasteMs > 0;
        body.haste = fighter.hasBuff(12) ? 1.f + creatures::abilityValue(fighter, 100) : 1.f;
        if (dtMs >= 0) {
            float *riderEnergy = nullptr;
            if (fighter.controller == 5) { // riding actor step §3: a reverse link may resolve key zero
                if (controlled && controlled->fighter.id == fighter.ownerId)
                    riderEnergy = &controlled->body.ridingEnergy;
                else if (const auto owner = list.find(fighter.ownerId); owner != list.end())
                    riderEnergy = &owner->second.live.ridingEnergy;
            }
            creatures::step(body, dtMs, block, objects, riderEnergy, &fighter);
        }
        appendMovementEffects(body, block, controlled != nullptr, effects);
        creatures::faceReceivedActor(body.yaw, r.headPitch, r.drawnLook, body, fighter, dtMs);
        if (std::exchange(body.rolled, false) && effects && controlled)
            effects->frame.sounds.push_back({soundPosition(body.position), 26, 1.f, 1.f});
        if (!attach(r, fighter, fighters, controlled)) {
            const glm::vec3 rotation = rotationOf(body);
            if (!r.smoothing) {
                std::copy_n(body.position, 3, body.follow);
                r.drawnRotation = rotation;
            } else {
                // §2.2: all axes close (d × T(f)) / 65536, then Z closes a second time.
                for (int axis = 0; axis < 3; axis++) {
                    const uint64_t gap = uint64_t(body.position[axis]) - uint64_t(body.follow[axis]);
                    const int64_t delta = std::bit_cast<int64_t>(gap * uint64_t(k)) / 65536;
                    body.follow[axis] = std::bit_cast<int64_t>(uint64_t(body.follow[axis]) + uint64_t(delta));
                }
                const float gap = float(std::bit_cast<int64_t>(uint64_t(body.position[2]) - uint64_t(body.follow[2])));
                body.follow[2] = std::bit_cast<int64_t>(uint64_t(body.follow[2]) + uint64_t(int64_t(gap * f)));
                for (int axis = 0; axis < 3; axis++)
                    r.drawnRotation[axis] = creatures::angleStep(r.drawnRotation[axis], rotation[axis], f);
                body.bob = 0.f;
            }
        }
        creatures::refreshDrawnAim(r.drawnLook, body.look, fighter, fighter.actionMs, dtMs);
    };
    if (controlled) {
        creatures::visitReceivedActors(fighters, controlled->fighter.id, [&] {
            if (advanceControlled)
                advanceControlled();
            if (controlled->fighter.health > 0.f)
                std::copy_n(controlled->body.position, 3, controlled->body.follow);
        }, advance);
    } else {
        for (auto &[id, fighter] : fighters)
            advance(id, fighter);
    }
}

void RemoteCreatures::finishStep(creatures::ActorMap<RemoteFighter> &fighters, const Controlled *controlled) {
    const auto clearTarget = [&](creatures::Combatant &fighter) {
        if (const uint64_t id = fighter.relations.contact)
            if (const auto *target = fighterAt(id, fighters, controlled); target && target->health <= 0.f)
                fighter.relations.contact = 0;
    };
    const auto finish = [&](uint64_t id, RemoteFighter &state) {
        clearTarget(state.fighter);
        if (state.fighter.controller == 5)
            if (const auto actor = list.find(id); actor != list.end())
                attach(actor->second, state.fighter, fighters, controlled);
    };
    if (controlled) {
        creatures::visitReceivedActors(fighters, controlled->fighter.id, [&] {
            clearTarget(controlled->fighter);
        }, finish);
    } else {
        for (auto &[id, fighter] : fighters)
            finish(id, fighter);
    }
}

const RemoteCreatures::Remote *RemoteCreatures::find(uint64_t id) const {
    const auto it = list.find(id);
    return it == list.end() ? nullptr : &it->second;
}

} // namespace net
