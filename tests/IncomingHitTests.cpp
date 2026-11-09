#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
}

int main() {
    for (uint8_t kind : {uint8_t(0), uint8_t(3)}) {
        Combatant actor;
        actor.controller = 2;
        actor.characterClass = 0;
        actor.powerTier = 8;
        actor.health = 100;
        actor.stealth = 10;
        actor.action = 84;
        actor.actionMs = 77;
        actor.actionLockMs = 7;
        actor.hitFlash = .7f;
        actor.appearanceFlags = 0x2000;
        Body body;
        body.velocity = {2, 3, 4};
        body.push = {5, 6, 7};
        HitRecord hit;
        hit.amount = 10;
        hit.kind = kind;
        hit.critical = 0xff;
        hit.stun = 100;
        hit.lockMs = 500;
        hit.knockBack = {8, 9, 10};
        receiveHit(actor, body, hit);
        check(actor.health == 90 && actor.stealth == 7.5f && actor.hitStun == 100 &&
              actor.action == 84 && actor.actionMs == 77 && actor.actionLockMs == 7 && actor.hitFlash == .7f &&
              body.velocity == glm::vec3(2, 3, 4) && body.push == glm::vec3(5, 6, 7),
              "incoming zero/three share direct health and level-only meter arithmetic without world role, protection, cancellation or vector gates");
    }
    {
        const auto saved = itemBonuses().damage;
        for (float amount : {4.f, -4.f}) {
            Combatant actor;
            actor.health = 0;
            actor.mana = .1f;
            actor.charge = .2f;
            actor.blockGauge = -.5f;
            actor.hitFlash = .7f;
            actor.hitStun = 3;
            actor.actionLockMs = 7;
            actor.equipment[MAIN_HAND].type = 3;
            int calls = 0;
            itemBonuses().damage = [&](const Item &) { calls++; return 1.f; };
            Body body;
            body.push = {1, 0, 0};
            HitRecord hit;
            hit.kind = HIT_BLOCKED;
            hit.amount = amount;
            hit.stun = 100;
            hit.lockMs = 500;
            hit.knockBack = {2, 3, 4};
            receiveHit(actor, body, hit);
            check(calls == 1 && actor.health == 0 && actor.mana == -.4f && actor.charge == -.4f &&
                  actor.blockGauge == (amount > 0 ? -1.f : 1.5f) && actor.hitFlash == (amount > 0 ? 1.f : .7f) &&
                  actor.hitStun == 3 && actor.actionLockMs == 7 && body.velocity == hit.knockBack &&
                  body.push == glm::vec3(1, 0, 0),
                  "incoming block assigns velocity, queries its divisor once and updates resources in order even at zero health");
        }
        itemBonuses().damage = saved;
    }
    for (float amount : {-3.f, 0.f, 3.f}) {
        Combatant actor;
        actor.id = 72;
        actor.health = 0;
        actor.hitStun = 3;
        actor.actionLockMs = 7;
        actor.buffs = {{6, 2, 1000, 31}, {4, 1, 1000, 99}, {6, 0, 0, 41}};
        actor.buffs[0].typePadding = {3, 7, 11};
        actor.buffs[0].timePadding = {13, 17, 19, 23};
        Body body;
        body.velocity = {2, 3, 4};
        HitRecord hit;
        hit.attacker = 999;
        hit.target = 72;
        hit.kind = HIT_ABSORBED;
        hit.amount = amount;
        hit.stun = 100;
        hit.lockMs = 500;
        std::vector<BuffEvent> events;
        receiveHit(actor, body, hit, &events);
        check(actor.health == 0 && actor.buffs.size() == 3 && events.size() == 2 &&
              actor.buffs[0].value == (amount < 2 ? 2 - amount : 0) &&
              actor.buffs[0].timeMs == (amount < 2 ? 1000 : 0) &&
              actor.buffs[2].value == (amount < 0 ? -amount : 0) &&
              actor.buffs[2].timeMs == 0 && actor.buffs[1].value == 1 &&
              actor.hitStun == 3 && actor.actionLockMs == 7 && body.velocity == glm::vec3(2, 3, 4),
              "incoming shields process negative/zero/positive amounts and expired residents without a health gate or unlink");
        check(events[0].source == 72 && events[0].target == 72 && events[1].source == 72 &&
              events[1].target == 72 && events[0].record.source == 31 && events[1].record.source == 41 &&
              events[0].record.serialize() == actor.buffs[0].serialize(),
              "incoming shield events use controlled identity in both headers and preserve complete resident payloads");
    }
    {
        const auto saved = itemBonuses().health;
        Combatant actor;
        actor.controller = 1;
        actor.characterClass = 0;
        actor.multipliers[0] = 0;
        actor.equipment[MAIN_HAND].type = 3;
        actor.health = 9;
        Body body;
        HitRecord hit;
        hit.amount = -2;
        int calls = 0;
        itemBonuses().health = [&](const Item &) { return ++calls == 1 ? 10.f : 20.f; };
        receiveHit(actor, body, hit);
        check(actor.health == 20 && calls == 2,
              "incoming healing compares the post-subtraction value and stores a freshly queried second health limit");
        calls = 0;
        actor.health = 9;
        actor.actionMs = 77;
        hit.amount = 20;
        hit.stun = 100;
        receiveHit(actor, body, hit);
        check(actor.health == 0 && actor.hitStun == 100 && actor.actionMs == 77 && calls == 1,
              "incoming fatal amount merges timer, clamps health below zero and still queries the limit without resetting the action clock");
        hit.amount = -20;
        receiveHit(actor, body, hit);
        check(actor.health == 0 && calls == 1, "later incoming healing cannot resurrect a zero-health controlled creature");
        actor.health = std::numeric_limits<float>::quiet_NaN();
        actor.hitStun = 0;
        const auto bits = std::bit_cast<uint32_t>(actor.health);
        receiveHit(actor, body, hit);
        check(std::bit_cast<uint32_t>(actor.health) == bits && actor.hitStun == 0 && calls == 1,
              "unordered incoming health fails the ordered-positive gate before timer or limit work");
        itemBonuses().health = saved;
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
