// Bounded scheduling contracts; no reference-game instructions or assets.
#include "creatures/Projectiles.h"
#include "creatures/Npcs.h"
#include "creatures/RayActions.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <tuple>

using namespace creatures;
namespace {
int failures = 0;
void check(bool passed, const char *description) {
    if (!passed) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}

Combatant ranger(int action) {
    Combatant result;
    result.characterClass = 2;
    result.equipment[MAIN_HAND] = {3, BOW, 1, 0, 1, 0};
    result.action = action;
    result.mana = 1;
    return result;
}

struct Collision {
    Combatant source, target;
    Body sourceBody, targetBody;
    Projectiles world;
    ShotEffects effects;

    Collision() {
        source.id = 41;
        source.characterClass = 3;
        sourceBody.place({20, 0, 0});
        target.id = 72;
        target.controller = 1;
        Projectile shot;
        shot.shooter = source.id;
        shot.kind = SPELL;
        shot.element = 1;
        shot.damage = 10;
        world.list.push_back(shot);
    }
    void step(const std::function<int()> &draw, ProjectileContext context) {
        world.step(1, {{&source, &sourceBody}, {&target, &targetBody}},
            [&](uint64_t id) { return id == source.id ? &source : nullptr; },
            [](int32_t, int32_t, int32_t) { return uint8_t(0); }, draw, effects, context);
    }
};

struct RayCase {
    Combatant source;
    Body body;
    NpcFrame frame;
    Projectiles resident;
    std::vector<ShotTarget> targets;
    std::vector<int64_t> order;
    RayActionWorld world;
    int draws = 0;

    RayCase() {
        source.id = 41;
        source.controller = 1;
        source.characterClass = 3;
        source.specialization = 1;
        source.action = 94;
        source.actionMs = phases(source, source.action).windUp;
        source.mana = 1;
        source.aim = {1, 0, 0};
        body.place({0, 0, 2});
        world.sight.voxel = [](int32_t x, int32_t, int32_t) { return uint8_t(x >= 7 ? 1 : 0); };
        world.hit = [&](Combatant &target, const Body &, HitInput) {
            order.push_back(std::bit_cast<int64_t>(target.id));
            return true;
        };
        world.launch = [&](const Projectile &shot) {
            order.push_back(INT64_MIN);
            resident.list.push_back(shot);
            frame.shots.push_back(shot);
        };
    }
    void step(int32_t milliseconds = 1) {
        world.targets = targets;
        stepRayAction(source, body, milliseconds, world,
                      {frame.emittedHits, frame.buffs, frame.sounds, frame.bursts}, [&] { draws++; return 32767; });
    }
};

void rayLaunches() {
    {
        RayCase ray;
        ray.source.controller = 0;
        ray.source.actionMs = phases(ray.source, 94).windUp;
        Combatant target;
        target.id = 72;
        target.controller = 1;
        Body targetBody;
        targetBody.place({3, 0, 2});
        std::array<PlayerView, 2> players{{{ray.source, ray.body, 0, ray.source.actionMs, true, false, true},
                                         {target, targetBody, 0, 0, false}}};
        Npcs world;
        world.dispatchPlayerActions(players, 1, ray.world.sight.voxel, ray.frame);
        check(ray.frame.shots.size() == 2 && ray.source.struck == std::vector<uint64_t>{target.id} &&
                  !ray.frame.emittedHits.empty(),
              "the actual offline player dispatcher gives action 94 its endpoint, actor hit and secondary cloud");
        ray.frame.reset();
        ray.source.actionMs = phases(ray.source, 94).windUp;
        worldgen::WorldMap terrain;
        world.stepInto(1, terrain, {players.begin(), players.end()}, 43200000, 0,
            ray.world.sight.voxel, ray.frame, {}, {}, {}, {}, {},
            [&](int32_t piece, std::span<PlayerView> views) {
                check(ray.resident.list.size() == 2 && ray.frame.shots.size() == 2 &&
                          ray.resident.list[0].age == 0,
                      "the production world call has resident ray copies before its object and flight tail");
                world.stepProjectiles(ray.resident, piece, terrain, ray.world.sight.voxel, views, ray.frame);
            }, &ray.resident);
        check(ray.resident.list.size() == 2 && ray.resident.list[0].age == 1 && ray.frame.shots[0].age == 0,
              "bound flight steps each fresh resident once and retains the unchanged launch output");
        ray.source.action = 0;
        world.stepInto(1, terrain, {players.begin(), players.end()}, 43200000, 0,
            ray.world.sight.voxel, ray.frame, {}, {}, {}, {}, {},
            [&](int32_t piece, std::span<PlayerView> views) {
                world.stepProjectiles(ray.resident, piece, terrain, ray.world.sight.voxel, views, ray.frame, 2);
            }, &ray.resident);
        check(ray.resident.list.size() == 2 && ray.resident.list[0].age == 2 && ray.frame.shots.size() == 2,
              "a later bound world piece advances the residents without replaying their launch collector");
    }
    {
        RayCase ray;
        Combatant positive, negative, linked, dodging, dead;
        positive.id = 5;
        negative.id = uint64_t(-3);
        linked.id = 9;
        dodging.id = 10;
        dodging.dodgeMs = -1;
        dead.id = 11;
        dead.health = 0;
        ray.source.ordinaryLink = linked.id;
        ray.source.struck = {777};
        ray.source.deferred = {888};
        Body targetBody;
        targetBody.place({3, 0, 2});
        targetBody.size = {2, 2, 2};
        ray.targets = {{&positive, &targetBody}, {&ray.source, &ray.body}, {}, {&negative, &targetBody},
                       {&linked, &targetBody}, {&dodging, &targetBody}, {&dead, &targetBody}};
        ray.world.hit = [&](Combatant &target, const Body &, HitInput input) {
            ray.order.push_back(std::bit_cast<int64_t>(target.id));
            check(ray.source.struck.back() == target.id && input.selector == 0 && input.area == 0,
                  "ray bookkeeping and selector are visible before the hit resolver");
            check(ray.frame.shots.size() == (target.id == negative.id ? 1u : 2u),
                  "the endpoint and prior secondary copy precede the next signed actor's resolver");
            ray.source.multipliers[2] += 1.f;
            return target.id != negative.id;
        };
        ray.step();
        check(ray.order == std::vector<int64_t>{INT64_MIN, -3, INT64_MIN, 5, INT64_MIN} &&
                  ray.source.struck == std::vector<uint64_t>{negative.id, positive.id} && ray.source.deferred.empty() &&
                  ray.draws == 4 && ray.frame.sounds.empty(),
              "action 94 clears both sets, skips linked and locked actors and launches after false and true returns");
        check(ray.frame.shots.size() == 3 && ray.resident.list.size() == 3,
              "the initial crossing produces separate resident and outgoing copies at both launch sites");
        if (ray.frame.shots.size() == 3) {
            check(ray.frame.shots[0].position[0] == 7 * 65536 && ray.frame.shots[1].position[0] == 2 * 65536 &&
                      ray.frame.shots[2].position[0] == 2 * 65536,
                  "the endpoint precedes the strict-radius sample shared by multiple admitted actors");
            for (size_t index = 0; index < 3; index++) {
                Projectile expected;
                expected.shooter = ray.source.id;
                expected.position[0] = (index == 0 ? 7 : 2) * 65536;
                expected.position[2] = 2 * 65536;
                expected.damage = float(index + 1) * .1f;
                expected.radius = 4;
                expected.strength = 0;
                expected.kind = BURST;
                expected.element = 2;
                check(ray.frame.shots[index].serialize() == expected.serialize() &&
                          ray.resident.list[index].serialize() == expected.serialize(),
                      "each launch has the exact zero-strength record and freshly queried binary32 magic amount");
            }
            ray.resident.list[0].age = 123;
            check(ray.frame.shots[0].age == 0, "resident flight mutations preserve the independent output record");
        }
        ray.frame.reset();
        ray.source.actionMs = phases(ray.source, 94).windUp + 200;
        ray.step(201);
        check(ray.frame.shots.empty(), "action 94 has no later 200-ms launch or actor-hit sample");
    }
    for (const bool connected : {false, true})
        for (const uint8_t role : {uint8_t(0), uint8_t(1)})
            for (const bool controlled : {false, true}) {
                RayCase ray;
                ray.source.controller = role;
                ray.source.action = 28;
                ray.source.actionMs = 0;
                ray.world.connected = connected;
                ray.world.controlled = controlled ? &ray.source : nullptr;
                ray.step();
                const bool admitted = controlled || (!connected && role != 0);
                check(ray.frame.shots.size() == size_t(admitted) &&
                          (ray.source.mana < 1.f) == admitted && ray.draws == 0,
                      "native mode, source role and exact controlled pointer gate the ray before its mana prelude");
            }
    for (const int32_t action : {28, 94, 95}) {
        RayCase ray;
        ray.source.action = action;
        ray.source.actionMs = phases(ray.source, action).windUp - 1;
        ray.step();
        check(ray.frame.shots.empty(), "landing exactly on the signed ray threshold leaves its launch closed");
        ray.source.actionMs = phases(ray.source, action).windUp;
        ray.step();
        check(ray.frame.shots.size() == 1, "an old age equal to the threshold launches when the new age exceeds it");
    }
    {
        RayCase ray;
        ray.source.mana = 0;
        ray.source.buffs.push_back({9, 2.f, 500});
        ray.step();
        check(ray.frame.shots.size() == 1 && ray.source.mana == 0.f && ray.frame.buffs.size() == 1 &&
                  ray.frame.buffs[0].record.kind == 9 && ray.frame.buffs[0].record.timeMs == 0,
              "zero-cost action 94 still resets kind nine before its launch");
    }
    {
        RayCase ray;
        ray.source.action = 95;
        ray.source.actionMs = phases(ray.source, 95).windUp;
        ray.source.mana = .2f;
        ray.step();
        check(ray.source.mana == 0 && ray.frame.shots.empty() && ray.draws == 0,
              "negative post-cost mana clamps and exits before timing, launches and draws");
    }
    {
        RayCase ray;
        ray.source.actionMs = INT32_MAX;
        ray.step();
        check(ray.frame.shots.empty() && ray.draws == 0, "signed age wrap rejects the common ray window");
        ray.source.actionMs = 16777217;
        ray.step();
        check(ray.frame.shots.empty(), "a late integer age above binary32's exact range cannot become a fresh launch");
    }
    {
        RayCase ray;
        Combatant target;
        target.id = 5;
        Body body;
        body.place({0, 0, 2});
        ray.targets = {{&target, &body}};
        ray.world.hit = [&](Combatant &, const Body &, HitInput) {
            ray.source.actionMs += 1;
            return true;
        };
        ray.step();
        check(ray.frame.shots.size() == 1,
              "a hit resolver's age mutation closes the independently reevaluated secondary launch gate");
    }
    for (const bool connected : {false, true}) {
        RayCase ray;
        ray.source.specialization = 2;
        ray.source.actionMs = phases(ray.source, 94).windUp;
        ray.source.health = 20;
        ray.world.connected = connected;
        ray.world.controlled = &ray.source;
        Combatant target;
        target.id = 5;
        Body body;
        body.place({0, 0, 2});
        ray.targets = {{&target, &body}};
        ray.step();
        check(ray.frame.shots.empty() && ray.frame.emittedHits.size() == 1 &&
                  ray.frame.emittedHits[0].attacker == ray.source.id &&
                  ray.frame.emittedHits[0].target == ray.source.id && ray.frame.emittedHits[0].amount < 0 &&
                  (ray.source.health > 20) == !connected,
              "specialization two emits its drain after success and directly heals only in mode zero");
    }
    {
        RayCase ray;
        ray.source.action = 95;
        ray.source.combo = 40;
        const Phases timing = phases(ray.source, 95);
        const float before = attackSpeed(ray.source);
        Combatant cleared = ray.source;
        cleared.combo = 0;
        const float ratio = before / attackSpeed(cleared);
        bool found = false;
        for (int32_t clock = timing.windUp + 1; clock < timing.windUp + timing.active; clock++) {
            constexpr int32_t piece = 20;
            if ((clock + piece - timing.windUp) / 200 == (clock - timing.windUp) / 200)
                continue;
            const int32_t rescaled = int32_t(ratio * float(clock));
            const bool originalSound = (clock + piece) / 200 != clock / 200;
            const bool rescaledSound = (rescaled + piece) / 200 != rescaled / 200;
            if (originalSound == rescaledSound)
                continue;
            found = true;
            ray.source.actionMs = clock;
            ray.step(piece);
            check(ray.source.actionMs == rescaled && ray.frame.sounds.size() == size_t(rescaledSound) &&
                      ray.draws == int(rescaledSound) && ray.frame.shots.empty(),
                  "an empty repeated ray rescales the live age before its absolute sound-grid draw");
            break;
        }
        check(found, "the empty-ray sound fixture distinguishes original and rescaled signed clocks");
    }
    {
        RayCase ray;
        ray.body.position[0] = (int64_t(1) << 55) + 3;
        ray.body.position[1] = -(int64_t(1) << 55) - 5;
        ray.source.aim = {0, 0, 0};
        ray.step();
        check(ray.frame.shots.size() == 1 && ray.frame.shots[0].position[0] == ray.body.position[0] &&
                  ray.frame.shots[0].position[1] == ray.body.position[1],
              "a zero-reach endpoint still launches with every original fixed-position bit");
    }
}

void launchResidency() {
    for (const int32_t action : {22, 50, 43, 89}) {
        Combatant actor = ranger(action);
        actor.mana = 1;
        actor.charge = .2f;
        actor.spentCharge = .2f;
        if (action == 43) {
            actor.characterClass = 3;
            actor.buffs.push_back({9, 1, 1000});
        }
        const Phases timing = phases(actor, action);
        const int32_t piece = action == 50 ? timing.active / 4 : 1;
        Body body;
        std::vector<Projectile> shots, residents;
        ShotEffects events;
        int draws = 0;
        launch(actor, body, {1, 0, 0}, float(timing.windUp), float(piece), [&] { draws++; return 32767; },
            shots, events, {}, [&](const Projectile &projectile) {
                check(shots.empty(), "resident insertion precedes the travelling or stationary outgoing copy");
                if (action == 43)
                    check(actor.charge == .2f && events.buffs.empty() && events.sounds.empty() && draws == 1,
                          "magic residents precede positive-charge spending, proc reset and firing sound");
                else if (action == 50)
                    check(events.sounds.size() == 1 && draws == 2,
                          "volley pitch and damage draws precede resident insertion in their native order");
                else if (action == 22)
                    check(events.sounds.empty() && draws == 1,
                          "single-shot damage precedes residency and its pitch draw follows the output copy");
                residents.push_back(projectile);
            });
        check(shots.size() == 1 && residents.size() == 1 && shots[0].serialize() == residents[0].serialize(),
              "each projectile creator publishes one value-identical resident and output pair");
        if (action == 43)
            check(actor.charge == 0 && events.buffs.size() == 1 && events.buffs[0].record.timeMs == 0 &&
                      events.sounds.size() == 1,
                  "magic output follows its launch-specific charge and buff work");
    }
}

void trajectoryRay() {
    struct Case {
        double origin;
        int32_t occupiedX;
        uint8_t type;
        bool survives, pointContact;
        const char *description;
    };
    for (const auto &test : std::array{
             Case{10.875, 10, 1, false, false, "backward terrain contact rolls back without a point effect"},
             Case{10.625, 11, 1, true, false, "a solid voxel ahead of the advanced point does not stop the shot"},
             Case{10.0, 9, 1, true, false, "a terrain face at exact flight distance does not stop the shot"},
             Case{10.875, 11, 1, false, true, "an occupied advanced cell emits the separate point effect"},
             Case{10.875, 11, 0x82, true, false, "water passes both the point and trajectory reads"}}) {
        Projectile shot;
        shot.position[0] = int64_t(test.origin * 65536.);
        shot.position[2] = 65536;
        shot.velocity = {2, 0, .9375f}; // gravity for 125 ms leaves exactly (2, 0, 0)
        Projectiles world;
        world.list.push_back(shot);
        ShotEffects out;
        int draws = 0;
        world.step(125, {}, {}, [&](int32_t x, int32_t, int32_t) { return x == test.occupiedX ? test.type : uint8_t(0); },
                   [&] { draws++; return 0; }, out, {true});
        check((!world.list.empty()) == test.survives && out.bursts.size() == size_t(test.pointContact) &&
                  draws == 0, test.description);
        if (!world.list.empty())
            check(world.list[0].position[0] == shot.position[0] + 16384,
                  "the unobstructed arrow advances a quarter block in fixed units");
        if (!out.bursts.empty())
            check(out.bursts[0].position[0] == shot.position[0],
                  "the point effect uses the exact rolled-back position");
    }
    for (const uint8_t state : {uint8_t(1), uint8_t(0)}) {
        Projectile shot;
        shot.position[0] = 12 * 65536;
        shot.position[1] = 2 * 65536;
        shot.position[2] = 65536;
        shot.velocity = {10, 0, .9375f};
        Projectiles world;
        world.list.push_back(shot);
        ShotEffects out;
        const visibility::Record object{3, {11 * 65536, 2 * 65536, 0}, {1, 3, 3}, 0, state};
        const visibility::RecordLookup records = [&](int32_t x, int32_t y, const visibility::RecordVisitor &visit) {
            if (x == 0 && y == 0)
                visit(object);
        };
        int draws = 0;
        world.step(125, {}, {}, [](int32_t, int32_t, int32_t) { return uint8_t(0); },
                   [&] { draws++; return 0; }, out, {true, nullptr, &records});
        check(world.list.empty() == (state != 0) && out.bursts.empty() && draws == 0,
              "the first of three substeps tests the object side within full piece distance");
    }
}

void pointImpactSample() {
    auto zone = std::make_shared<worldgen::Zone>(0, 0);
    auto &restored = zone->column(10, 2);
    restored.bottom = -2;
    restored.blocks = {{17, 127, 241, 0x82}, {9, 8, 7, worldgen::DRY_AIR}};
    auto &advanced = zone->column(11, 2);
    advanced.bottom = -2;
    advanced.blocks = {{200, 40, 20, worldgen::SOLID}, {10, 11, 12, worldgen::DRY_AIR}};
    worldgen::WorldMap terrain;
    terrain.add(zone);
    Projectile shot;
    shot.position[0] = 10 * 65536 + 57344;
    shot.position[1] = 2 * 65536;
    shot.position[2] = -65536;
    shot.velocity = {2, 0, .9375f};
    Projectiles world;
    world.list.push_back(shot);
    ShotEffects effects;
    int draws = 0;
    world.step(125, {}, {}, [&](int32_t x, int32_t y, int32_t z) { return terrain.read(x, y, z).type(); },
               [&] { draws++; return 0; }, effects, {.terrain = &terrain});
    check(world.list.empty() && effects.bursts.size() == 1 && draws == 0,
          "an exact negative Z plane reads the lower solid cell before the backward ray");
    if (effects.bursts.empty())
        return;
    const auto &effect = effects.bursts.front();
    check(std::equal(std::begin(effect.position), std::end(effect.position), std::begin(shot.position)) &&
              effect.colour == glm::vec4(17.f / 255.f, 127.f / 255.f, 241.f / 255.f, 1.f) &&
              effect.velocity == glm::vec3(0, 0, 10) && effect.size == .1f &&
              effect.count == 4 && effect.style == LIT && effect.spread == 3.f,
          "the burst copies the restored fixed point and its water cell RGB with the native effect fields");
}

void dedicatedFlight() {
    worldgen::WorldMap terrain;
    terrain.add(std::make_shared<worldgen::Zone>(0, 0));
    const BlockType block = [&](int32_t x, int32_t y, int32_t z) { return terrain.read(x, y, z).type(); };
    for (const uint8_t kind : {uint8_t(HIT_DAMAGE), uint8_t(HIT_BLOCKED), uint8_t(HIT_ABSORBED)}) {
        Combatant source, first, last;
        source.id = 7;
        source.controller = 1;
        first.id = uint64_t(-3);
        last.id = 3;
        first.controller = last.controller = 0;
        first.health = last.health = 20.f;
        if (kind == HIT_BLOCKED) {
            first.action = 8;
            first.mana = .125f;
            first.blockGauge = .5f;
        } else if (kind == HIT_ABSORBED) {
            first.buffs.push_back({ABSORB, 1000.f, 3000, 12});
        }
        Body sourceBody, firstBody, lastBody;
        sourceBody.place({10, 10, 20});
        firstBody.place({20, 10, 20});
        lastBody.place({20, 10, 20});
        std::array<PlayerView, 3> players{{{last, lastBody}, {source, sourceBody}, {first, firstBody}}};
        Projectile arrow;
        arrow.shooter = source.id;
        std::copy_n(firstBody.position, 3, arrow.position);
        arrow.damage = 19.f;
        arrow.slow = 418;
        NpcFrame frame;
        frame.shots.push_back(arrow);
        Projectiles flight;
        worldgen::Random random(1);
        Npcs world(random);
        world.stepProjectiles(flight, 1, terrain, block, players, frame);
        check(frame.emittedHits.size() == 2 && frame.playerHits.size() == 1 &&
                  frame.emittedHits[0].target == first.id && frame.emittedHits[0].kind == kind &&
                  frame.emittedHits[0].serialize() == frame.emittedHits[1].serialize() &&
                  flight.list.empty() && frame.shots.front().serialize() == arrow.serialize(),
              "the first signed player target receives two identical guard, ward or ordinary records in order");
        check(first.health == 20.f && last.health == 20.f && first.slowMs == 418 && last.slowMs == 0,
              "the role-zero relay keeps player health while the contact caller updates the admitted target clock");
        if (kind == HIT_BLOCKED)
            check(first.mana == .625f && first.blockGauge < .5f && frame.sounds.size() == 1 && frame.sounds[0].sound == 24,
                  "guard's producer updates the live player state and sound before its delivery-only apply");
        if (kind == HIT_ABSORBED)
            check(first.buffs.front().value == 1000.f && frame.sounds.size() == 1 && frame.sounds[0].sound == 91,
                  "ward emits twice for a player without locally consuming the resident ward");
        if (kind != HIT_DAMAGE) {
            worldgen::Random expected(1);
            for (int i = 0; i < 5; i++)
                expected.draw();
            check(random.draw() == expected.draw(),
                  "duplicate guard and ward delivery preserves gain, critical, proc and pitch draw counts");
        }
    }
    Combatant source;
    source.id = 9;
    source.controller = 1;
    Body sourceBody;
    sourceBody.place({10, 10, 20});
    std::array<PlayerView, 1> players{{{source, sourceBody}}};
    Npcs world;
    Npc target;
    target.fighter.id = 20;
    target.fighter.controller = 3;
    target.fighter.health = 12.f;
    target.body.place({20, 10, 20});
    world.npcs().push_back(target);
    Projectile arrow;
    arrow.shooter = source.id;
    std::copy_n(target.body.position, 3, arrow.position);
    arrow.damage = 500.f;
    NpcFrame frame;
    frame.shots = {arrow, arrow};
    Projectiles flight;
    world.stepProjectiles(flight, 1, terrain, block, players, frame);
    check(world.npcs().front().fighter.health == 0 && world.npcs().front().fatalPresented &&
              frame.emittedHits.size() == 1 && frame.playerHits.empty() && flight.list.size() == 1 && frame.shots.size() == 2,
          "the first shot kills the live actor before the second visit while both launch copies survive");
    check(frame.sounds.size() >= 2 && frame.sounds.front().sound != 27 && frame.sounds.back().sound == 27,
          "projectile impact feedback precedes the death sound produced by immediate application");
}

void offlineFlight() {
    worldgen::WorldMap terrain;
    terrain.add(std::make_shared<worldgen::Zone>(0, 0));
    const BlockType block = [&](int32_t x, int32_t y, int32_t z) { return terrain.read(x, y, z).type(); };
    Combatant local, source;
    local.id = 3;
    local.health = 20.f;
    source.id = 7;
    source.controller = 1;
    Body localBody, sourceBody;
    localBody.place({20, 10, 20});
    sourceBody.place({10, 10, 20});
    std::array<PlayerView, 2> players{{{local, localBody, 0, 0, false, false, true}, {source, sourceBody}}};
    worldgen::Random random(1);
    Npcs world(random);
    Npc first;
    first.fighter.id = uint64_t(-3);
    first.fighter.controller = 3;
    first.fighter.health = 12.f;
    first.body.place(localBody.blocks());
    world.npcs().push_back(first);

    Projectile oldLaunch;
    oldLaunch.kind = BURST;
    oldLaunch.position[0] = 40 * 65536;
    Projectile resident = oldLaunch;
    resident.age = 20;
    Projectile arrow;
    arrow.shooter = source.id;
    std::copy_n(localBody.position, 3, arrow.position);
    arrow.damage = 500.f;
    NpcFrame frame;
    frame.shots = {oldLaunch, arrow, arrow};
    Projectiles flight;
    flight.list.push_back(resident);
    bool sawEarlierDeath = false;
    world.onControlledHit = [&](HitRecord &record) {
        sawEarlierDeath = record.target == local.id && world.npcs().front().fighter.health == 0;
    };

    world.stepProjectiles(flight, 20, terrain, block, players, frame, 1);
    check(sawEarlierDeath && local.health == 0 && world.npcs().front().fatalPresented &&
              frame.emittedHits.size() == 2 && frame.emittedHits[0].target == first.fighter.id &&
              frame.emittedHits[1].target == local.id && frame.playerHits.empty(),
          "offline signed actor order kills the NPC before the next arrow applies to the controlled target");
    check(frame.seen.size() == 2 && frame.seen[0].first.target == first.fighter.id &&
              frame.seen[1].first.target == local.id && frame.seen[1].second == localBody.blocks(),
          "each standalone projectile output reaches combat presentation once");
    check(frame.shots.size() == 3 && frame.shots[0].serialize() == oldLaunch.serialize() &&
              frame.shots[1].serialize() == arrow.serialize() && frame.shots[2].serialize() == arrow.serialize() &&
              flight.list.size() == 1 && flight.list[0].age == 40,
          "same-piece retirements preserve independent launch copies and do not replay earlier launches");
    world.stepProjectiles(flight, 1, terrain, block, players, frame, frame.shots.size());
    check(flight.list.size() == 1 && flight.list[0].age == 41 && frame.emittedHits.size() == 2,
          "the next piece advances only the surviving resident and emits no duplicate hit");

    worldgen::Random healRandom(4567);
    Npcs healingWorld(healRandom);
    local.health = 20;
    auto &burst = flight.list.front();
    burst.shooter = local.id;
    burst.element = 2;
    burst.damage = 10.f;
    burst.age = 999;
    std::copy_n(localBody.position, 3, burst.position);
    healingWorld.stepProjectiles(flight, 1, terrain, block, std::span{players}.first(1), frame, frame.shots.size());
    worldgen::Random expected(4567);
    expected.draw();
    expected.draw();
    check(local.health == 30.f && frame.emittedHits.size() == 3 && frame.emittedHits.back().amount == -10.f &&
              flight.list.front().age == 1000 && healRandom.draw() == expected.draw(),
          "the owner's first water pulse applies in place and spends critical then controlled proc draws");
}
}

int main() {
    rayLaunches();
    launchResidency();
    trajectoryRay();
    pointImpactSample();
    dedicatedFlight();
    offlineFlight();
    {
        Collision c;
        c.source.action = 17;
        // Spell hits are magic: their presentation makes no draw (attack-routine M5).
        const int values[] = {32767, 0, 32767, 0};
        int draws = 0;
        c.step([&] { return draws < 4 ? values[draws++] : (++draws, 32767); }, {false, &c.source});
        check(c.source.hasBuff(9) && c.effects.buffs.size() == 1,
              "impact proc is resident and queued with the generic hit");
        check(draws == 4 && c.effects.hits.size() == 2 && c.effects.buffs.size() == 1 &&
              c.effects.buffs[0].record.kind == 9,
              "contact and impact each draw critical then their own producer roll before generic resolution");
        check(c.source.buffs.size() == 1 && c.source.buffs[0].kind == 9,
              "a wrong-class contact still rolls, and a current action 17 does not replace projectile selector zero");
    }
    for (uint8_t subtype : {uint8_t(0), uint8_t(1), uint8_t(2), uint8_t(BRACELET),
                            uint8_t(BOOMERANG), uint8_t(STAFF), uint8_t(DAGGER)}) {
        Collision c;
        c.source.equipment[MAIN_HAND] = {3, subtype};
        const bool half = subtype <= 2 || subtype == BRACELET;
        const int procDraw = half || subtype == DAGGER ? 6000 : 4000;
        int draws = 0;
        c.step([&] { return draws++ == 3 ? procDraw : 32767; }, {false, &c.source});
        check(draws == 4 && c.source.hasBuff(9) == (subtype == DAGGER),
              "producer probability applies its weapon-pair half or three-tenths scale before the draw");
    }
    {
        Collision c;
        c.source.equipment[MAIN_HAND] = {4, STAFF};
        int draws = 0;
        c.step([&] { return draws++ == 3 ? 4000 : 32767; }, {false, &c.source});
        check(c.source.hasBuff(9), "nonweapon raw staff subtype does not scale the proc probability");
    }
    {
        Collision c;
        c.source.action = 91;
        int draws = 0;
        c.step([&] { return draws++ == 3 ? 0 : 32767; }, {false, &c.source});
        check(draws == 4 && c.effects.hits.empty() && c.source.hasBuff(9) && c.effects.buffs.size() == 1,
              "impact producer survives a later generic ground-target rejection");
    }
    {
        Collision c;
        c.world.list[0].element = 2;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, &c.source});
        check(draws == 4 && c.effects.hits.size() == 1 && c.effects.hits[0].record.amount > 0,
              "hostile water contact skips its separate event while the later impact still enters generic resolution");
    }
    for (uint8_t role : {uint8_t(0), uint8_t(6)}) {
        Collision c;
        c.world.list[0].kind = BURST;
        c.world.list[0].element = 2;
        c.world.list[0].age = 999;
        c.target.controller = role;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, &c.source});
        check(draws == 2 && c.effects.hits.size() == (role == 0 ? 1u : 0u) &&
              (role != 0 || c.effects.hits[0].record.amount == -10.f),
              "water burst's separate event requires a false relationship result and then excludes role six");
    }
    {
        Collision c;
        auto &shot = c.world.list[0];
        shot.element = 2;
        shot.special = 2;
        shot.strength = .7f;
        shot.velocity = {1, 0, 0};
        c.step([] { return 32767; }, {false, &c.source});
        check(c.world.list.size() == 1 && shot.kind == BURST && shot.age == 0 && shot.damage == .5f &&
              shot.radius == 8.5f && shot.strength == 0 && shot.special == 0 && shot.velocity.x == 1,
              "water conversion uses the old flag and strength and retains its existing velocity");
    }
    {
        Projectiles world;
        Projectile shot;
        shot.kind = BURST;
        shot.age = INT32_MAX;
        world.list.push_back(shot);
        ShotEffects effects;
        world.step(1, {}, {}, [](int32_t, int32_t, int32_t) { return uint8_t(0); }, [] { return 0; }, effects);
        check(world.list.size() == 1 && world.list[0].age == INT32_MIN,
              "projectile clock addition retains the native signed 32-bit wrap before its lifetime comparison");
    }
    {
        Collision c;
        c.world.list[0].kind = ARROW;
        c.source.mana = .2f;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {true, nullptr});
        check(draws == 4 && c.source.mana == .2f && c.effects.buffs.empty(),
              "nonlocal network resource-gain draws occur before suppression of the resource write");
    }
    {
        Collision c;
        c.source.characterClass = 2;
        c.source.specialization = 1;
        c.world.list[0].kind = ARROW;
        int draws = 0;
        c.step([&] { return ++draws == 4 ? 0 : 32767; }, {false, &c.source});
        check(draws == 5 && c.source.hasBuff(10) && c.effects.buffs.size() == 1 &&
              c.effects.buffs[0].source == 41 && c.effects.buffs[0].target == 41 &&
              c.effects.buffs[0].record.source == 0 && c.effects.buffs[0].record.timeMs == 30000,
              "owned ranger contact appends its fresh self buff after gain and critical draws");
    }
    {
        Collision c;
        c.source.mana = .2f;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, nullptr});
        check(draws == 1 && c.effects.hits.size() == 1 && c.effects.buffs.empty(),
              "mode-zero nonlocal role-zero source reaches contact but leaves the later impact scan before its draws");
    }
    {
        Collision c;
        c.source.controller = 1;
        c.source.characterClass = 2;
        c.source.specialization = 1;
        c.target.controller = 0;
        c.world.list[0].kind = ARROW;
        c.world.list[0].slow = 403;
        c.target.slowMs = 81;
        const float health = c.target.health;
        int draws = 0;
        c.step([&] { return ++draws == 4 ? 0 : 32767; }, {true, &c.target});
        check(draws == 3 && c.effects.hits.empty() && c.effects.buffs.empty() && c.target.slowMs == 403 &&
                  c.target.health == health,
              "the controlled target's clock rises after caller draws and generic connected rejection");
    }
    {
        Collision c;
        c.world.list[0].kind = ARROW;
        c.world.list[0].slow = 601;
        c.target.action = 8;
        c.target.blockGauge = .5f;
        c.target.mana = .1f;
        c.target.charge = .2f;
        const float health = c.target.health;
        const float guard = std::max(-1.f, .5f - c.world.list[0].damage / damage(c.target));
        int draws = 0;
        c.step([&] {
            if (++draws == 4)
                check(c.target.blockGauge == guard && c.target.slowMs == 0,
                      "guard mutation precedes its pitch draw and the caller's clock write");
            return 32767;
        }, {true});
        check(draws == 4 && c.target.health == health && c.target.mana == .6f &&
                  c.target.charge == .6f && c.target.slowMs == 601 && c.effects.hits.size() == 1 &&
                  c.effects.hits[0].record.kind == HIT_BLOCKED && !c.effects.hits[0].applyLocally,
              "connected guard changes the live target without applying the emitted record");
    }
    {
        Collision c;
        c.world.list[0].kind = ARROW;
        c.world.list[0].slow = 1300;
        c.source.ordinaryLink = c.target.id;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {true});
        check(draws == 0 && c.effects.hits.empty() && c.target.slowMs == 0 && c.world.list.size() == 1,
              "the full ordinary link excludes the target before any contact draw");
    }
    {
        Collision c;
        c.world.list[0].kind = ARROW;
        c.world.list[0].special = 7;
        c.world.list[0].slow = 250;
        c.target.slowMs = 89;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {true});
        check(draws == 2 && c.effects.hits.size() == 1 && c.effects.hits[0].record.area == 7 &&
                  c.target.slowMs == 89,
              "charged impact carries the entire charge byte and omits the direct target-clock write");
    }
    {
        Collision c;
        c.world.list[0].kind = BURST;
        c.world.list[0].element = 2;
        c.world.list[0].age = 999;
        c.world.list[0].slow = 10;
        c.sourceBody.place({0, 0, 0});
        c.targetBody.place({20, 0, 0});
        int applied = 0;
        c.step([] { return 32767; }, {true, &c.source, nullptr, [&](const HitRecord &hit) {
            ++applied;
            check(hit.attacker == c.source.id && hit.target == c.source.id && hit.amount == -10.f &&
                      c.source.slowMs == 0 && c.effects.hits.size() == 1,
                  "a beneficial controlled self hit applies immediately after its append");
        }});
        check(applied == 1 && c.effects.hits.size() == 1 && !c.effects.hits[0].applyLocally &&
                  c.source.slowMs == 10,
              "the collected self hit has already been applied before its target-clock write");
    }
    {
        Collision c;
        c.targetBody.place({4, 0, 1});
        auto &shot = c.world.list[0];
        shot.kind = ARROW;
        shot.position[2] = 65536;
        shot.radius = 5;
        const visibility::Record obstacle{3, {2 * 65536, 0, 0}, {1, 3, 3}, 0, 1};
        const visibility::RecordLookup records = [&](int32_t x, int32_t y, const visibility::RecordVisitor &visit) {
            if (x == 0 && y == 0)
                visit(obstacle);
        };
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {true, nullptr, &records});
        check(draws == 0 && c.effects.hits.empty() && c.world.list.size() == 1,
              "a live opaque side face rejects direct contact before any draw");
    }
    for (bool impact : {false, true}) {
        Collision c;
        c.world.list[0].special = impact ? 1 : 0;
        c.world.list[0].kind = impact ? SPELL : ARROW;
        c.world.list[0].damage = -10;
        c.world.list[0].element = 0;
        c.target.dodgeMs = impact ? 1.f : 0.f;
        int draws = 0;
        c.world.step(1, {{&c.target, &c.targetBody}}, {},
            [=](int32_t, int32_t, int32_t) { return uint8_t(impact ? 1 : 0); },
            [&] { draws++; return 32767; }, c.effects);
        check(!c.effects.hits.empty() && c.effects.hits[0].record.amount == (impact ? -10.f : 0.f) &&
              draws == (impact ? 2 : 1),
              "null source admits ordinary contacts and locked impact candidates; only contact clamps negative input");
    }
    {
        auto actor = ranger(24);
        ShotEffects effects;
        chargeSound(actor, 16777399, 1, {}, effects);
        check(effects.sounds.size() == 1 && effects.sounds[0].sound == 55,
              "held-action sound retains one-millisecond bucket crossings beyond binary32 integer precision");
    }
    for (uint8_t weapon : {uint8_t(STAFF), uint8_t(WAND), uint8_t(BRACELET)})
        for (uint8_t specialization : {uint8_t(0), uint8_t(1)}) {
            Combatant actor;
            actor.characterClass = 3;
            actor.specialization = specialization;
            actor.equipment[MAIN_HAND] = {3, weapon};
            actor.actionMs = 1000;
            actor.buffs.push_back({9, 1, 1000});
            check(startSecondaryAttack(actor), "powered selected secondary attack starts after idle recovery");
            const bool skip = actor.action == 31 || actor.action == 33 || actor.action == 37 || actor.action == 46;
            check(actor.actionMs == (skip ? (float)phases(actor, actor.action).windUp : 0.f),
                  "buff nine skips staff and selected projectile anticipation, excluding 45 and 43");
            const float before = actor.actionMs;
            check(!startSecondaryAttack(actor) && actor.actionMs == before,
                  "busy powered secondary start leaves its current clock unchanged");
        }
    {
        Combatant actor;
        actor.actionMs = 1000;
        actor.dodgeMs = -1;
        actor.mana = 1;
        check(!startSecondaryAttack(actor) && actor.action == 0, "a negative nonzero roll timer rejects an ordinary secondary start");
        actor.dodgeMs = 600;
        actor.characterClass = 4;
        actor.specialization = 1;
        actor.equipment[MAIN_HAND] = {3, LONGSWORD};
        actor.criticalLatched = true;
        check(startSecondaryAttack(actor) && actor.action == 5 && actor.dodgeMs == 600 && actor.criticalLatched,
              "assassin selection preserves the roll exception without mutating the later hit latch");
        actor = {};
        actor.actionMs = 1000;
        actor.mana = .099f;
        check(!startSecondaryAttack(actor), "secondary helper rejects mana below its native requirement");
        actor.mana = .1f;
        check(startSecondaryAttack(actor), "secondary helper accepts mana equality");
    }
    for (int action : {22, 23, 26, 50, 55, 96, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 89, 90, 108,
                       24, 25, 27, 36, 59, 63, 64}) {
        auto actor = ranger(action);
        std::vector<SoundEvent> sounds;
        int draws = 0;
        actionSounds(actor, 0, 2000, {}, [&] { draws++; return 0; }, sounds);
        check(sounds.empty() && draws == 0, "ranged dispatch has no extra melee lift/swing layer or pitch draws");
    }
    {
        auto actor = ranger(89);
        const float anticipation = (float)phases(actor, 89).windUp;
        Body body;
        ShotEffects effects;
        std::vector<Projectile> shots;
        int draws = 0;
        launch(actor, body, {}, anticipation - 50, 300, [&] { draws++; return 0; }, shots, effects);
        check(shots.size() == 1 && effects.sounds.size() == 2 && effects.sounds[0].sound == 36 &&
              effects.sounds[1].sound == 38 && draws == 2,
              "area projectile can channel in its last wind-up interval and release with zero direction");
    }
    {
        auto actor = ranger(22);
        actor.actionMs = -1;
        stepAction(actor, 32);
        check(actor.actionMs == -1, "negative active action clock does not advance through the general path");
        actor.action = 0;
        stepAction(actor, 32);
        check(actor.actionMs == 31, "idle action permits a negative clock to advance");
        actor.controller = 6;
        actor.actionMs = 0;
        stepAction(actor, 32);
        check(actor.actionMs == 0, "control discriminator six suppresses the general clock increment");
        actor.controller = 1;
        actor.actionMs = 100000;
        stepAction(actor, 32);
        check(actor.actionMs == 100032, "completed action clock keeps advancing without an invented duration cap");
    }
    for (int remainingStun : {-1, 0, 1}) {
        auto actor = ranger(24);
        actor.charge = .2f;
        actor.actionMs = 200;
        actor.hitStun = remainingStun;
        ShotEffects effects;
        stepCharge(actor, 190, 10, {}, effects);
        check(remainingStun > 0 ? actor.charge == 0 : actor.charge > .2f,
              "late charge gate clears only a still-positive stun");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>();
        zone->zx = zone->zy = 0;
        worldgen::Spawn spawn;
        spawn.type = 44;
        spawn.controller = 1;
        spawn.x = spawn.y = 10 * 65536;
        spawn.z = 3 * 65536;
        zone->spawns.push_back(spawn);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant player;
        player.id = 1;
        Body body;
        body.place({15, 10, 3});
        const BlockType floor = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 0 ? 1 : 0); };
        Npcs npcs;
        npcs.step(0, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
        check(npcs.all().size() == 1, "synthetic creature is activated through the actual world facade");
        if (!npcs.all().empty()) {
            npcs.npcs()[0].thinkMs = 32;
            npcs.npcs()[0].fighter.hitStun = 32;
            npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(!npcs.all().empty() && npcs.all()[0].thinkMs == 64,
                  "stun expiring exactly this update still suppresses early behavior and retains accumulated time");
            auto &npc = npcs.npcs()[0];
            npc.fighter.characterClass = 2;
            npc.fighter.equipment[MAIN_HAND] = {3, BOW};
            npc.fighter.action = 22;
            npc.fighter.actionMs = (float)phases(npc.fighter, 22).windUp;
            npc.fighter.hitStun = 100;
            npc.fighter.mana = 1;
            const auto emitted = npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(emitted.shots.size() == 1 && npc.fighter.action == 0 && npc.fighter.actionMs == 32 &&
                  npc.fighter.hitStun == 68, "real NPC emits its pending shot before the surviving stun interrupts its action");
            npc.fighter.action = 22;
            npc.fighter.actionMs = (float)phases(npc.fighter, 22).windUp;
            npc.fighter.hitStun = 32;
            npc.fighter.actionLockMs = 33;
            const auto locked = npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(locked.shots.empty() && npc.fighter.actionLockMs == 1,
                  "real NPC's surviving action lock suppresses a would-be anticipation crossing");
            npc.fighter.characterClass = 3;
            npc.fighter.specialization = 1;
            npc.fighter.action = 94;
            npc.fighter.actionMs = phases(npc.fighter, 94).windUp;
            npc.fighter.actionLockMs = 0;
            npc.fighter.hitStun = 100;
            npc.fighter.mana = 1;
            npc.body.look = {1, 0, 0};
            const auto ray = npcs.step(1, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(!ray.shots.empty() && ray.shots[0].kind == BURST && ray.shots[0].strength == 0,
                  "the actual dedicated NPC turn dispatches action 94 through the shared ray family");
        }
    }
    {
        auto actor = ranger(25);
        const float threshold = (float)phases(actor, 25).windUp;
        actor.actionMs = threshold;
        ShotEffects effects;
        stepCharge(actor, threshold - 1, 1, {}, effects);
        check(actor.charge > 0, "charging begins when the advanced clock reaches anticipation");
    }
    {
        auto actor = ranger(25);
        actor.charge = .2f;
        actor.actionMs = 210;
        ShotEffects effects;
        stepCharge(actor, 190, 20, {}, effects);
        check(effects.sounds.size() == 1 && std::abs(effects.sounds[0].pitch - 1.1f) < 1e-6f,
              "charge sound pitch reads charge before the gain");
        check(actor.charge > .2f, "the later charge gain still occurs");
        actor.actionMs = 199;
        effects.sounds.clear();
        stepCharge(actor, -1, 200, {}, effects);
        check(effects.sounds.empty(), "signed sound quotients truncate toward zero");
    }
    {
        auto actor = ranger(8);
        actor.actionMs = 210;
        ShotEffects effects;
        stepCharge(actor, 190, 20, {}, effects);
        check(effects.sounds.empty(), "action eight charges without entering the ranged sound branch");
    }
    {
        auto actor = ranger(22);
        Body body;
        const float threshold = (float)phases(actor, 22).windUp;
        std::vector<Projectile> shots;
        ShotEffects effects;
        int draws = 0;
        auto draw = [&] { draws++; return 0; };
        launch(actor, body, {1, 0, 0}, threshold - 1, 1, draw, shots, effects);
        check(shots.empty() && draws == 0, "landing exactly at a single-shot threshold does not emit");
        launch(actor, body, {1, 0, 0}, threshold, 1, draw, shots, effects);
        check(shots.size() == 1 && draws == 2, "a single shot includes equality only on the old side");
    }
    {
        auto actor = ranger(26);
        actor.charge = .5f;
        Body body;
        const float threshold = (float)phases(actor, 26).windUp;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, threshold - 1, 400, [] { return 0; }, shots, effects);
        check(shots.size() == 1, "a large boomerang update emits once even across both thresholds");
    }
    {
        auto actor = ranger(55);
        actor.charge = .5f;
        Body body;
        const Phases timing = phases(actor, 55);
        const float period = (float)(timing.active / 4);
        std::vector<Projectile> shots;
        ShotEffects effects;
        int draws = 0;
        const auto draw = [&] { return draws++ == 0 ? 0 : 32767; };
        launch(actor, body, {1, 0, 0}, 0, period, draw, shots, effects);
        check(shots.size() == 1 && draws == 2, "a volley emits exactly once at its pulse boundary");
        check(effects.sounds.size() == 1 && effects.sounds[0].pitch == 1,
              "the volley pitch uses the first random draw");
        check(!shots.empty() && std::abs(shots[0].damage - 2.278125f) < 1e-5f,
              "volley damage uses the second draw's spread");
        shots.clear();
        launch(actor, body, {1, 0, 0}, period, (float)timing.active * 2, draw, shots, effects);
        check(shots.size() == 1, "a large volley update does not catch up every skipped pulse");
    }
    {
        auto actor = ranger(45);
        actor.characterClass = 3;
        Body body;
        const Phases timing = phases(actor, 45);
        const float first = (float)timing.windUp, period = (float)(timing.active / 3);
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, first, period, [] { return 0; }, shots, effects);
        check(shots.empty(), "a repeated spell landing on the selected boundary defers emission");
        check(std::abs(actor.mana - .7f) < 1e-6f, "anticipation magic cost is paid even when the selected candidate defers emission");
        launch(actor, body, {1, 0, 0}, first + period, 1, [] { return 0; }, shots, effects);
        check(shots.size() == 1, "the following update can emit from equality at that boundary");
        check(std::abs(actor.mana - .7f) < 1e-6f, "later repeated emission does not repeat the anticipation cost");
    }
    // Re-certified projectiles contract (2026-11-14).
    {
        // §2.2: W sums the action's weapon set; a bow in hand counts alone (no off-hand weapon).
        auto actor = ranger(22);
        actor.equipment[OFF_HAND] = {3, SHIELD, 1, 0, 1, 0};
        Body body;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, (float)phases(actor, 22).windUp, 16, [] { return 0; }, shots, effects);
        check(shots.size() == 1 && std::abs(shots[0].damage - 11.25f) < 1e-4f,
              "a bow counts alone in the weapon set: the off-hand shield adds nothing");
    }
    for (int action : {5, 28, 94, 95}) {
        Combatant healer;
        healer.characterClass = 3, healer.specialization = 1;
        healer.action = action;
        Body body;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(healer, body, {1, 0, 0}, (float)phases(healer, action).windUp, 16, [] { return 0; }, shots, effects,
               [](int32_t x, int32_t, int32_t) { return uint8_t(x >= 7 ? 1 : 0); });
        check(shots.empty(), "generic projectile launch leaves stationary clouds with the shared ray dispatcher");
    }
    // Ranged-audio re-certification: buff-9 removal only for 37/43/45/46, not 108; powered
    // physical launches (released charge > 0 or action 23) remove buff 10.
    for (int action : {37, 43, 108, 38}) {
        Combatant mage;
        mage.characterClass = 3;
        mage.action = action;
        mage.mana = 1;
        mage.buffs.push_back({9, 0, 30000});
        Body body;
        std::vector<Projectile> shots;
        ShotEffects effects;
        const float P1 = (float)phases(mage, action).windUp;
        launch(mage, body, {1, 0, 0}, P1, 16, [] { return 0; }, shots, effects);
        const bool removes = action == 37 || action == 43;
        // Consumption replaces the record with a cleared one (time zero).
        check(shots.size() == 1 && (mage.buffs[0].timeMs == 0) == removes,
              "magic firing removes buff 9 only for actions 37, 43, 45 and 46");
        check(!effects.sounds.empty() && effects.sounds.back().volume == std::bit_cast<float>(0x3f666666u),
              "the magic firing sound follows the projectile at volume 0x3f666666");
    }
    for (const auto [action, charge, powered] : std::array<std::tuple<int, float, bool>, 3>{{{22, 0.f, false}, {22, .5f, true}, {23, 0.f, true}}}) {
        auto actor = ranger(action);
        actor.charge = charge;
        actor.mana = 1;
        actor.buffs.push_back({10, 0, 30000});
        Body body;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, (float)phases(actor, action).windUp, 16, [] { return 0; }, shots, effects);
        check(shots.size() == 1 && (actor.buffs[0].timeMs == 0) == powered && actor.charge == 0 && actor.spentCharge == charge,
              "the charge transfer precedes the launch test and powered launches remove buff 10");
        check(!effects.sounds.empty() && effects.sounds.back().sound == (powered ? 22 : 15),
              "unpowered action 22 plays sound 15, powered launches and action 23 play sound 22");
    }
    {
        // §2.5: the element applies to the thrown rock too.
        Combatant mage;
        mage.characterClass = 3, mage.specialization = 1;
        mage.action = 108;
        mage.mana = 1;
        Body body;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(mage, body, {1, 0, 0}, (float)phases(mage, 108).windUp, 16, [] { return 0; }, shots, effects);
        check(shots.size() == 1 && shots[0].kind == ROCK && shots[0].element == 2, "a water mage's rock is water");
    }
    {
        // §5.1 step 4: a dodging target joins the shooter's evaded-by set; no hit.
        Collision c;
        c.world.list[0].kind = ARROW;
        c.target.dodgeMs = 1;
        c.step([] { return 0; }, {false, &c.source});
        check(c.effects.hits.empty() && c.source.deferred.size() == 1 && c.source.deferred[0] == c.target.id,
              "a dodge is recorded in the shooter's +0x11B4 set");
    }
    {
        // §5: a still projectile pushes (0, 0, 3c).
        Collision c;
        c.world.list[0].strength = 2;
        c.step([] { return 32767; }, {false, &c.source});
        check(!c.effects.hits.empty() && c.effects.hits[0].record.knockBack.x == 0 &&
              c.effects.hits[0].record.knockBack.z == 6, "a still spell's knock-back is (0, 0, 3c)");
    }
    {
        // §6.4: the burst cube has a half-size of 5 blocks (10 wide).
        Collision c;
        auto &shot = c.world.list[0];
        shot.special = 1;
        shot.velocity = {50, 0, 0};
        shot.position[0] = 32768;
        c.targetBody.place({0.5, 4, 0});
        c.world.step(16, {{&c.source, &c.sourceBody}, {&c.target, &c.targetBody}},
            [&](uint64_t id) { return id == c.source.id ? &c.source : nullptr; },
            [](int32_t x, int32_t, int32_t) { return uint8_t(x == 1 ? 1 : 0); }, [] { return 32767; }, c.effects,
            {false, &c.source});
        check(c.world.list.empty() && c.effects.hits.size() == 1, "a burst 4 blocks away reaches the target");
    }
    {
        // §4 step 1: the boomerang catch scales by F = trunc(f x 65536) twice in fixed point.
        Combatant thrower;
        thrower.id = 5;
        Body home;
        home.place({10, 0, 0});
        Projectiles world;
        Projectile shot;
        shot.shooter = thrower.id;
        shot.kind = BOOMERANG_SHOT;
        shot.age = 1133;
        world.list.push_back(shot);
        ShotEffects effects;
        world.step(1, {{&thrower, &home}}, {}, [](int32_t, int32_t, int32_t) { return uint8_t(0); },
                   [] { return 0; }, effects);
        const int64_t gap = home.position[0];
        const int64_t F = (int64_t)(0.33f * 65536.f);
        check(!world.list.empty() && world.list[0].position[0] == (gap * F / 65536) * F / 65536,
              "the catch moves by the 16.16 fixed-point product");
    }
    for (const int32_t age : {800, 801, 1100, 1200, 1201}) {
        Combatant thrower;
        thrower.id = 0; // a live zero-key source is resolved even at zero health
        thrower.health = 0;
        thrower.aim = {2, 0, 0};
        Body home;
        home.place({110, 0, 0});
        Projectile shot;
        shot.kind = BOOMERANG_SHOT;
        shot.shooter = thrower.id;
        shot.age = age;
        shot.position[0] = 100 * 65536;
        Projectiles world;
        world.list.push_back(shot);
        ShotEffects effects;
        const int32_t elapsed = age == 800 || age == 1200 ? 1 : 0;
        int draws = 0;
        world.step(elapsed, {{&thrower, &home}}, {}, [](int32_t, int32_t, int32_t) { return uint8_t(0); },
                   [&] { draws++; return 0; }, effects, {true});
        if (age == 1201) {
            check(world.list.empty() && draws == 0, "boomerang checks signed expiry before any steering or draw");
            continue;
        }
        check(world.list.size() == 1 && world.list[0].age == age + elapsed && draws == 0,
              "boomerang uses the pre-increment age and a live zero-key source through the catch");
        if (world.list.empty())
            continue;
        const auto &resident = world.list[0];
        if (age == 800)
            check(resident.velocity.x == 3.f && resident.position[0] == shot.position[0] + 196,
                  "outbound steering samples the full aim offset before per-millisecond blending");
        else if (age <= 1100)
            check(resident.velocity.x == 100.f && resident.position[0] == shot.position[0],
                  "return steering assigns ten times the full source separation even for zero elapsed");
        else
            check(resident.velocity.x == 0.f && resident.position[0] == home.position[0],
                  "age 1200 catches at the source and remains resident until another world piece");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
