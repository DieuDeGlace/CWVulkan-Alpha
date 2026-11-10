// Checks the creature code against the worked examples and test vectors of
// docs/specs/alpha-creatures-models.md and alpha-physics-camera.md.
#include "ItemNames.h"
#include "Loot.h"
#include "Quests.h"
#include "NewCharacter.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "creatures/Strike.h"
#include "creatures/Abilities.h"
#include "creatures/Appearance.h"
#include "creatures/Combat.h"
#include "creatures/Crafting.h"
#include "creatures/Skills.h"
#include "creatures/Spirits.h"
#include "creatures/FollowCamera.h"
#include "creatures/Inventory.h"
#include "creatures/Names.h"
#include "creatures/Npcs.h"
#include "creatures/Particles.h"
#include "creatures/Paths.h"
#include "creatures/Physics.h"
#include "creatures/Pose.h"
#include "creatures/Projectiles.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>

using namespace creatures;

static int failures = 0;
static void same(double got, double want, const std::string &what, double tolerance = 0) {
    const bool ok = tolerance == 0 ? got == want : std::abs(got - want) <= tolerance;
    if (!ok) {
        failures++;
        std::printf("FAIL %s: got %.9g, want %.9g\n", what.c_str(), got, want);
    }
}

int main() {
    // Models §2: the table's first and duplicated slots.
    same(std::string(MODEL_KEYS[0]) == "body2", 1, "slot 0 is body2");
    same(std::string(MODEL_KEYS[1300]) == "orc-head", 1, "slot 1300 ends as orc-head");
    same(MODEL_KEYS[1559] == nullptr, 1, "slot 1559 is empty");

    // Open points 6 §6.1: a level-29 insect guard's bite, as measured in the game: 20.7-35.8 after
    // the armour of a level-1 warrior, at the spread's ends.
    {
        Combatant guard;
        guard.controller = 1;
        guard.level = 29;
        guard.appearanceFlags = 0x32;
        guard.action = 51;
        same(strikeBase(guard), 30.272274, "a natural weapon's strike base", 0.001);
        same(damage(guard), 3.363586, "the damage stat is only the block's reference", 0.001);
        int roll = 0;
        const auto draws = [&] { return (roll++ % 2) ? 0 : 32767; }; // no critical, the widest spread
        const glm::vec3 box(0.96f, 0.96f, 2.16f);
        const HitInput in = strikeInput(guard, {0, 0, 0}, box, {1, 0, 0}, box, draws);
        same(in.amount, 37.84, "the strongest bite", 0.01);
        same(in.area, 0, "a plain blow is no area hit");
        same(in.knockBack.z, 0.25 * 2 / attackSpeed(guard), "the push's lift", 0.001);
        same(strikeWindow(guard, 593, 16), 0, "not before the window");
        same(strikeWindow(guard, 600, 16), 1, "inside the window");
        same(strikeWindow(guard, 695, 16), 0, "after the window");
        same(reachesTarget(guard, {0, 0, 0}, 0, box, {0, 2.5, 0}, box), 1, "in reach ahead");
        same(reachesTarget(guard, {0, 0, 0}, 0, box, {0, 3.6, 0}, box), 0, "out of reach ahead");
    }

    // Open points 4 §9.4: a crow in the ground comes clear, its bottom 1.1 over a layer;
    // cells of unloaded zones lift nothing.
    {
        Body crow;
        crow.size = {0.96f, 0.96f, 2.16f};
        crow.flyer = true;
        crow.place({0.5, 0.5, 37});
        const BlockType grass = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 36 ? 4 : 0); };
        step(crow, 16, grass);
        step(crow, 16, grass);
        same(crow.blocks().z, 38.14, "clear of the grass, 1.1 over its layer", 0.06);
        Body held;
        held.size = crow.size;
        held.place({0.5, 0.5, 37});
        const BlockType unloaded = [](int32_t, int32_t, int32_t) { return UNLOADED_CELL; };
        step(held, 16, unloaded);
        same(held.blocks().z, 37, "no lift out of an unloaded zone", 0.05);
    }

    // Models §4.3: a male human with head 2, hair 7, colour 120/80/40.
    {
        Appearance a;
        CharacterChoice c;
        c.race = 0;
        c.head = 2;
        c.hair = 7;
        c.hairColour[0] = 120;
        c.hairColour[1] = 80;
        c.hairColour[2] = 40;
        int32_t type = 99;
        int draws = 0;
        generate(type, a, &c, [&] { return draws++, 0; });
        same(type, 2, "human type");
        same(draws, 0, "no draws with a choice record");
        same(a.model(Part::Head), 1248, "head human-head-m03");
        same(a.model(Part::Hair), 1259, "hair human-hair-m08");
        same(a.model(Part::Hand), 430, "hand2");
        same(a.model(Part::Foot), 432, "foot");
        same(a.model(Part::Body), 1, "body4");
        same(a.hairColour[0], 120, "hair red");
        same(a.box.z, 2.16f, "box height");
        c.head = 3;
        Appearance darker;
        generate(type, darker, &c, [] { return 0; });
        same(darker.model(Part::Hand), 431, "head 3 gets brown-hand");
    }

    // Models §7.6: the walking hand targets at amplitude 1, phase π/3.
    {
        Appearance a;
        a.box = {0.96f, 0.96f, 2.16f};
        PoseState s;
        s.amplitude = 1;
        s.phase = (float)(3.141592653589793 / 3);
        Motion m;
        poseParts(s, a, m, 16, [](uint16_t) { return glm::vec3(10); });
        // (The spec's 5.43 rounds 12 cos 1.0996 = 5.448 too far.)
        same(s.rightHand.x, 5.448, "right hand x", 0.01);
        same(s.rightHand.y, 7.80, "right hand y", 0.01);
        same(s.rightHand.z, 1.9, "right hand z", 0.01);
        same(s.leftHand.x, -5.448, "left hand x", 0.01);
        same(s.leftHand.y, -13.60, "left hand y", 0.01);
    }

    // Models §7.11 example 2: a greatsword wind-up (pose 2) at t = 200 ms, one 16 ms frame from rest.
    {
        Appearance a;
        a.box = {0.96f, 0.96f, 2.16f};
        Combatant c;
        c.multipliers[1] = 1 / attackSpeed(c); // S·a = 1, so P1 = 800 ms
        c.equipment[MAIN_HAND] = {3, GREATSWORD, 1, 0, 1, 0};
        c.action = 57;
        c.actionMs = 200;
        same(poseOf(c), 2, "action 57 is pose 2");
        same(phases(c, 57).windUp, 800, "P1 800", 1);
        PoseState s;
        s.fresh = false;
        Motion m;
        m.fighter = &c;
        poseParts(s, a, m, 16, [](uint16_t) { return glm::vec3(10); });
        same(s.turnR.x, 11.6977, "right-hand turn after one frame", 0.05);
        // Models §6.5: food held out to a pet (action 82) is drawn between the hands.
        {
            Combatant holder = c;
            holder.action = 82;
            holder.equipment[12].type = 20, holder.equipment[12].subtype = 19;
            Motion hm;
            hm.fighter = &holder;
            PoseState hs;
            const auto parts = poseParts(hs, a, hm, 16, [](uint16_t) { return glm::vec3(10); });
            same(std::any_of(parts.begin(), parts.end(),
                             [&](const PartDraw &d) { return d.model == itemModel(holder.equipment[12]); }),
                 1, "the food in the hands");
        }
        same(s.handR.z, 0.779847, "right-hand lift after one frame", 0.005);
        same(s.rightHand.z, -3.320153, "drawn right hand z", 0.005);
        same(s.leftHand.x, 8, "left hand gripped at R + (2, 0, 0)", 1e-4);
    }

    // Trails §1.5: a sword swing (action 1, pose 16) opens a trail between 300 and 750 ms and
    // collapses it after; the shield charge release (action 104) traces half a ring.
    {
        Appearance a;
        a.box = {0.96f, 0.96f, 2.16f};
        Combatant c;
        c.multipliers[1] = 1 / attackSpeed(c);
        c.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        c.action = 1;
        Motion m;
        m.fighter = &c;
        PoseState s;
        const auto voxels = [](uint16_t) { return glm::vec3(3, 3, 16); };
        std::vector<TrailCorner> corners;
        for (float t = 0; t <= 400; t += 16) {
            c.actionMs = t;
            m.yawDegrees = t * 0.3f; // the swing: the blade sweeps round
            poseParts(s, a, m, 16, voxels);
        }
        trailTriangles(s, c, glm::vec3(0), corners);
        same(corners.size(), 90, "a swing's trail is 30 triangles");
        same(corners.size() == 90 ? corners[24].alpha : 0, 30, "butt alpha at point 4");
        same(corners.size() == 90 ? corners[25].alpha : 0, 122, "tip alpha at point 4");
        c.actionMs = 800;
        poseParts(s, a, m, 16, voxels);
        corners.clear();
        trailTriangles(s, c, glm::vec3(0), corners);
        same(corners.size(), 0, "collapsed after 750 ms");
        c.equipment[MAIN_HAND] = {3, BOW, 1, 0, 1, 0};
        c.actionMs = 400;
        s.trail[0][5] += glm::vec3(1);
        corners.clear();
        trailTriangles(s, c, glm::vec3(0), corners);
        same(corners.size(), 0, "archers show no trail");

        c.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        c.action = 104;
        c.actionMs = 150;
        m.yawDegrees = 0;
        PoseState arc;
        arc.fresh = false;
        poseParts(arc, a, m, 16, voxels);
        same(arc.trail[0][8].y, 10, "the ring 10 blocks in front", 1e-4);
        same(arc.trail[1][8].y, 5, "its inner edge 5 blocks in front", 1e-4);
        same(arc.trail[0][0].x, -10, "starting 10 blocks to the left", 1e-4);
    }

    // Particles §10: a bomb burst's particle, collision coming to rest, the cap and a flame.
    {
        Burst b = bombBurst({10, 20, 30});
        const auto bytes = b.serialize();
        same(bytes.size(), 72, "a burst record is 72 bytes");
        const Burst back = Burst::parse(bytes.data());
        same(back.count, 20, "the count round-trips");
        same(back.spread, 3, "spread 3");
        same((double)back.position[2], 30.0 * 65536, "the position round-trips");
        b.count = 1;
        // Draws: velocity z 0.9, y 0.2, x 0.5; spin z, y, x; then the fire colour.
        const int draws[] = {29490, 6553, 16383, 16383, 16383, 16383, 32767};
        int next = 0;
        Particles list;
        list.expand(b, [&] { return draws[next++]; });
        same(list.list.size(), 1, "one particle");
        same(list.list[0].velocity.z, 22.4, "velocity z 22.4", 0.01);
        same(list.list[0].velocity.y, -1.8, "velocity y -1.8", 0.01);
        same(list.list[0].velocity.x, 0, "velocity x 0", 0.001);
        same(list.list[0].colour.g, 1, "a fire colour (1, u, 0)", 1e-6);
        const BlockType air = [](int32_t, int32_t, int32_t) { return (uint8_t)0; };
        list.update(16, air);
        same(list.list[0].velocity.z, 22.4 - 0.48, "gravity 30 blocks/s/s", 0.01);
        for (int k = 0; k < 250; k++)
            list.update(16, air);
        same(list.list.size(), 0, "gone after 4000 ms");

        // Example 5: a falling cube that collides stops dead on the ground.
        Particle p;
        p.collides = true;
        p.velocity = {3, 0, -40};
        p.spin = {10, 10, 10};
        p.position[0] = 0, p.position[1] = 0, p.position[2] = (int64_t)(10.1 * 65536);
        list.list = {p};
        const BlockType ground = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z < 10 ? 4 : 0); };
        list.update(20, ground);
        same(glm::length(list.list[0].velocity), 0, "stopped on the ground");
        same(glm::length(list.list[0].spin), 0, "and stopped turning");
        same(list.list[0].position[2] >= 10 * 65536, 1, "above the grass");

        list.list.assign(6040, Particle{});
        list.update(0, air);
        same(list.list.size(), 5000, "capped at 5000");

        // §5.4: a fire explosion ends in 80 fire cubes spread 20, with sound 81.
        {
            Combatant mage;
            mage.action = 87;
            const float D = (float)duration(mage, 87);
            Body body;
            ShotEffects fx;
            std::vector<Projectile> shots;
            launch(mage, body, glm::vec3(0), D - 10, 16, [] { return 0; }, shots, fx);
            same(fx.bursts.size(), 1, "one explosion burst");
            same(fx.bursts.empty() ? 0 : fx.bursts[0].count, 80, "80 particles");
            same(fx.bursts.empty() ? 0 : fx.bursts[0].spread, 20, "spread 20");
            same(std::any_of(fx.sounds.begin(), fx.sounds.end(), [](const SoundEvent &e) { return e.sound == 81; }), 1,
                 "sound 81");
        }

        // Open points 3 §3.7 example 1: a staff's charge cone, flame 0 at wall clock 1000.
        {
            const auto cone = flames(1000, 3.2f, 0.32f, 86, {1, 0, 0}, {1, 1, 0}, 0.3f);
            same(cone[0].edge, 0.48, "cone flame edge", 1e-4);
            same(cone[0].position.x, 0, "a quarter turn round", 1e-3);
            same(cone[0].position.y, 1.6, "half the radius out", 1e-3);
            same(cone[0].position.z, 3.2, "half way up", 1e-4);
        }

        // §8: campfire flame 0 at t = 1000.
        const auto fire = flames(1000, 0.8f, 0.2f, 20, {1, 0, 0}, {1, 1, 0});
        same(fire[0].edge, 0.3, "flame edge 0.3", 1e-5);
        same(fire[0].colour.g, 0.5, "half way to yellow", 1e-5);
        same(fire[0].position.x, 0.647, "on its circle x", 0.001);
        same(fire[0].position.y, 0.470, "on its circle y", 0.001);
        same(fire[0].position.z, 0.8, "half way up", 1e-5);
    }

    // Models §7.3: a runner's feet come down as the walk phase passes each multiple of π; a
    // walker slower than 5 blocks/s and a dodger make no footsteps.
    {
        Appearance a;
        a.box = {0.96f, 0.96f, 2.16f};
        const auto steps = [&](glm::vec3 v, float dodge) {
            PoseState s;
            Motion m;
            m.velocity = v;
            m.dodgeMs = dodge;
            int n = 0;
            for (int i = 0; i < 120; i++) // 2 s
                n += stepWalk(s, a, m, 16);
            return n;
        };
        same(steps({8, 0, 0}, 0) > 2, 1, "a runner's footsteps");
        same(steps({4, 0, 0}, 0), 0, "none below 5 blocks/s");
        same(steps({8, 0, 0}, 300), 0, "none while dodging");
    }

    // Audio §6.2: a sword swing whooshes as it crosses P1; a greatsword is lifted with a groan
    // of swing-slow (17) and swings an octave lower.
    {
        Combatant c;
        c.multipliers[1] = 1 / attackSpeed(c);
        c.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        c.action = 1;
        std::vector<SoundEvent> sounds;
        actionSounds(c, 290, 16, glm::dvec3(0), [] { return 16383; }, sounds);
        same(sounds.size() == 1 && sounds[0].sound == 15, 1, "the swing at P1");
        same(sounds.empty() ? 0 : sounds[0].pitch, 1.5, "pitch (0.9 + 0.2u) x 1.5", 0.001);
        c.equipment[MAIN_HAND] = {3, GREATSWORD, 1, 0, 1, 0};
        c.action = 57;
        sounds.clear();
        actionSounds(c, 0, 16, glm::dvec3(0), [] { return 16383; }, sounds);
        same(sounds.size() == 1 && sounds[0].sound == 17, 1, "a two-hander is lifted");
    }

    // Quests (docs/design/quests.md): the board is the same for everyone; a battle's allies
    // and enemies fight; a prisoner is freed once its guards are gone; the book saves.
    {
        const quests::Quest a = quests::offer(123, 0x1234, 2, {1000, 1000, 40}, 7, 5, 1);
        const quests::Quest b = quests::offer(123, 0x1234, 2, {1000, 1000, 40}, 7, 5, 1);
        same(a == b, 1, "the same offer for the same giver and day");
        same(!quests::giverName(a).empty() && quests::offerText(a, a.giverAt, "Trolls").find(quests::giverName(a)) != std::string::npos,
             1, "the giver says its name");
        same(a.id != quests::offer(123, 0x1234, 2, {1000, 1000, 40}, 8, 5, 1).id, 1, "a new one the next day");
        const double far = glm::length(glm::dvec2(a.site - a.giverAt));
        same(far >= 120 && far <= 300, 1, "the place is 120-300 blocks away");

        quests::Book book;
        book.quests.push_back(a);
        book.quests[0].count = 1;
        same(quests::Book::load(book.save()).quests == book.quests, 1, "the book round-trips");

        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({40.5, 150.5, 10 + 1.08 + 0.01});

        quests::Quest battle = a;
        battle.kind = quests::BATTLE, battle.allies = 5, battle.enemies = 5, battle.required = 5, battle.count = 0;
        battle.site = {40, 40, 10};
        battle.level = 3;
        Npcs npcs;
        npcs.setQuestCreatures(battle.id, quests::creatures(battle, [](int32_t, int32_t) { return 10; }));
        int enemyDeaths = 0, allyDeaths = 0;
        for (int i = 0; i < 3750; i++) { // 60 s
            const NpcFrame f = npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, flat);
            for (const auto &d : f.deaths)
                (d.questRole == quests::ENEMY ? enemyDeaths : allyDeaths)++;
        }
        same(enemyDeaths + allyDeaths > 0, 1, "the two sides of a battle fight");

        quests::Quest rescue = a;
        rescue.kind = quests::RESCUE, rescue.enemies = 4, rescue.prisoners = 1, rescue.required = 1, rescue.count = 0;
        rescue.site = {40, 140, 10};
        Npcs camp;
        camp.setQuestCreatures(rescue.id, quests::creatures(rescue, [](int32_t, int32_t) { return 10; }));
        camp.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, flat);
        uint64_t prisoner = 0;
        for (const Npc &n : camp.all())
            if (n.questRole == quests::PRISONER)
                prisoner = n.fighter.id;
        same(prisoner != 0 && camp.guarded(rescue.id, {40, 140, 10}, quests::FREE_CLEAR), 1, "a guarded prisoner");
        for (Npc &n : camp.npcs())
            if (n.fighter.id == prisoner) {
                same(n.fighter.mode == n.body.mode && (n.fighter.mode & 0x400), 1,
                     "quest activation publishes the captive's sitting flag to both state views");
                n.fighter.mode |= 0xa020;
            }
        same(camp.free(prisoner, {0, 0, 10}), rescue.id, "freed, it belongs to its quest");
        for (const Npc &n : camp.all())
            if (n.fighter.id == prisoner)
                same(n.fighter.mode == n.body.mode && (n.fighter.mode & 0xa420) == 0xa020, 1,
                     "freeing a captive clears only sitting and immediately publishes the full retained word");
        same(camp.free(prisoner, {0, 0, 10}), 0, "and is no captive any more");

        quests::Quest champion = a;
        champion.kind = quests::CHAMPION, champion.enemies = 3, champion.required = 1, champion.count = 0;
        champion.level = 4;
        const std::vector<worldgen::Spawn> band = quests::creatures(champion, [](int32_t, int32_t) { return 10; });
        same(band.size() == 4 && band[0].questRole == quests::LEADER && band[0].level == 4 + quests::CHAMPION_LEVELS, 1,
             "a champion three levels up, with its followers");
        quests::Book hunt;
        hunt.quests.push_back(champion);
        same(hunt.died(champion.id, quests::ENEMY), 0, "a follower does not count");
        same(hunt.died(champion.id, quests::LEADER) && hunt.quests[0].state == quests::DONE, 1, "the champion does");
        int kinds[5] = {};
        for (uint64_t giver = 1; giver < 400; giver++)
            kinds[quests::offer(123, giver, 2, {0, 0, 0}, 1, 1, 0).kind]++;
        same(kinds[1] && kinds[2] && kinds[3] && kinds[4], 1, "every kind is offered");
    }

    // Traps (alpha-open-points-1.md §1.7): a spike trap in a level-5 place retracts after 2 s,
    // shoots up for a player 3 blocks away, and hurts it 20 a pass; hostile monsters are spared.
    {
        worldgen::Zone zone;
        worldgen::StaticObject spike;
        spike.kind = 7;
        spike.x = 5ll << 16, spike.y = 5ll << 16, spike.z = 1ll << 16;
        spike.state = 1;
        spike.timer = 3127;
        zone.statics.push_back(spike);
        StaticStates states;
        states.generated(zone);
        const std::map<StaticStates::Key, const worldgen::Zone *> held{{{0, 0}, &zone}};
        const auto none = [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &) { return false; };
        const auto level5 = [](glm::dvec3) { return 5; };
        std::vector<HitRecord> hits;
        std::vector<SoundEvent> sounds;
        const std::vector<StaticStates::Creature> nobody;
        states.step(16, none, held);
        states.traps(16, 1000, nobody, held, level5, hits, sounds);
        same((*states.zone(0, 0))[0].state, 0, "retracted after 2 s");
        const std::vector<StaticStates::Creature> here{{1, {8, 5, 1.91}, 0}, {2, {5, 7, 1.91}, 1}};
        double clock = 1016;
        for (int i = 0; i < 130 && (*states.zone(0, 0))[0].state == 0; i++, clock += 16) {
            states.step(16, none, held);
            states.traps(16, clock, here, held, level5, hits, sounds);
        }
        same((*states.zone(0, 0))[0].state, 1, "up for the player");
        same(std::any_of(sounds.begin(), sounds.end(), [](const SoundEvent &e) { return e.sound == 52; }), 1, "with sound 52");
        hits.clear();
        for (int i = 0; i < 7; i++, clock += 16) {
            states.step(16, none, held);
            states.traps(16, clock, here, held, level5, hits, sounds);
        }
        same(!hits.empty() && hits[0].target == 1, 1, "the player is hurt");
        same(hits.empty() ? 0 : hits[0].amount, 20, "20 a pass at level 5", 1e-4);
        same(hits.empty() ? 0 : hits[0].knockBack.x, 20, "thrown away from it", 1e-4);
        same(std::none_of(hits.begin(), hits.end(), [](const HitRecord &h) { return h.target == 2; }), 1,
             "the monster is spared");
    }

    // Open points 2 §2.2: a static only stops motion toward its centre (base on z).
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::StaticObject table;
        table.kind = 12;
        table.x = 10ll << 16, table.y = 10ll << 16, table.z = 5ll << 16;
        table.scale[0] = table.scale[1] = 3, table.scale[2] = 1;
        zone->statics.push_back(table);
        worldgen::WorldMap map;
        map.add(zone);
        StaticStates states;
        states.generated(*zone);
        const glm::dvec3 lo(10.6, 9.5, 5.2), hi(11.6, 10.5, 7.2); // overlapping its +x half
        same(states.blocks(map, lo, hi), 1, "it overlaps");
        same(states.blocks(map, lo, hi, 0, 0.1), 0, "walking out on +x is free");
        same(states.blocks(map, lo, hi, 0, -0.1), 1, "walking further in is not");
    }

    // Open points 2 §1: the search walks round a stand 3.5 blocks tall; as read (§1.4, the
    // +y ledge flag opens the gate on flat ground, open point OP2-3) a table blocks it too.
    {
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        const auto route = [&](double height) {
            PathSearch search;
            search.statics = [height](int32_t, int32_t, int32_t, int32_t, const std::function<void(const SearchStatic &)> &visit) {
                SearchStatic s;
                s.at = {5.5, 0.5, 1};
                s.half = {0.5, 3};
                s.height = height;
                s.kind = 12;
                s.ref = {0, 0, 0};
                visit(s);
            };
            search.start({0.5, 0.5, 1}, {10.5, 0.5, 1}, 1);
            for (int i = 0; i < 40 && search.searching(); i++)
                search.rounds({0.96f, 0.96f, 2.16f}, 1, flat);
            bool through = false;
            for (const auto &c : search.path)
                through = through || (c[0] == 5 && c[1] > -3 && c[1] < 3);
            same(!search.path.empty() && search.path.back()[0] >= 9, 1, "the search gets there");
            return through;
        };
        same(route(1), 0, "round a table on flat ground (as read)");
        same(route(3.5), 0, "round a tall stand");
    }

    // Models §6.2: weapon models.
    same(itemModel({3, 0, 1, 0, 1, 0}), 899, "iron sword seed 0");
    same(itemModel({3, GREATSWORD, 1, 2, 1, 3}), 5 * (3 + 365) + 2, "greatsword seed 3 rarity 2");
    same(itemModel({3, 1, 5, 0, 1, 0}), NO_MODEL, "obsidian axe has no model");

    // Physics §1: how a frame is cut into steps.
    {
        auto pieces = [](int dt) {
            std::string s;
            for (int p : stepPieces(dt))
                s += std::to_string(p) + " ";
            return s;
        };
        same(pieces(16) == "16 ", 1, "dt 16");
        same(pieces(21) == "20 1 ", 1, "dt 21");
        same(pieces(45) == "20 20 5 ", 1, "dt 45");
        same(pieces(100) == "20 20 20 20 20 ", 1, "dt 100");
    }

    // Physics §6: walking on flat ground moves at S = 6 and stores 6 · 0.96074 after friction.
    {
        Body b;
        b.place({0.5, 0.5, 1 + 1.08 + 0.01});
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        step(b, 16, floor); // settle onto the ground
        b.intent = {0, 80, 0};
        for (int i = 0; i < 30; i++) // up to walking speed
            step(b, 16, floor);
        const double y0 = b.blocks().y;
        step(b, 16, floor);
        same(b.blocks().y - y0, 0.096, "walking move per frame", 1e-4);
        same(b.velocity.y, 6 * std::pow(1 - 0.0025, 16), "stored speed after friction", 1e-3);
    }

    // Travel §0, §3.5: the skill factor and the wind's noise.
    same(skillFactor(1), 0.090909, "v(1)", 1e-5);
    same(skillFactor(10), 0.5, "v(10)", 1e-6);
    same(valueNoise(0, 0), -0.281791, "N2 at the origin", 1e-5);
    {
        auto air = [](int32_t, int32_t, int32_t) -> uint8_t { return 0; };
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        auto sea = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 2 : 0; };
        // Travel §3.8 example 1 without the wind (not the player): v = (8.16, 0, −2.048).
        {
            Body b;
            b.place({0.5, 0.5, 100});
            b.mode = GLIDER;
            b.skills[3] = 1;
            b.velocity = {8, 0, -2};
            b.intent = {10, 0, 0};
            step(b, 16, air);
            same(b.velocity.x, 8.16, "glide step v.x", 1e-4);
            same(b.velocity.z, -2.048, "glide step sinks at a tenth of gravity", 1e-4);
            b.velocity = {20, 0, -1};
            step(b, 16, air);
            same(b.velocity.x, 6 * 1.772727, "glider speed limit 10.636", 1e-3);
        }
        // Example 3: hovering with Space; the player feels the wind too.
        {
            Body b;
            b.place({0.5, 0.5, 100});
            b.local = true;
            b.mode = GLIDER | RUN;
            b.skills[3] = 1;
            b.velocity = {0, 0, -0.5f};
            const float wz = wind(b.position).z;
            step(b, 16, air);
            same(b.velocity.z, -0.068 + wz * 0.016, "hover v.z", 1e-4);
            same(b.stamina, 0.9968, "lift stamina", 1e-5);
        }
        // Example 5: into a wall at (−9, 0, −1) with N = (1, 0, 0) crashes.
        {
            Body b;
            b.place({0.5, 0.5, 100});
            b.local = true;
            b.mode = GLIDER;
            b.skills[3] = 1;
            b.contact = ON_WALL;
            b.wallNormal = {1, 0, 0};
            b.velocity = {-9, 0, -1};
            step(b, 16, air);
            same(b.crashed, 1, "wall crash");
            same((b.mode & GLIDER) != 0, 0, "the crash folds the glider");
        }
        // Travel §4.3: sailing with boat driving 1, running: S = 15.273, stored 15.212.
        {
            Body b;
            b.place({0.5, 0.5, 9});
            b.action = 107;
            b.mode = RUN;
            b.skills[5] = 1;
            b.intent = {0, 80, 0};
            for (int i = 0; i < 40; i++)
                step(b, 16, sea);
            const double y0 = b.blocks().y;
            step(b, 16, sea);
            same(b.blocks().y - y0, 0.24436, "boat move per frame", 2e-4);
            same(b.velocity.y, 15.212, "boat stored speed", 2e-2);
        }
        // Travel §7: swimming 3, running: S = 7.385.
        {
            Body b;
            b.place({0.5, 0.5, 5});
            b.mode = RUN;
            b.skills[4] = 3;
            b.intent = {0, 80, 0};
            for (int i = 0; i < 40; i++)
                step(b, 16, sea);
            const double y0 = b.blocks().y;
            step(b, 16, sea);
            same(b.blocks().y - y0, 7.3846 * 0.016, "swim move per frame", 2e-4);
        }
        // Travel §5.8: riding with pet riding 1, running: S = 13.09; the ride ends without a pet
        // or with Ctrl.
        {
            Body b;
            b.place({0.5, 0.5, 1 + 1.08 + 0.01});
            b.local = true;
            b.action = 106;
            b.mountValid = true;
            b.mode = RUN;
            b.skills[1] = 1;
            step(b, 16, floor);
            b.intent = {0, 80, 0};
            for (int i = 0; i < 30; i++)
                step(b, 16, floor);
            const double y0 = b.blocks().y;
            step(b, 16, floor);
            same(b.blocks().y - y0, 6 * 1.090909 * 2 * 0.016, "riding move per frame", 2e-4);
            same(b.ridingEnergy, 1 - 31 * 0.002 * 0.016, "riding energy drain", 1e-5);
            b.mode |= CLIMB;
            step(b, 16, floor);
            same(b.action, 0, "Ctrl dismounts");
            b.action = 106, b.mode = RUN, b.mountValid = false;
            step(b, 16, floor);
            same(b.action, 0, "no pet, no ride");
        }
        // Travel §6.5: climbing holding W reaches 0.246 after a frame, 6.27 steady.
        {
            auto wall = [](int32_t x, int32_t, int32_t) -> uint8_t { return x >= 1 ? 1 : 0; };
            Body b;
            b.place({0.5, 0.5, 100});
            b.mode = CLIMB;
            b.skills[2] = 100;
            step(b, 16, wall); // find the wall
            same((b.contact & ON_WALL) != 0, 1, "touching the wall");
            b.velocity = glm::vec3(0);
            b.intent = {0, 0, 16};
            step(b, 16, wall);
            same(b.velocity.z, 0.246, "first climbing frame", 1e-3);
            for (int i = 0; i < 300; i++)
                step(b, 16, wall);
            same(b.velocity.z, 0.256 * 0.96075 / 0.03925, "steady climbing speed (6.27)", 1e-2);
            Body c;
            c.place({0.5, 0.5, 100});
            c.mode = CLIMB;
            step(c, 16, wall);
            c.intent = {0, 0, 16};
            step(c, 16, wall);
            same(c.stamina, 1 - (1 / 1.1) * 0.0005 * 16, "climbing stamina at skill 0", 1e-5);
        }
        // Travel §8: a 30-block fall takes a quarter of the player's health.
        {
            Body b;
            b.place({0.5, 0.5, 1 + 30 + 1.08 + 0.01});
            b.local = true;
            for (int i = 0; i < 200 && !(b.contact & ON_GROUND); i++)
                step(b, 16, floor);
            same(b.fallHurt, 0.25, "30-block fall (stopped up to a substep above the floor)", 2e-2);
        }
    }

    // Physics §7.5: θ 120, ψ 0, distance 10 puts the camera at focus + (0, 8.660, 5).
    {
        FollowCamera c;
        c.pitch = 120;
        c.yaw = 0;
        const glm::vec3 d = c.direction();
        same(d.x, 0, "camera d.x", 1e-6);
        same(d.y, -0.8660254, "camera d.y", 1e-6);
        same(d.z, -0.5, "camera d.z", 1e-6);
        c.look(40, 0);
        same(c.targetYaw, -10, "40 counts turn 10 degrees");
    }

    // Behaviour §4.1, §8.4: a level 1 player warrior (health multiplier 100) has 260 health.
    {
        Combatant c;
        same(maxHealth(c), 260, "warrior health", 1e-4);
        c.health = 1;
        c.action = 84;
        regenerate(c, 1000);
        same(c.health, 1 + 13, "action 84 adds 5 % per second", 1e-4);
    }

    // Behaviour §4.6: experience for the next level, and a level-up refilling health.
    {
        same(experienceForLevel(1), 50, "cost(1)");
        same(experienceForLevel(2), 97, "cost(2)");
        same(experienceForLevel(10), 360, "cost(10)");
        same(experienceForLevel(100), 881, "cost(100)");
        Combatant c;
        c.health = 0.5f;
        addExperience(c, 160);
        same(c.level, 3, "160 experience: level 3");
        same(c.experience, 160 - 50 - 97, "experience left");
        same(c.health, maxHealth(c), "health refilled");
    }
    {
        // Held level-consumer boundaries: one health query per admitted level,
        // after debit/increment; nonpositive entry levels skip threshold work.
        const auto previousHealth = itemBonuses().health;
        Combatant c;
        c.health = .5f;
        c.equipment[CHEST_SLOT] = {4, 0, 1, 0, 1, 0};
        std::vector<std::pair<int32_t, int32_t>> queries;
        std::vector<float> previousValues;
        itemBonuses().health = [&](const Item &) {
            queries.emplace_back(c.level, c.experience);
            previousValues.push_back(c.health);
            return 0.f;
        };
        addExperience(c, 160);
        same(queries == std::vector<std::pair<int32_t, int32_t>>{{2, 110}, {3, 13}}, 1,
             "each level queries maximum health once after its current threshold debit and level store");
        same(previousValues.size() == 2 && previousValues[0] == .5f && previousValues[1] > .5f && c.health > previousValues[1], 1,
             "health is replaced between repeated increments without an extra maximum query");
        queries.clear();
        c.level = 1, c.experience = 49, c.health = 17;
        addExperience(c, 0);
        same(c.level == 1 && c.experience == 49 && c.health == 17 && queries.empty(), 1,
             "below-threshold zero credit makes no health query or write");
        c.experience = 50;
        addExperience(c, 0);
        same(c.level == 2 && c.experience == 0 && queries.size() == 1, 1,
             "threshold equality can level immediately even for zero added credit");
        queries.clear();
        c.level = 0, c.experience = INT32_MAX, c.health = 17;
        addExperience(c, 1);
        same(c.experience == INT32_MIN && c.level == 0 && c.health == 17 && queries.empty(), 1,
             "experience addition wraps all 32 bits before nonpositive-level entry rejection");
        addExperience(c, -1);
        same(c.experience == INT32_MAX && c.health == 17 && queries.empty(), 1,
             "negative credit is not clamped and entry rejection makes no maximum query");
        itemBonuses().health = previousHealth;
    }
    {
        Inventory wallet;
        ItemRecord platinum{};
        platinum[0] = 13, platinum[1] = 0xff, platinum[0x0c] = 0xff, platinum[0x0d] = 0xff, platinum[0x0e] = 0xff;
        setItemLevel(platinum, 9);
        const auto original = platinum;
        wallet.add(platinum);
        same(wallet.platinum == 9 && wallet.money == 0 && wallet.bags.size() == 2 &&
             wallet.bags[0].empty() && wallet.bags[1].empty() && platinum == original, 1,
             "category thirteen credits signed quantity directly after group growth without an item entry or input mutation");
        setItemLevel(platinum, -7);
        wallet.add(platinum);
        same(wallet.platinum, 2, "a negative signed Platinum Coins quantity reduces the same balance");
        setItemLevel(platinum, 0);
        wallet.add(platinum);
        same(wallet.platinum, 2, "zero Platinum Coins quantity is accepted without changing the balance");
        wallet.platinum = UINT32_MAX;
        setItemLevel(platinum, 1);
        wallet.add(platinum);
        same(wallet.platinum, 0, "Platinum Coins credit retains low thirty-two bits on wrap");
    }

    // Behaviour §3: a bare-handed player punches 6, then 7 when pressed again in time.
    {
        Combatant c;
        c.actionMs = 1000; // action 0 long over
        same(startPrimaryAttack(c), 1, "attack starts");
        same(c.action, 6, "first punch");
        // S = 1 + 0.1·P/8 with P = 1 at level 1.
        same(phases(c, 6).windUp, (int)(50 / 1.0125f), "punch wind-up");
        same(duration(c, 6), 49 + 148 + 493, "punch duration");
        same(startPrimaryAttack(c), 1, "busy primary attempt remains admitted");
        same(c.actionMs, 0, "busy primary attempt leaves the punch clock alone");
        stepAction(c, 700);
        same(startPrimaryAttack(c), 1, "attack again");
        same(c.action, 7, "second punch chains");
        c.equipment[MAIN_HAND] = {3, GREATSWORD};
        c.action = 0;
        same(primaryAttack(c), 57, "great weapon opens with 57");
        c.combo = 3;
        same(secondaryAttack(c), 65, "odd combo: 65");
    }

    // Behaviour §5: armour takes off its full value with no combo running.
    {
        Combatant attacker, target;
        attacker.id = 1;
        target.id = 2;
        target.controller = 1;
        target.health = 100;
        target.multipliers[0] = 100; // maximum health 100
        target.multipliers[3] = 2; // armour 2·P·2^0 = 2
        HitInput hit;
        hit.amount = 10;
        hit.knockBack = {0, 3, 0};
        glm::vec3 v(1, 0, 0);
        int draws = 0;
        HitOutcome out = resolveHit(target, v, &attacker, hit, [&] { return draws++, 16383; });
        same(out.records.size(), 1, "one record");
        same(out.records[0].amount, 8, "10 minus armour 2", 1e-5);
        same(out.records[0].kind, HIT_DAMAGE, "damage kind");
        same(v.x, 1, "stable native producer leaves motion velocity unchanged", 1e-5);
        same(out.sounds.size(), 1, "impact sound");
        same(out.sounds[0].sound, 7, "fist impact");
        Body targetBody;
        targetBody.velocity = v;
        applyHit(target, targetBody, out.records[0], {&attacker});
        same(target.health, 92, "health after the hit", 1e-4);
        same(targetBody.push.y, 3, "response remains separate from velocity");
    }

    // Behaviour §10.4: item stats.
    {
        Item sword{3, 0, 1, 0, 5, 0};
        same(itemDamage(sword), 5.6568546, "iron sword damage", 1e-6);
        sword.rarity = 2;
        same(itemDamage(sword), 8.0, "rarity 2", 1e-5);
        sword.rarity = 0;
        sword.spirits = 3;
        same(itemDamage(sword), 5.7791777, "three spirits", 1e-5);
        same(itemDamage({3, GREATSWORD, 1, 0, 5}), 11.313709, "greatsword", 1e-5);
        same(itemDamage({3, DAGGER, 1, 0, 5}), 2.8284273, "dagger", 1e-6);
        Item chest{4, 0, 1, 0, 5, 0};
        same(itemHealth(chest), 21.213205, "chest health seed 0", 1e-5);
        chest.seed = 1;
        same(itemHealth(chest), 18.384777, "seed 1", 1e-5);
        chest.seed = 7;
        same(itemHealth(chest), 16.263456, "seed 7", 1e-5);
        same(itemArmour(chest), 1.4142137, "chest armour", 1e-6);
        same(itemArmour({6, 0, 1, 0, 5}), 0.7071068, "boots armour", 1e-6);
        Item seeded{3, 0, 1, 0, 5, 10};
        same(itemSpeed(seeded), 0.008838835, "sword speed", 1e-8);
        same(itemCritical(seeded), 0.0044194176, "sword crit", 1e-8);
        seeded.seed = 20;
        same(itemCritical(seeded), 0, "crit below 0.001");
        // Equipped: a level 1 iron sword in each hand adds 5 to 7.5 health each.
        Combatant c;
        c.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        const float h = maxHealth(c) - 260;
        same(h >= 5 && h <= 7.5, 1, "one sword's health");
    }

    // Behaviour §14: sound ids to keys.
    same(std::string(soundKey(24)) == "block.wav", 1, "sound 24");
    same(std::string(soundKey(62)) == "male-groan.wav", 1, "sound 62");
    same(std::string(soundKey(100)) == "owl2.wav", 1, "sound 100");
    same(soundKey(101) == nullptr, 1, "sound 101 plays nothing");

    // Behaviour §12.3: kill experience.
    {
        same(killExperience(1, 0, 4, false, 0), 1, "level 1 kill");
        same(killExperience(5, 0, 4, false, 0), 3, "level 5 kill");
        same(killExperience(10, 2, 4, false, 0), 7, "level 10 tier 2 kill");
        same(killExperience(20, 0, 4, false, 0), 9, "level 20 kill");
        same(killExperience(5, 0, 4, true, 0), 33, "level 5 boss");
    }

    // Behaviour §8.1: an action that misses its active window ends the chain.
    {
        Combatant c;
        c.actionMs = 1000;
        startPrimaryAttack(c);
        c.combo = 4;
        stepAction(c, 100);
        same(c.combo, 4, "inside the wind-up the combo stands");
        stepAction(c, 200);
        same(c.combo, 0, "missed: chain ends");
    }

    // Pets and bombs §3, §4, §5: a placed bomb burns for 4000 P(L) ms and goes off once, hitting
    // a player 5 blocks away (25 P(L), or twice that on a critical, less resistance) and
    // pushing them away; its id is the first below zero.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({15.5, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        const uint64_t bomb = npcs.placeBomb({10.5, 10.5, 1.5}, 3);
        same((int64_t)bomb, -1, "the first bomb's id");
        same((int64_t)npcs.placeBomb({60.5, 10.5, 1.5}, 3), -2, "the next one below it");
        same(npcs.all()[0].fighter.health, 500, "the new-creature health");
        const float fuse = 4000 * levelPower(3);
        float t = 0;
        int blasts = 0, hits = 0;
        for (int i = 0; i < 20000 && t < fuse + 2000; i++) {
            const NpcFrame f = npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
            t += 16;
            for (const HitEmission &emission : f.playerHits) {
                const HitRecord &h = emission.record;
                if (h.attacker == bomb) {
                    hits++;
                    same(h.amount > 0 && h.amount <= 50 * levelPower(3), 1, "blast damage within 25 P(L) or a critical");
                    same(h.knockBack.x > 9 && std::abs(h.knockBack.z - 2.5f) < 1e-4f, 1, "pushed away from the bomb");
                }
            }
            for (const auto &e : f.explosions)
                if (e.bomb == bomb) {
                    blasts++;
                    same(t, fuse, "it goes off when its health runs out", 16);
                    same(e.level, 3, "the blast keeps the bomb's level");
                }
        }
        same(blasts, 1, "one blast");
        same(hits, 1, "the player 5 blocks away is hit once");
    }

    // Physics §4.5: a crate (a static 2 x 2 x 1 at x 5..7) stops a walker, which may stand on it;
    // §4.4: overlapping creatures are pushed apart.
    {
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        const ObjectTest crate = [](glm::dvec3 lo, glm::dvec3 hi, int, double, const IgnoredStatics *) {
            return lo.x < 7 && hi.x > 5 && lo.y < 1.5 && hi.y > -0.5 && lo.z < 2 && hi.z > 1;
        };
        Body b;
        b.place({0.5, 0.5, 1 + 1.08 + 0.01});
        b.intent = {80, 0, 0};
        bool touched = false;
        for (int i = 0; i < 120; i++) {
            step(b, 16, flat, crate);
            touched = touched || (b.contact & ON_OBJECT);
        }
        same(b.blocks().x + 0.48 <= 5.001, 1, "stopped at the crate's face");
        same(touched, 1, "contact 0x40 set");
        Body top;
        top.place({6, 0.5, 2 + 1.08 + 0.5});
        for (int i = 0; i < 30; i++)
            step(top, 16, flat, crate);
        same(std::abs(top.blocks().z - (2 + 1.08)) < 0.05 && (top.contact & ON_GROUND), 1, "standing on the crate");
        const glm::vec3 push = separation({0.96f, 0.96f, 2.16f}, {1.96f, 1.96f, 2.16f}, {1, 0, 0}, {0.5, 0, 0}, false);
        same(push.x, 5 * 1.96 / 0.96, "pushed away at 5 s'x/sx blocks a second", 1e-4);
    }
    // Physics §3.3: a dash (action 76) drives 20 a step along the facing, capped at 60, only
    // between P1 and P1 + X; §3.2: a pet's base speed is 12.
    {
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Body b;
        b.place({0.5, 0.5, 1 + 1.08 + 0.01});
        step(b, 16, flat);
        b.action = 76, b.windUpMs = 100, b.activeMs = 400, b.durationMs = 1000, b.yaw = 0;
        b.actionMs = 50;
        step(b, 16, flat);
        same(std::abs(b.velocity.y) < 0.01, 1, "no dash before P1");
        b.actionMs = 200;
        for (int i = 0; i < 4; i++)
            step(b, 16, flat);
        same(b.velocity.y > 40 && b.velocity.y <= 60, 1, "dashing forward, at most 60");
        Body pet;
        pet.place({0.5, 0.5, 1 + 1.08 + 0.01});
        pet.controller = 5;
        pet.intent = {0, 80, 0};
        for (int i = 0; i < 30; i++)
            step(pet, 16, flat);
        const double y0 = pet.blocks().y;
        step(pet, 16, flat);
        same(pet.blocks().y - y0, 12 * 0.016, "a pet walks at 12", 1e-4);
        // §3.3: a swing's first 200 ms lunges along the facing unless an enemy is in contact.
        Body swing;
        swing.place({0.5, 0.5, 1 + 1.08 + 0.01});
        step(swing, 16, flat);
        swing.action = 3, swing.actionMs = 50, swing.yaw = 0;
        step(swing, 16, flat);
        same(swing.velocity.y > 5, 1, "lunging forward");
        Body near = swing;
        near.velocity = glm::vec3(0);
        near.targetInContact = true;
        step(near, 16, flat);
        same(std::abs(near.velocity.y) < 0.01, 1, "no lunge with an enemy in reach");
        same(inContact({0.96f, 0.96f, 2.16f}, {0, 0, 0}, {0.96f, 0.96f, 2.16f}, {2.5, 0, 0}), 1, "2.5 blocks is in contact");
        same(inContact({0.96f, 0.96f, 2.16f}, {0, 0, 0}, {0.96f, 0.96f, 2.16f}, {2.7, 0, 0}), 0, "2.7 is not");
    }
    // Path search §2.1: open ground, a one-block ledge and a two-block wall.
    {
        const glm::vec3 box(0.96f, 0.96f, 2.16f);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z <= 9 ? 1 : 0; };
        same(clearRoute(box, false, {0.5, 0.5, 11.08}, {10.5, 0.5, 11.08}, 2.88f, flat), 1, "open ground is clear");
        auto ledge = [](int32_t x, int32_t, int32_t z) -> uint8_t { return z <= 9 || (z == 10 && x >= 103) ? 1 : 0; };
        same(clearRoute(box, false, {100.5, 100.5, 11.08}, {106.5, 100.5, 12.08}, 2.88f, ledge), 1, "a one-block ledge");
        auto wall = [](int32_t x, int32_t, int32_t z) -> uint8_t { return z <= 9 || ((z == 10 || z == 11) && x >= 103) ? 1 : 0; };
        same(clearRoute(box, false, {100.5, 100.5, 11.08}, {106.5, 100.5, 13.08}, 2.88f, wall), 0, "a two-block wall is not");
        // §5: the search on flat ground from (0, 0, 10) to (3, 2, 10).
        PathSearch search;
        search.start({0.5, 0.5, 10.1}, {3.5, 2.5, 10.1}, 2);
        search.round(box, flat);
        same(search.path.size() == 2 && search.path[1] == PathSearch::Cell{1, 1, 10}, 1, "round 1: best (1, 1)");
        search.round(box, flat);
        same(search.path.back() == PathSearch::Cell{2, 2, 10}, 1, "round 2: best (2, 2)");
        search.round(box, flat);
        same(search.path.back() == PathSearch::Cell{3, 2, 10}, 1, "round 3: the goal");
        // §6: a creature follows a search around a wall to its goal.
        auto blocked = [](int32_t x, int32_t y, int32_t z) -> uint8_t { return z <= 9 || (x == 3 && y >= -4 && y <= 4 && z <= 12) ? 1 : 0; };
        Body walker;
        walker.place({0.5, 0.5, 10 + 1.08 + 0.01});
        PathSearch route;
        route.start({0.5, 0.5, 10.1}, {6.5, 0.5, 10.1}, 1);
        bool arrived = false;
        for (int i = 0; i < 1000 && !arrived; i++) {
            route.rounds(walker.size, 1, blocked);
            route.follow(walker, 16, blocked);
            route.timerMs += 16;
            worldStep(walker, 16, blocked);
            arrived = std::abs(walker.blocks().x - 6.5) < 1.5 && std::abs(walker.blocks().y - 0.5) < 1.5;
        }
        same(arrived, 1, "around the wall to the goal");
    }

    // Wildlife §4.6: a group leader walks its route by path search and stands at the point.
    // These behavior fixtures use zone1 so their spawn key is nonzero. A key-zero
    // actor is a valid preliminary target, tested separately in TargetSelectionTests.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 276ll << 16, s.y = 20ll << 16, s.z = 10ll << 16;
        s.type = 2;
        s.controller = 3;
        s.behaviour = 2;
        s.route = {{336, 20, 10}};
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({276.5, 150.5, 10 + 1.08 + 0.01}); // within 200 to wake it, beyond 8 to be ignored
        Npcs npcs;
        double closest = 1e9;
        for (int i = 0; i < 1500; i++) { // 24 s
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, flat);
            if (!npcs.all().empty())
                closest = std::min(closest, std::abs(npcs.all()[0].body.blocks().x - 336.0));
        }
        same(closest < 10, 1, "the leader reaches within 10 blocks of its route point");
    }

    // Schedules §3: a resident appears at its current stop and walks to the next when its time
    // comes, arriving within 4 blocks.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 276ll << 16, s.y = 20ll << 16, s.z = 10ll << 16;
        s.type = 2;
        s.controller = 3;
        s.characterClass = 0x88;
        s.schedule = {{276ll << 16, 20ll << 16, 10ll << 16, 0}, {316ll << 16, 20ll << 16, 10ll << 16, 10 * 3600000}};
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({296.5, 150.5, 10 + 1.08 + 0.01});
        Npcs npcs;
        npcs.step(16, map, {{me, body, 0, me.actionMs}}, 9 * 3600000.0, 0, flat);
        same(!npcs.all().empty() && std::abs(npcs.all()[0].body.blocks().x - 276.5) < 1, 1, "at home before ten");
        double closest = 1e9;
        for (int i = 0; i < 1500; i++) {
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 11 * 3600000.0, 0, flat);
            if (!npcs.all().empty())
                closest = std::min(closest, std::abs(npcs.all()[0].body.blocks().x - 316.0));
        }
        same(closest < 5, 1, "walked to the ten o'clock stop");
    }

    // Trails spec §4: a resident at its stop walks to a free bench in sight, sits about 20 s,
    // then stands up.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 276ll << 16, s.y = 20ll << 16, s.z = 10ll << 16;
        s.type = 2;
        s.controller = 3;
        s.characterClass = 0x88;
        s.schedule = {{276ll << 16, 20ll << 16, 10ll << 16, 0}};
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({296.5, 150.5, 10 + 1.08 + 0.01});
        Npcs npcs;
        bool taken = false;
        int sat = 0;
        npcs.seats = [](glm::ivec3, double) {
            Npcs::Seats out;
            out.keep = out.pick = {{1, 0, 3, {282.0, 21.5, 10.0}}};
            return out;
        };
        npcs.seatFree = [&](const Seat &) { return !taken; };
        npcs.sit = [&](Npc &n, const Seat &) {
            taken = true;
            n.fighter.action = 83;
            sat++;
        };
        bool satDown = false, stoodUp = false;
        for (int i = 0; i < 2500; i++) { // 40 s at noon
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 12 * 3600000.0, 0, flat);
            if (npcs.all().empty())
                continue;
            const int32_t action = npcs.all()[0].fighter.action;
            satDown = satDown || action == 83;
            stoodUp = stoodUp || (satDown && action == 0);
        }
        same(satDown, 1, "the resident sat on the bench");
        same(stoodUp, 1, "and stood up after its 20 s");
        same(sat, 1, "it sat once (20 s of rest after standing)");
    }

    // Materials §1.1, §1.6 example 4: ordinary cube colours, and fire and ice average to a pale lilac.
    {
        same(spiritColour(1).x, 0.7, "iron", 1e-6);
        same(spiritColour(10).y, 0.5, "copper is half the input", 1e-6);
        Item sword;
        sword.type = 3;
        sword.cubes = {{0, 0, 0, 128}, {0, 0, 1, 130}, {0, 0, 2, 1}};
        sword.spirits = 3;
        glm::vec3 a(0);
        same(averageSpiritColour(sword, a), 1, "a sword with spirits");
        same(a.x, 0.975, "average r", 1e-5);
        same(a.y, 0.75, "average g", 1e-5);
        same(a.z, 0.825, "average b", 1e-5);
    }
    // Behaviour §12.2 example: one unholy spirit of level 21 (c = 0.5) at full combo: damage
    // + 0.025, life steal 0.1; an ice spirit slows the target.
    {
        Combatant attacker, target;
        attacker.id = 1, target.id = 2;
        attacker.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        attacker.equipment[MAIN_HAND].cubes = {{0, 0, 0, 0x81, 21}};
        attacker.equipment[MAIN_HAND].spirits = 1;
        attacker.combo = (int32_t)std::ceil(comboMax(attacker));
        target.health = maxHealth(target);
        auto hitWith = [&](bool area) {
            Combatant t = target;
            glm::vec3 v(0);
            HitInput in;
            in.amount = 10;
            in.area = area;
            return std::make_pair(resolveHit(t, v, &attacker, in, [] { return 16384; }), t);
        };
        const auto plain = hitWith(false), spirited = hitWith(true);
        same(spirited.first.records.empty() ? 0 : spirited.first.records.back().amount - plain.first.records.back().amount, 0.025,
             "unholy damage", 1e-4);
        same(spirited.first.records.size() == 2 ? spirited.first.records[0].amount : 0, -0.1,
             "source/self healing record", 1e-5);
        attacker.equipment[MAIN_HAND].cubes = {{0, 0, 0, 0x82, 21}};
        same(hitWith(true).second.slowMs, 500, "ice slows for 500 ms");
    }
    // Pets §7: a collie (type 19) walks up to a player holding out collie food and is tamed by
    // its fifth bite.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 19;
        s.level = 4;
        s.controller = 1;
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.level = 3;
        me.skills[0] = 2;
        me.health = maxHealth(me);
        me.equipment[12].type = 20, me.equipment[12].subtype = 19;
        me.action = 82;
        Body body;
        body.place({274.5, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.setNamedWorld(true);
        int bites = 0;
        bool tamed = false;
        for (int i = 0; i < 1500 && !tamed; i++) { // up to 24 s
            const NpcFrame f = npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
            for (const HitEmission &h : f.playerHits)
                bites += h.record.kind == 6;
            for (const auto &t : f.tamed) {
                tamed = true;
                same(t.owner, 1, "tamed for the player");
                same(t.level, 3, "the level capped at the owner's");
            }
        }
        same(tamed, 1, "the collie is tamed");
        same(bites, 5, "by five bites");
        const Npc *pet = npcs.petOf(me);
        same(pet && pet->fighter.controller == 5 && pet->fighter.ownerId == 1, 1, "a pet of the player");
        same(pet ? pet->fighter.multipliers[0] : 0, 100 * (1 + 0.5f * skillFactor(2)), "health multiplier 108.33", 1e-3);
    }
    // Pets §9.3, §10.2, §13, §11.1: a pet item in the slot makes a pet at the player that follows,
    // comes when called from afar, and returns 20 s after dying.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.level = 5;
        me.health = maxHealth(me);
        me.equipment[12].type = 19, me.equipment[12].subtype = 19, me.equipment[12].level = 3;
        Body body;
        body.place({30.5, 30.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.setNamedWorld(true);
        auto step = [&] { return npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor); };
        step();
        const Npc *pet = npcs.petOf(me);
        same(pet ? (int64_t)pet->fighter.id : 0, -1, "the pet's id");
        body.place({50.5, 30.5, 1 + 1.08 + 0.01});
        for (int i = 0; i < 300; i++)
            step();
        pet = npcs.petOf(me);
        const double gap = pet ? glm::length(glm::dvec2(pet->body.blocks() - body.blocks())) : 99;
        same(gap < 4.5, 1, "it follows to within 4 blocks");
        body.place({300.5, 30.5, 1 + 1.08 + 0.01});
        npcs.callPet(me, body.blocks());
        pet = npcs.petOf(me);
        same(pet && glm::length(glm::dvec2(pet->body.blocks() - body.blocks())) < 0.01, 1, "T brings it from 270 blocks");
        npcs.npcs()[0].fighter.health = 0;
        for (int i = 0; i < 1300; i++) // 20.8 s
            step();
        pet = npcs.petOf(me);
        same(pet && pet->fighter.health == maxHealth(pet->fighter), 1, "back after 20 s at full health");
        me.equipment[12] = Item{};
        step();
        step();
        same(npcs.petOf(me) == nullptr, 1, "gone when the slot is emptied");
    }

    // Behaviour §13.2 step 3: below a quarter of its health a creature drinks a potion it carries.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::Spawn s;
        s.x = 10ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 4;
        worldgen::ItemRecord potion;
        potion.type = 1, potion.subtype = 1, potion.level = 1;
        s.inventory.push_back(potion);
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({25.5, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
        Npc &hurt = npcs.npcs()[0];
        hurt.fighter.health = 0.2f * maxHealth(hurt.fighter);
        for (int i = 0; i < 4; i++)
            npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
        same(npcs.all()[0].fighter.action, 80, "it drinks");
        same(npcs.all()[0].inventory.size() == 1 && itemType(npcs.all()[0].inventory[0]) == 0 &&
             itemSubtype(npcs.all()[0].inventory[0]) == 0, 1, "the potion is used up without erasing its payload row");
    }

    // Behaviour §9, §13: a creature wakes near the player, hunts, fights, dies and returns.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16;
        s.y = 10ll << 16;
        s.z = 1ll << 16;
        s.type = 4;
        s.level = 1;
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({281.5, 10.5, 1 + 1.08 + 0.01}); // 15 blocks away, as in §13.2's example
        Npcs npcs;
        auto step = [&](float ms) {
            return npcs.step(ms, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
        };
        step(16);
        same(npcs.all().size(), 1, "the spawn woke within 200 blocks");
        same(npcs.all()[0].fighter.health, maxHealth(npcs.all()[0].fighter), "full health");
        int hits = 0;
        Projectiles shots; // a ranged weapon's hits travel as projectiles
        for (int i = 0; i < 600; i++) { // 10 s
            const NpcFrame f = step(16);
            hits += (int)f.playerHits.size();
            shots.list.insert(shots.list.end(), f.shots.begin(), f.shots.end());
            ShotEffects fx;
            std::vector<ShotTarget> targets{{&me, &body}};
            for (Npc &n : npcs.npcs())
                targets.push_back({&n.fighter, &n.body});
            shots.step(16, targets, nullptr, floor, [] { return 0; }, fx);
            hits += (int)fx.hits.size();
        }
        same(hits > 0, 1, "it fought the player (close up or from afar, by its weapon)");
        // The player kills it.
        Npc &it = npcs.npcs()[0];
        it.fighter.health = 0.001f;
        it.target = me.id;
        // Step up beside it with a sword (bare hands do 1, which its armour stops), face it and
        // swing until it falls.
        me.equipment[MAIN_HAND] = {3, 0, 1, 0, 1, 0};
        body.place(it.body.blocks() + glm::dvec3(1.2, 0, 0));
        const glm::dvec3 to = it.body.blocks() - body.blocks();
        const float yaw = (float)(std::atan2(-to.x, to.y) * 180.0 / 3.141592653589793);
        int32_t experience = 0, deferredExperience = 0;
        size_t deaths = 0;
        for (int i = 0; i < 200 && !npcs.all().empty(); i++) {
            const int32_t before = me.actionMs;
            startPrimaryAttack(me);
            stepAction(me, 16);
            const NpcFrame f = npcs.step(16, map, {{me, body, yaw, before, true, false, true}}, 43200000, 0, floor);
            for (const auto &credit : f.kills)
                if (credit.player == me.id)
                    experience += credit.experience;
            deferredExperience += f.experience;
            deaths += f.deaths.size();
        }
        same(npcs.all().empty(), 1, "killed");
        same(deaths, 1, "its death is reported once, for its loot");
        same(experience, 1, "a level 1 kill is worth 1");
        same(me.experience == 1 && deferredExperience == 0, 1, "the controlled recipient receives the credit immediately without a deferred duplicate");
        step(60000);
        same(npcs.all().empty(), 1, "not back the same day within 20 minutes");
        step(1200000);
        step(16);
        same(npcs.all().size(), 1, "back after 20 minutes");

        // Server simulation §5.3: spawn ids (the worked example).
        same(Npcs::spawnId(32806, 32800, 3) == 550301083139ull, 1, "spawn id example");
        // §6.1, §6.2: on the server a player's hit arrives as a record; the kill is credited
        // through the threat map, and the body goes 1000 ms later.
        Combatant other;
        other.id = 2;
        other.health = maxHealth(other);
        Body otherBody;
        otherBody.place(body.blocks() + glm::dvec3(0, 40, 0));
        Npc &again = npcs.npcs()[0];
        HitRecord blow;
        blow.attacker = other.id;
        blow.target = again.fighter.id;
        blow.amount = again.fighter.health + 100;
        PlayerView a{me, body, 0, me.actionMs, false}, b{other, otherBody, 0, other.actionMs, false};
        const NpcFrame server = npcs.step(16, map, {a, b}, 43200000, 1, floor, {blow});
        // (Player 1, whom it was fighting, is in its threat map too.)
        same(std::any_of(server.kills.begin(), server.kills.end(),
                         [&](const KillRecord &k) { return k.player == 2 && k.victim == blow.target; }),
             1, "the hitting player is credited");
        same(npcs.all().size(), 1, "the body stays a moment");
        for (int i = 0; i < 70; i++)
            npcs.step(16, map, {a, b}, 43200000, 1, floor);
        same(npcs.all().empty(), 1, "and goes after a second");
        // §5.4: with every player beyond radius + 20 the creature goes.
        step(1200000);
        step(16);
        same(npcs.all().size(), 1, "back again");
        body.place(npcs.all()[0].home + glm::dvec3(230, 0, 2));
        step(16);
        same(npcs.all().size(), 1, "active acquired threat retains the creature within 512 blocks of home");
        // Noncombat despawn: acquisition now publishes the native threat slot.
        // End that supplied relationship before checking the independent radius.
        npcs.npcs()[0].fighter.relations.threats.clear();
        npcs.npcs()[0].target = 0;
        step(16);
        same(npcs.all().empty(), 1, "despawned beyond 220 blocks");
    }

    // Behaviour §16.9: the worked examples of the ability bar.
    {
        Combatant w;
        w.characterClass = 1, w.specialization = 0, w.level = 5;
        w.equipment[MAIN_HAND] = Item{3, 0, 1, 0, 5, 0};
        w.skills[6] = 2, w.skills[7] = 0, w.skills[8] = 1;
        const auto slots = abilitySlots(w);
        same(slots[1] == 63 && slots[2] == 54 && slots[3] == 0 && slots[4] == 102 && slots[5] == 0, 1, "warrior slots");
        Body body;
        useAbilities(w, body, {false, false, true, false, false, false}, glm::vec3(6, 8, 0));
        same(w.action, 54, "key 1 smashes");
        same(body.velocity.x, 12, "smash velocity x");
        same(body.velocity.y, 16, "smash velocity y");
        same(body.velocity.z, 15, "smash velocity z");
        stepAction(w, 16);
        same(w.cooldowns[54], 17666, "smash cooldown, recorded as it starts");
        same(usableNow(w, 54), 0, "smash waits for its cooldown");
        useAbilities(w, body, {false, false, false, false, true, false}, glm::vec3(0));
        same(w.cooldowns[102], 57272, "warfrenzy cooldown");
        same(w.hasBuff(2), 1, "warfrenzy buff");

        Combatant m;
        m.characterClass = 3, m.specialization = 1, m.level = 2;
        m.equipment[MAIN_HAND] = Item{3, 11, 2, 0, 2, 0};
        m.skills[6] = 2, m.skills[7] = 1, m.skills[8] = 0;
        const auto ms = abilitySlots(m);
        same(ms[0] == 44 && ms[1] == 45 && ms[2] == 34 && ms[3] == 103 && ms[4] == 0, 1, "mage slots");
        same(manaCost(m, 34), 0.109375, "healing stream cost", 1e-6);

        Combatant r;
        r.characterClass = 2, r.level = 1;
        r.equipment[MAIN_HAND] = Item{3, 6, 2, 0, 1, 0};
        const auto rs = abilitySlots(r);
        same(rs[0] == 22 && rs[1] == 25 && rs[2] == 0 && rs[3] == 0 && rs[4] == 0, 1, "ranger slots");
    }

    // Screens §5.4.1: species keys, generated titles, peaceful animals and the level curve.
    same(std::string(speciesKey(4)) == "GoblinMale", 1, "type 4 is a goblin");
    same(speciesKey(95) == nullptr && speciesKey(129) == nullptr, 1, "types 95 and 129 have no key");
    same(std::string(speciesKey(155)) == "Blowfish", 1, "type 155");
    same(generatedTitle(1000, 4) == "Urzic", 1, "boss goblin title");
    same(generatedTitle(123456, 44) == "Kurrior", 1, "bandit NPC title");
    same(peacefulAnimal(56, 0), 1, "a chicken is peaceful");
    same(peacefulAnimal(55, 0x200), 0, "a boss crow is not");
    same(levelCurve(10), 0.31034, "p(10)", 1e-5);
    same(levelCurve(20), 0.48718, "p(20)", 1e-5);

    // Screens §8.1.1: power, price, bags, stacking, money and the carry limit.
    {
        same(powerOf(1), 1, "power at level 1");
        same(powerOf(5), 17, "power at level 5");
        same(powerOf(10), 32, "power at level 10");
        const auto make = [](uint8_t type, uint8_t sub, int16_t level) {
            ItemRecord r{};
            r[0] = type, r[1] = sub;
            setItemLevel(r, level);
            return r;
        };
        ItemRecord sword = make(3, 0, 5);
        same(price(sword), 28, "iron sword price");
        same(bagOf(sword), 0, "a weapon goes to Equipment");
        same(bagOf(make(11, 0, 1)), 2, "ingredients");
        same(bagOf(make(20, 0, 1)), 3, "pet food");
        same(bagOf(make(1, 0, 1)), 1, "a potion goes to Items");
        same(twoHanded(make(3, 15, 1)), 1, "a greatsword is two-handed");
        same(canUse(sword, 5, 1), 1, "a warrior may use a sword");
        same(canUse(sword, 5, 3), 0, "a mage may not");
        same(canUse(sword, 4, 1), 0, "nor a warrior below its power");

        Inventory inv = Inventory::fresh();
        same(inv.bags.size() == 4 && inv.bags[1].size() == 50, 1, "four bags of fifty");
        const auto coins = [&](uint8_t kind, int16_t n) {
            ItemRecord r = make(12, 0, n);
            r[0x0d] = kind;
            return r;
        };
        inv.add(coins(11, 2)); // two gold coins
        inv.add(coins(12, 3)); // three silver
        inv.add(coins(10, 4));
        same(inv.money, 20304, "coins become money");
        const ItemRecord herb = make(11, 3, 7); // seven of an ingredient
        inv.add(herb);
        inv.add(make(11, 3, 2));
        same(inv.bags[2][0].count, 9, "ingredients pile up");
        same(itemLevel(inv.bags[2][0].item), 1, "the stored quantity field is 1");
        same(inv.canCarry(herb), 1, "nine may take more");
        inv.bags[2][0].count = 50;
        same(inv.canCarry(herb), 0, "fifty is the limit");
        inv.add(sword);
        inv.add(sword);
        same(inv.bags[0][0].count == 1 && inv.bags[0][1].count == 1, 1, "weapons do not stack");

        // §8.1.8: pick a sword up, refused in another tab, equip it from the hand.
        same(inv.pressSlot(0, 0), SOUND_GRAB, "picking up plays the grab sound");
        same(inv.pressSlot(1, 0), SOUND_REFUSED, "a weapon may not go in Items");
        same(inv.pressBox(1, 5, 1), SOUND_PUT_DOWN, "the right weapon box takes it");
        same(itemType(inv.equipment[MAIN_HAND]) == 3 && inv.hand.empty(), 1, "equipped from the hand");
        // Right-click a greatsword: it goes in the right hand, the sword back to the bag.
        inv.bags[0][5] = {1, make(3, 15, 1)};
        const UseOutcome u = inv.useSlot(0, 5, false, 5, 1, true);
        same(u.sound, SOUND_PUT_DOWN, "equipping plays the put-down sound");
        same(itemSubtype(inv.equipment[MAIN_HAND]), 15, "the greatsword is in the right hand");
        same(inv.bags[0][0].count, 1, "the sword went back to the first empty slot");
        same(inv.unequip(1), SOUND_PUT_DOWN, "unequip");
        same(itemType(inv.equipment[MAIN_HAND]), 0, "the right hand is empty");
        // A potion is used from the stack.
        inv.add(make(1, 1, 1));
        inv.add(make(1, 1, 1));
        const auto quick = inv.quickItems(1);
        same(quick.size(), 1, "one quick item");
        const UseOutcome drink = inv.useSlot(quick[0].first, quick[0].second, false, 1, 1, true);
        same(drink.consume.has_value() && inv.bags[1][0].count == 1, 1, "a potion is taken to drink");
        // Item names §9: a level-5 life potion heals 282.84 over 3 s in 15 ticks.
        Combatant drinker;
        drinker.level = 50;
        drinker.health = 100;
        drinker.action = 80;
        drinker.held = Item::parse(make(1, 1, 5).data());
        same(itemHeal(drinker.held), 282.842712, "life potion heal", 1e-3);
        for (int i = 0; i < 400; i++)
            stepAction(drinker, 10);
        same(drinker.health, 100 + 282.842712, "healed over the potion's 3 s", 1e-2);
    }

    // World items §3: a drop falls to the ground, is picked up within 4 blocks, and old drops
    // are left behind when a zone is loaded again.
    {
        worldgen::Zone zone;
        zone.zx = 1, zone.zy = 2;
        worldgen::GroundItem mushroom{300, 520, 10, 11, 15, 45, 1};
        zone.items.push_back(mushroom);
        WorldItems world;
        world.generated(zone);
        const std::vector<WorldItem> *list = world.zone(1, 2);
        same(list && list->size() == 1, 1, "the generated mushroom");
        same((*list)[0].blocks().x, 300.5, "on its column's centre");
        same((*list)[0].scale, 1.0 / 14, "at the default scale", 1e-7);
        const BlockType ground = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z < 10 ? 1 : 0); };
        ItemRecord sword{};
        sword[0] = 3;
        same(world.drop(sword, {310.2, 530.7, 25.4}, 1, 7, ground, [] { return 100; }), 1, "dropped in a held zone");
        same(world.drop(sword, {0, 0, 25}, 1, 7, ground, [] { return 100; }), 0, "lost outside the held zones");
        same((*list)[1].blocks().z, 10, "it fell to the ground");
        same((*list)[1].landingMs, 600, "its landing timer");
        same(world.takeChanged().size(), 1, "the zone changed");
        std::vector<SoundEvent> sounds;
        world.step(600, true, ground, [] { return 0; }, sounds);
        same(sounds.size() == 1 && sounds[0].sound == 59, 1, "it lands with drop-item.wav");
        same(world.pickUp(1, 2, 1, {320, 530, 10}).has_value(), 0, "too far to pick up");
        same(world.pickUp(1, 2, 1, {311, 531, 10}).has_value(), 1, "picked up");
        same(list->size(), 1, "gone from the zone");
        const WorldItem back = WorldItem::fromRecord((*list)[0].record().data());
        same(back == (*list)[0], 1, "the network record round-trips");
        std::vector<std::array<uint8_t, 325>> saved;
        WorldItem old = (*list)[0];
        old.day = 3;
        saved.push_back(old.save());
        saved.push_back((*list)[0].save());
        world.restore(1, 2, saved, 7);
        same(world.zone(1, 2)->size(), 1, "a drop older than three days is left out");
    }

    // Interaction §8 examples 5 and 6: a chest opened spills its loot; a door toggles and
    // swings over a second; a stool seats the player and frees when they walk off.
    {
        worldgen::Zone zone;
        zone.zx = 0, zone.zy = 0;
        worldgen::StaticObject chest;
        chest.kind = 10, chest.x = 50ll << 16, chest.y = 50ll << 16, chest.z = 10ll << 16, chest.state = 2;
        chest.contents.resize(3);
        for (auto &item : chest.contents)
            item.type = 11, item.level = 1;
        worldgen::StaticObject door;
        door.kind = 1, door.x = 60ll << 16, door.y = 50ll << 16, door.z = 10ll << 16, door.state = 1;
        worldgen::StaticObject stool;
        stool.kind = 16, stool.x = 70ll << 16, stool.y = 50ll << 16, stool.z = 100ll << 16, stool.rotation = 1;
        stool.scale[2] = 0.5f;
        zone.statics = {chest, door, stool};
        WorldItems items;
        items.generated(zone);
        StaticStates statics;
        statics.generated(zone);
        const BlockType ground = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z < 10 ? 1 : 0); };
        Combatant me;
        me.id = 1;
        Body body;
        body.size.z = 1.8f;
        float yaw = 0;
        std::vector<SoundEvent> sounds;
        const auto use = [&](int32_t k) {
            sounds.clear();
            statics.use(zone, k, me, body, yaw, items, 0, ground, [] { return 16383; }, sounds);
        };
        use(0);
        same(items.zone(0, 0)->size(), 3, "the chest's three items spill");
        same((*statics.zone(0, 0))[0].state, 0, "the chest is open");
        same(sounds.size() == 1 && sounds[0].sound == 54, 1, "with sound 54");
        // (A door closed for 25 s: the setter's 1000 - timer rule gives 0, §8 example 2.)
        statics.step(25000, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &) { return false; }, {});
        use(1);
        same((*statics.zone(0, 0))[1].state, 0, "the door opens");
        (void)statics.takeChanged();
        statics.step(500, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &) { return false; }, {});
        same(openFraction(1, (*statics.zone(0, 0))[1]), 0.5, "half open after half a second", 1e-6);
        use(2);
        same(body.blocks().z, 101.4, "seated on the stool", 1e-4);
        same(yaw, 270, "facing (1 + 2) x 90");
        same(me.action, 83, "sitting");
        std::map<StaticStates::Key, const worldgen::Zone *> held{{{0, 0}, &zone}};
        const auto where = [&](uint64_t, glm::dvec3 &at, glm::vec3 &intent, int32_t &action) {
            at = body.blocks(), intent = body.intent, action = me.action;
            return true;
        };
        statics.step(16, where, held);
        same((*statics.zone(0, 0))[2].occupant, 1, "still seated");
        body.intent = {1, 0, 0};
        statics.step(16, where, held);
        same((*statics.zone(0, 0))[2].occupant, 0, "walking off frees the stool");
    }

    // Loot rolls §1, §2 and §5: the rolls' vectors and the goblin's two worked streams.
    {
        worldgen::Random r123(123), r42(42);
        same(rarityRoll(r123, 2), 2, "rho(2) from srand(123)");
        same(r123.count(), 4, "four draws");
        same(rarityRoll(r42, 3), 4, "rho(3) from srand(42), a bonus");
        for (const auto &[n, want] : std::initializer_list<std::pair<int16_t, int16_t>>{{1, 1}, {2, 1}, {3, 1}, {5, 3}, {10, 8}, {20, 24}, {50, 63}, {100, 117}}) {
            worldgen::Random r(123);
            same(rerollLevel(r, n), want, "re-roll of level " + std::to_string(n));
        }
        LootVictim goblin;
        goblin.controller = 1, goblin.type = 4, goblin.level = 5;
        goblin.position[0] = 100ll << 16, goblin.position[1] = 100ll << 16, goblin.position[2] = 20ll << 16;
        goblin.equipment[6][0] = 3, goblin.equipment[6][1] = 13;
        goblin.equipment[7][0] = 3;
        goblin.equipment[2][0] = 4;
        ItemRecord potion{}, feather{};
        potion[0] = 1, potion[1] = 1;
        feather[0] = 11, feather[1] = 2;
        goblin.inventory = {{2, potion}, {1, feather}};
        const BlockType ground = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z < 10 ? 1 : 0); };
        worldgen::Zone zone;
        {
            WorldItems items;
            items.generated(zone); // zone (0, 0), held
            worldgen::Random r(123);
            dropLoot(goblin, items, 0, ground, r);
            same(r.count(), 16, "srand(123): 16 draws");
            const auto &list = *items.zone(0, 0);
            same(list.size(), 2, "16 copper and the feather");
            if (list.size() == 2) {
                same(itemType(list[0].item) == 12 && itemMaterial(list[0].item) == 10 && itemLevel(list[0].item) == 16, 1,
                     "16 copper");
                same(list[0].landingMs, 664, "its landing timer");
                same(list[0].angle, 224.45, "its angle", 0.01);
                same(list[0].position[0] - goblin.position[0], -57463, "its offset in x");
                same(itemType(list[1].item), 11, "the feather");
            }
        }
        {
            WorldItems items;
            items.generated(zone);
            worldgen::Random r(170);
            dropLoot(goblin, items, 0, ground, r);
            same(r.count(), 61, "srand(170): 61 draws");
            const auto &list = *items.zone(0, 0);
            same(list.size(), 5, "five items");
            if (list.size() == 5) {
                same(itemType(list[0].item), 4, "the goblin's chest");
                same(itemType(list[1].item) == 3 && itemSubtype(list[1].item) == 2 && itemLevel(list[1].item) == 2, 1,
                     "a level-2 weapon of subtype 2");
                same(itemType(list[2].item) == 20 && itemSubtype(list[2].item) == 63, 1, "PlainRunner pet food");
                same(itemLevel(list[3].item), 19, "19 copper");
            }
        }
    }

    // Crafting §7: a new mage's grants, a muffin at a campfire, and a sword from a formula.
    {
        std::vector<ItemRecord> known;
        grantLevel(known, 1, 3, [] { return 4; });
        same(known.size(), 12, "a level-1 mage's twelve recipes");
        const ItemRecord &wand = known.back();
        same(itemType(wand) == 3 && itemSubtype(wand) == 11 && itemMaterial(wand) == 2, 1, "a wood wand");
        const auto tabs = recipeTabs(known);
        same(tabs[0].size() == 1 && tabs[1].size() == 1 && tabs[3].size() == 2 && tabs[4].size() == 1 && tabs[5].size() == 7, 1,
             "the recipes sorted into their tabs");

        Inventory inv = Inventory::fresh();
        ItemRecord pumpkin{};
        pumpkin[0] = 11, pumpkin[1] = 16;
        creatures::setItemLevel(pumpkin, 3);
        inv.add(pumpkin);
        ItemRecord muffinKey{};
        muffinKey[0] = 1, muffinKey[1] = 9;
        creatures::setItemLevel(muffinKey, 2);
        const Recipe muffin = buildRecipe(muffinKey);
        same(muffin.station, CAMPFIRE_STATION, "muffins need a campfire");
        same(timesCraftable(inv, muffin), 3, "three pumpkins make three");
        const CraftOutcome made = craft(inv, muffin, [] { return 1; });
        same(made.made.has_value() && made.sounds.size() == 1 && made.sounds[0] == 0x59, 1, "a muffin, with craft.wav");
        same(timesCraftable(inv, muffin), 2, "two pumpkins left");

        // A rarity-2 formula: 8 iron cubes and 2 sapphire nuggets of rarity 2.
        ItemRecord formula{};
        formula[0] = 2, formula[8] = 3, formula[0x0c] = 2, formula[0x0d] = 1;
        uint32_t seed = 37;
        std::memcpy(&formula[4], &seed, 4);
        creatures::setItemLevel(formula, 6);
        inv.bags[1][5] = {1, formula};
        same(inv.useSlot(1, 5, false, 10, 1, true).sound, SOUND_RECIPE_LEARNT, "the formula is learnt");
        const ItemRecord &swordKey = inv.recipes.back();
        same(itemType(swordKey) == 3 && itemLevel(swordKey) == 6 && swordKey[8] == 0, 1, "its key is the sword");
        const Recipe sword = buildRecipe(swordKey);
        same(sword.ingredients.size() == 2 && sword.ingredients[0].second == 8 && sword.ingredients[1].second == 2 &&
                 itemMaterial(sword.ingredients[1].first) == 14 && itemRarity(sword.ingredients[1].first) == 2,
             1, "8 iron cubes and 2 sapphire nuggets");
        same(sword.station, ANVIL, "at an anvil");
    }

    // Item services §1.6, §2.4: identifying Leftovers, and both adaption costs.
    {
        ItemRecord leftovers{};
        leftovers[0] = 14, leftovers[0x0c] = 1;
        creatures::setItemLevel(leftovers, 5);
        same(identificationFee(leftovers) >= 1, 1, "the identifier's fee is at least 1 copper");
        worldgen::Random r(123);
        const ItemRecord made = identify(leftovers, r);
        same(r.count() == 17 || r.count() == 25, 1, "17 or 25 draws");
        same(itemLevel(made) == 5 && itemRarity(made) == 1 && itemType(made) >= 3 && itemType(made) <= 9, 1,
             "a weapon or armour piece at the Leftovers' level and rarity");
        same(adaptionCost(5, 1, 8), 12, "vanilla: level 5 rarity 1 to level 8");
        same(adaptionCost(10, 0, 5), 0, "nothing to pay for an item above the player");
        // The rebalanced curve (coremaze's Adaption Rebalance), worked from its formula.
        same(rebalancedAdaptionCost(10, 0, 20), 69, "rebalanced: 10 -> 20");
        same(rebalancedAdaptionCost(19, 3, 20), 13, "rebalanced: 19 rarity 3 -> 20");
        same(rebalancedAdaptionCost(40, 4, 50), 61, "rebalanced: 40 rarity 4 -> 50");
        same(adaptionCost(40, 4, 50) > 30000, 1, "where vanilla asks tens of thousands");
    }

    // Skills §8.1, §8.2: a level-10 warrior's points, and a ranger's change to Sniper.
    {
        SkillLevels draft{};
        const int32_t available = skillPoints(10, 5);
        same(available, 19, "2·10 − 2 + 5/4 points");
        same(raiseSkill(draft, 7, available), 0, "cyclone is locked under smash 5");
        for (int k = 0; k < 5; k++)
            raiseSkill(draft, 6, available);
        for (int k = 0; k < 5; k++)
            raiseSkill(draft, 7, available);
        for (int k = 0; k < 5; k++)
            raiseSkill(draft, 8, available);
        for (int k = 0; k < 4; k++)
            raiseSkill(draft, 2, available);
        same(pointsUsed(draft), 19, "all nineteen spent");
        same(raiseSkill(draft, 0, available), 0, "none left for pet taming");
        const SkillLevels committed = draft;
        same(lowerSkill(draft, 6), 0, "smash may not drop under cyclone");
        same(lowerSkill(draft, 8), 1, "war frenzy drops to 4");
        same(skillChangeCost(10, committed, 0, draft, 0), 19, "a re-skill point at level 10");
        same(skillChangeCost(10, committed, 0, draft, 1), 209, "and the change to Guardian");
        same(skillAbility(8, 1, 1), 101, "Guardian's skill 8 is bulwark");
        SkillLevels ranger{};
        ranger[6] = 3, ranger[7] = 2, ranger[8] = 1;
        same(skillChangeCost(6, ranger, 1, ranger, 0), 151, "Scout to Sniper at level 6");
        same(skillChangeCost(1, ranger, 1, ranger, 0), 0, "free at level 1");
    }

    // Character screens §3.9, §3.10: types, face and haircut counts, and a new rogue's kit.
    {
        same(creatureTypeOf(7, false), 13, "frogman male");
        same(faceCount(13) == 5 && haircutCount(13) == 5, 1, "five faces and haircuts");
        same(faceCount(0), 4, "an elf male's four faces");
        creatures::CharacterChoice choice;
        const CharacterRecord rogue = newCharacter("Al", choice, 4, [] { return 0; });
        same(itemSubtype(rogue.equipment[MAIN_HAND]) == 3 && itemSubtype(rogue.equipment[OFF_HAND]) == 3, 1,
             "a dagger in each hand");
        same(itemMaterial(rogue.equipment[RING_LEFT]) == 12 && itemMaterial(rogue.equipment[RING_RIGHT]) == 11, 1,
             "a silver and a gold ring");
        same(rogue.tabs.size() == 4 && rogue.tabs[1][0].count == 5, 1, "five life potions in Items");
        same(itemSubtype(rogue.tabs[0][0].item) == 4 && itemSubtype(rogue.tabs[0][1].item) == 4 &&
                 itemSubtype(rogue.tabs[0][2].item) == 5, 1, "two fists and a longsword, a slot each");
        same(rogue.level, 1, "level 1");
        same(rogue.recipes.empty(), 0, "the level-1 recipes");
    }

    // Item services §3: cubes on a weapon, the capacity, and what the bench offers.
    {
        ItemRecord sword{};
        sword[0] = 3, sword[0x0d] = 1;
        creatures::setItemLevel(sword, 10);
        same(spiritCapacity(sword), 16, "a sword takes 16");
        sword[1] = 15;
        same(spiritCapacity(sword), 32, "a greatsword 32");
        sword[1] = 0;
        placeSpirit(sword, {1, 2, 3}, 128, 5);
        placeSpirit(sword, {1, 2, 4}, 130, 5);
        placeSpirit(sword, {1, 2, 3}, 131, 6); // the same cell: replaced
        same(spiritCount(sword), 2, "two cubes");
        same(spiritAt(sword, 0).material == 131 && spiritAt(sword, 0).level == 6, 1, "the first replaced");
        removeSpirit(sword, {1, 2, 3});
        same(spiritCount(sword) == 1 && spiritAt(sword, 0).cell == glm::ivec3(1, 2, 4), 1, "the later one moved down");
        same(Item::parse(sword.data()).spirits, 1, "the stats see one spirit");
        Inventory inv = Inventory::fresh();
        ItemRecord cube{};
        cube[0] = 11, cube[1] = 10, cube[0x0d] = 1;
        creatures::setItemLevel(cube, 3); // three iron cubes
        inv.add(cube);
        ItemRecord spirit{};
        spirit[0] = 11, spirit[1] = 14, spirit[0x0d] = 128;
        creatures::setItemLevel(spirit, 10);
        inv.add(spirit);
        ItemRecord strong = spirit;
        creatures::setItemLevel(strong, 40);
        inv.add(strong); // above the weapon's power: not offered
        const auto palette = spiritPalette(inv, sword);
        same(palette.size(), 2, "the iron cubes and the level-10 fire spirit");
        same(palette.size() == 2 && palette[0].count == 3, 1, "three iron cubes");
        takeSpiritItem(inv, palette[0].item);
        same(holding(inv, palette[0].item), 2, "one cube used");
    }

    // Projectiles §8: an uncharged arrow's flight and damage.
    {
        Combatant ranger;
        ranger.id = 7;
        ranger.characterClass = 2;
        ranger.equipment[MAIN_HAND] = {3, BOW, 1, 0, 1, 0};
        ranger.action = 22;
        Body body;
        const Phases ph = phases(ranger, 22);
        std::vector<Projectile> shots;
        ShotEffects fx;
        launch(ranger, body, {1, 0, 0}, (float)ph.windUp, 16, [] { return 0; }, shots, fx);
        same(shots.size(), 1, "one arrow at P1");
        same(itemDamage(ranger.mainHand()), 8, "a level-1 bow's damage");
        if (!shots.empty()) {
            same(shots[0].damage, 11.25, "arrow damage at the top of the spread", 1e-4);
            same(shots[0].velocity.x, 100, "arrow speed");
            same(shots[0].special, 0, "an uncharged arrow hits directly");
            const auto bytes = shots[0].serialize();
            same(Projectile::parse(bytes.data()).damage, 11.25, "the record round-trips", 1e-4);
            Projectiles world;
            world.list = shots;
            const BlockType air = [](int32_t, int32_t, int32_t) { return (uint8_t)0; };
            const int64_t want[3][2] = {{104856, -124}, {209712, -372}, {314568, -748}};
            for (int s = 0; s < 3; s++) {
                world.step(16, {}, nullptr, air, [] { return 0; }, fx);
                same((double)world.list[0].position[0], (double)want[s][0], "arrow x after step " + std::to_string(s + 1));
                same((double)world.list[0].position[2], (double)want[s][1], "arrow z after step " + std::to_string(s + 1));
            }
            for (int s = 3; s < 313; s++)
                world.step(16, {}, nullptr, air, [] { return 0; }, fx);
            same(world.list.size(), 0, "the arrow times out after 313 steps");
        }
    }

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
