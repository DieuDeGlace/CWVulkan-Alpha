#include "net/RemoteCreatures.h"
#include "creatures/AngleStep.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <bit>

namespace net {

namespace {

glm::vec3 rotationOf(const creatures::Body &body) {
    return {body.swimPitch, body.bank, body.yaw};
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

void RemoteCreatures::updateLive(uint64_t id, const CreatureRecord &record, const creatures::Combatant &fighter) {
    const auto found = list.find(id);
    if (found == list.end() || !found->second.needsLiveSnapshot)
        return;
    auto &r = found->second;
    r.needsLiveSnapshot = false;
    // §1.6: every received field lands on the live state (the whole-record copy).
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
    b.mode = fighter.mode;
    b.dodgeMs = int32_t(fighter.dodgeMs);
    b.look = record.look();
    const glm::vec3 rotation = record.orientation();
    b.swimPitch = rotation.x;
    b.bank = rotation.y;
    b.yaw = rotation.z;
    r.headPitch = record.headPitch();
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
                           const Controlled *controlled, const std::function<void()> &advanceControlled) {
    // §2.2: the fraction f approaches 1 at 0.015 once per millisecond of the step.
    float f = 0.f;
    for (int32_t ms = 0; ms < dtMs; ms++)
        f = (1.f - f) * 0.015f + f;
    const int64_t k = int64_t(f * 65536.f);
    const auto advance = [&](uint64_t id, RemoteFighter &state) {
        auto &fighter = state.fighter;
        if (fighter.health > 0.f)
            creatures::tickBuffs(fighter, dtMs, false);
        const auto found = list.find(id);
        if (found == list.end())
            return;
        Remote &r = found->second;
        r.attached = false;
        if (!(fighter.health > 0.f))
            return;
        auto &body = r.live;
        body.stunMs = fighter.hitStun;
        if (dtMs > 0) {
            float *riderEnergy = nullptr;
            if (fighter.controller == 5) { // riding actor step §3: a reverse link may resolve key zero
                if (controlled && controlled->fighter.id == fighter.ownerId)
                    riderEnergy = &controlled->body.ridingEnergy;
                else if (const auto owner = list.find(fighter.ownerId); owner != list.end())
                    riderEnergy = &owner->second.live.ridingEnergy;
            }
            creatures::step(body, dtMs, block, objects, riderEnergy);
        }
        const int32_t clock = state.recordClock(); // the existing received-clock adapter
        creatures::faceAfterStep(body.yaw, r.headPitch, r.drawnLook, body, fighter, clock, dtMs);
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
        creatures::refreshDrawnAim(r.drawnLook, body.look, fighter, clock, dtMs);
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
