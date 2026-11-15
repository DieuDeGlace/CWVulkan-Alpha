// Checks the creature code against the worked examples and test vectors of
// docs/specs/alpha-creatures-models.md and alpha-physics-camera.md.
#include "GameOptions.h"
#include "ItemNames.h"
#include "Loot.h"
#include "Quests.h"
#include "NewCharacter.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "creatures/Strike.h"
#include "creatures/Abilities.h"
#include "creatures/Airships.h"
#include "creatures/AngleStep.h"
#include "net/RemoteCreatures.h"
#include "creatures/Appearance.h"
#include "creatures/Combat.h"
#include "creatures/ControllerAim.h"
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
#include "worldgen/Items.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <optional>
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

    // Spawn objects §2: the constructor's 13 empty items carry level 1, which a kept-gear
    // (flag 0x40) spawn retains through the finishing pass: 5 appearance draws, no equipment
    // draws, 28 rarity draws, no re-roll (every level < 2), one discarded draw.
    {
        worldgen::Spawn spawn;
        bool levelOne = true;
        for (const worldgen::ItemRecord &item : spawn.equipment)
            levelOne &= item.level == 1 && item.type == 0;
        same(levelOne, 1, "fresh spawn items are empty and level 1");
        spawn.type = 69;
        spawn.level = 5;
        spawn.appearanceFlags = 0x40;
        worldgen::Random random(123);
        finishSpawn(spawn, random, true);
        same((double)random.count(), 34, "kept-gear spawn finishing draws");
        levelOne = true;
        for (const worldgen::ItemRecord &item : spawn.equipment)
            levelOne &= item.level == 1;
        same(levelOne, 1, "kept-gear spawn keeps level-1 items");
    }

    // Spawn objects §4.2 step 3, §3.3: a general-branch spawn's chest (slot 2) carries the spawn
    // level (so the finishing pass re-rolls it), and the generator never clears slots 8 and 9.
    {
        worldgen::Spawn spawn;
        spawn.type = 2; // (an ordinary person: the general branch)
        spawn.level = 10;
        spawn.equipment[8].type = 9, spawn.equipment[8].level = 7;
        worldgen::Random random(77);
        finishSpawn(spawn, random, false);
        same(spawn.equipment[2].type, 4, "general branch gives a chest");
        same(spawn.equipment[2].level, 10, "chest at the spawn level");
        same(spawn.equipment[8].type, 9, "slot 8 is never cleared");
        same(spawn.equipment[8].level == 7 || spawn.equipment[8].level == 10, 1, "slot 8 level kept or newly placed");
    }

    // Strike-call-sites S6/S7: the periodic window, its area-heal flag and the deferred-key pass.
    {
        Combatant healer;
        healer.characterClass = 3, healer.specialization = 1;
        healer.action = 0x20;
        const Phases p = phases(healer, healer.action);
        const int32_t B = p.windUp, I = p.active / 6;
        healer.struck = {7};
        StrikeWindow w = strikeWindow(healer, float(B + 5), 16);
        same(w.strikes, 1, "the interval's first 100 units strike");
        same(w.areaHeal, 0, "no heal flag inside the 100-unit window");
        same((double)healer.struck.size(), 1, "the struck set stays within the interval");
        w = strikeWindow(healer, float(B + I - 10), 16);
        same(w.areaHeal, 1, "entering the next interval sets the heal flag");
        same((double)healer.struck.size(), 0, "and clears the struck set");
        w = strikeWindow(healer, float(B + 90), 16);
        same(w.areaHeal, 1, "the 100-unit window ending sets the heal flag");

        Combatant striker;
        striker.action = 1;
        striker.deferred = {5, 3, 9};
        striker.struck = {5};
        strikeWindow(striker, 0, 16);
        same((double)striker.counterPending.size(), 2, "deferred keys not struck since are passed on");
        same((double)striker.counterPending[0], 3, "in ascending key order");
        same((double)striker.deferred.size() + striker.struck.size(), 0, "the wind-up clears both sets");
    }

    // Strike-call-sites K2: the strike centre is the position plus the fixed rotated forward
    // offset; action 0x45 lowers it by half the height above the width.
    {
        Combatant a;
        a.action = 0x45;
        Body body;
        body.size = {1, 1, 3};
        body.position[0] = 100 * 65536, body.position[1] = 200 * 65536, body.position[2] = 50 * 65536;
        const StrikeGeometry g = strikeGeometry(a, body, 0);
        same((double)(g.centre[1] - body.position[1]), 98304, "forward by 1.5 widths");
        same((double)(g.centre[2] - body.position[2]), -65536, "lowered by (3 - 1) x 0.5");
        same((double)(g.centre[0] - body.position[0]), 0, "no sideways offset at yaw 0");
        same(g.reach, 1.5, "R = 1.5 widths");
    }

    // Strike-call-sites §5: the swing count on the first target, mana on the frame's first
    // success only (two draws), none for a charged blow, whose charge feeds pair (4, 0).
    {
        Combatant a;
        a.action = 1;
        a.characterClass = 1;
        int draws = 0;
        const auto rolls = [&] { draws++; return 16383; };
        bool hitAny = false;
        afterStrike(a, true, hitAny, rolls);
        same(a.combo, 1, "the first target counts a swing");
        same(draws, 2, "two mana draws");
        same(a.mana > 0, 1, "mana gained");
        same(hitAny, 1, "the hit-any flag is set");
        const float mana = a.mana;
        afterStrike(a, false, hitAny, rolls);
        same(a.combo, 1, "a later target counts no swing");
        same(draws, 2, "no draw after the frame's first success");
        same(a.mana, mana, "no more mana");

        Combatant rogue;
        rogue.action = 1;
        rogue.characterClass = 4, rogue.specialization = 0;
        rogue.spentPower = 0.5f;
        bool none = false;
        draws = 0;
        afterStrike(rogue, true, none, rolls);
        same(draws, 0, "a charged blow gains no mana");
        same(rogue.stealth, 0.5, "its charge feeds the crit charge");
    }

    // Strike-call-sites §6 and B4: the area heal record (roll, then critical) and the beam's
    // S4 = (heal power x 1.5) x (scaled unit draw (0.05) + 1), one draw.
    {
        Combatant healer;
        healer.characterClass = 3, healer.specialization = 1;
        healer.action = 0x21;
        int draws = 0;
        const auto rolls = [&] { draws++; return 32767; };
        const HitRecord heal = areaHealRecord(healer, 4, {1, 2, 3}, rolls);
        same(draws, 2, "the heal makes two draws");
        same(heal.amount, (magicBase(healer) * 0.75f) * -0.1f, "a tenth of the heal power, negative", 1e-5);
        same((double)heal.target, 4, "the heal names its target");
        draws = 0;
        const HitInput beam = beamInput(healer, false, rolls);
        same(draws, 1, "the beam amount makes one draw");
        same(beam.amount, (magicBase(healer) * 1.5f) * 1.05f, "S4 at the top of its spread", 1e-4);
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
    // Open points 3 §1.6: the saurian's model draws come head, body, hand (also +0x1a), tail,
    // after the five choice draws.
    {
        Appearance s;
        int32_t type = 119;
        int call = 0;
        static const int SEQUENCE[] = {0, 0, 0, 0, 0, 1, 4, 2, 1};
        generate(type, s, nullptr, [&] { return call < 9 ? SEQUENCE[call++] : (call++, 0); });
        same(s.model(Part::Head), 2146, "saurian head from draw 6");
        same(s.model(Part::Body), 2137, "saurian body from draw 7");
        same(s.model(Part::Hand) == 2144 && s.model(Part::Foot) == 2144, 1, "saurian hand from draw 8, twice");
        same(s.model(Part::Tail), 2151, "saurian tail from draw 9");
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

    // Pose selector contracts: case bodies whose targets changed. A fresh state takes each
    // target in one frame; standing still, the rest hands are at (+-6, 2.1, -4.1).
    {
        Appearance a;
        a.box = {0.96f, 0.5f, 2.16f};
        const auto voxels = [](uint16_t) { return glm::vec3(10); };
        const auto pose = [&](Combatant &c, int32_t action, int32_t ms, Motion m = {}, float amplitude = 0) {
            c.action = action;
            c.actionMs = ms;
            m.fighter = &c;
            PoseState s;
            s.amplitude = amplitude;
            const auto parts = poseParts(s, a, m, 16, voxels);
            return std::make_pair(s, parts);
        };
        Combatant c;
        c.multipliers[1] = 1 / attackSpeed(c);
        // 01-18 selector 8 with an off-hand model: the off-hand offset Z gains 5 and the off-hand
        // rest position is not regripped.
        c.equipment[OFF_HAND] = {3, 0, 1, 0, 1, 0};
        auto [s8, parts8] = pose(c, 98, 300);
        same(s8.handL.z, 5, "selector 8: off-hand offset Z += 5", 1e-6);
        same(s8.turnL.z, 90, "selector 8: off-hand angle (0, 60, 90)", 1e-4);
        same(s8.leftHand.x, -6, "selector 8: off hand stays at its rest position", 1e-4);
        c.equipment[OFF_HAND] = {};
        // 01-18 selector 1, t < 500: owner +0x1188 (the walk amplitude) is cleared.
        auto [s1, parts1] = pose(c, 54, 100, Motion{}, 1);
        same(s1.amplitude, 0, "selector 1 clears the walk amplitude");
        // 01-18 selector 0x13 strike: the main-hand offset target is (1, -4, -1).
        const Phases p7 = phases(c, 7);
        auto [s19, parts19] = pose(c, 7, p7.windUp + 1);
        same(s19.handR.y, -4, "selector 0x13: main-hand offset (1, -4, -1)", 1e-6);
        same(s19.shift.y, 3, "selector 0x13: body offset Y 3", 1e-6);
        // 25-30 selector 0x30, B <= clock < R0: body angles (0, 0, 90), off hand at main - 2.
        const Phases p65 = phases(c, 65);
        auto [s48, parts48] = pose(c, 65, p65.windUp + 1);
        same(s48.torso.z, 90, "selector 0x30: body angle Z 90", 1e-4);
        same(s48.leftHand.x, s48.rightHand.x - 2, "selector 0x30: off hand at main X - 2", 1e-4);
        // 31-49 selector 0x46: nothing is written after T1 + T2.
        const Phases p51 = phases(c, 51);
        same(p51.recovery > 1, 1, "action 51 has a recovery phase");
        {
            auto [s70, parts70] = pose(c, 51, p51.windUp + p51.active + 1);
            same(s70.tilt.x, 0, "selector 0x46: no creature angle after the strike", 1e-6);
        }
        // 31-49 selector 0x47: model 2101 between the hands during the wind-up.
        const Phases p108 = phases(c, 108);
        same(p108.windUp > 1, 1, "action 108 has a wind-up");
        {
            auto [s71, parts71] = pose(c, 108, 1);
            same(std::any_of(parts71.begin(), parts71.end(), [](const PartDraw &d) { return d.model == 2101; }), 1,
                 "selector 0x47 draws model 2101");
        }
        // Pose owner-inputs §4: selector 0x30 takes the late hand-turn source (owner +0x148).
        c.handTurnLate = {10, 20, 30};
        {
            auto [sh, partsh] = pose(c, 65, p65.windUp + 1);
            same(sh.roll == glm::vec3(10, 20, 30), true, "selector 0x30: hand turn from owner +0x148");
        }
        c.handTurnLate = glm::vec3(0);
        // Pose owner-inputs §8 / 01-18 §6: selector 0x0F turns the creature angle by -360 over
        // the active phase only when half the swing count leaves remainder 2 modulo 3.
        int32_t spinAction = -1;
        for (int32_t k = 1; k < 256 && spinAction < 0; k++) {
            c.action = k;
            if (poseOf(c) == 0x0F)
                spinAction = k;
        }
        same(spinAction > 0, true, "an action uses selector 0x0F");
        {
            const Phases ps = phases(c, spinAction);
            c.swingCount = 4;
            // A quarter of the active phase: -90 (a target of -180 would fold to no turn).
            auto [sa, partsa] = pose(c, spinAction, ps.windUp + ps.active / 4);
            same(sa.tilt.z, -360.f * float(ps.active / 4) / float(ps.active), "selector 0x0F spins on variant 2", 1e-2);
            c.swingCount = 2;
            auto [sb, partsb] = pose(c, spinAction, ps.windUp + ps.active / 4);
            same(sb.tilt.z, 0, "selector 0x0F does not spin on variant 1", 1e-6);
            c.swingCount = 0;
        }
        // Pose owner-inputs §8 writers: the combo is taken through the wind-up and at each pulse
        // boundary of a pulsing action (0x1E: a sixth of the active phase), not in between. The
        // phases depend on the combo (attack speed), so they are taken after it is set.
        {
            Combatant w = c;
            w.action = 0x1E;
            w.combo = 7;
            const Phases p7 = phases(w, 0x1E);
            updateSwingCount(w, p7.windUp - 10, 5);
            same(w.swingCount, 7, "writer 2: the wind-up takes the combo");
            w.combo = 9;
            const Phases p9 = phases(w, 0x1E);
            const int32_t P = p9.active / 6;
            same(P > 2, true, "action 0x1E pulses");
            updateSwingCount(w, p9.windUp + 1, 1);
            same(w.swingCount, 7, "inside a pulse nothing is written");
            updateSwingCount(w, p9.windUp + P - 1, 2);
            same(w.swingCount, 9, "writer 1: a pulse boundary takes the combo");
        }
        // Weapon pose contract §5: knocked down, a biped's creature offset Z is (box y - box z) / 2.
        Motion down;
        down.knockedDown = true;
        auto [sk, partsk] = pose(c, 0, 0, down);
        same(sk.lean.z, (0.5f - 2.16f) * 0.5f, "knocked down: offset from box y", 1e-6);
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

        // Weapon pose contract §8: a carried weapon is centred and turned -135 about y, so
        // its tip points down; a carried staff is turned -45, so its tip points up.
        for (const uint8_t kind : {uint8_t(0), uint8_t(STAFF)}) {
            c.equipment[MAIN_HAND] = {3, kind, 1, 0, 1, 0};
            c.action = 0;
            c.actionMs = 0;
            PoseState back;
            poseParts(back, a, m, 16, voxels);
            const glm::vec3 along = back.trail[0][0] - back.trail[1][0];
            const char *what = kind == STAFF ? "a carried staff points up" : "a carried sword points down";
            same(along.z > 0, kind == STAFF, what);
            same(std::fabs(along.z) / glm::length(along), std::sqrt(0.5), "carried at 45 degrees", 1e-3);
        }
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
            // The window is T - dt < D <= T (the staff's is T < D <= T + dt).
            launch(mage, body, glm::vec3(0), D - 10, 16, [] { return 0; }, shots, fx);
            same(fx.bursts.size(), 0, "no explosion while T < D");
            launch(mage, body, glm::vec3(0), D + 6, 16, [] { return 0; }, shots, fx);
            same(fx.bursts.size(), 1, "one explosion burst");
            same(fx.bursts.empty() ? 0 : fx.bursts[0].count, 80, "80 particles");
            same(fx.bursts.empty() ? 0 : fx.bursts[0].spread, 20, "spread 20");
            same(std::any_of(fx.sounds.begin(), fx.sounds.end(), [](const SoundEvent &e) { return e.sound == 81; }), 1,
                 "sound 81");
        }
        // §5.3: a staff's release, spread 8, at the creature plus the float vector x 65536.
        {
            Combatant mage;
            mage.action = 30;
            mage.aim = {1.5f, -2.25f, 0.1f};
            const float D = (float)duration(mage, 30);
            Body body;
            body.place({10, 20, 30});
            ShotEffects fx;
            std::vector<Projectile> shots;
            launch(mage, body, mage.aim, D - 10, 16, [] { return 0; }, shots, fx);
            same(fx.bursts.size(), 1, "one staff burst");
            const Burst *b = fx.bursts.empty() ? nullptr : &fx.bursts[0];
            same(b ? b->spread : 0, 8, "staff spread 8");
            same(b ? (double)b->position[0] : 0, 10.0 * 65536 + 1.5 * 65536, "staff burst x");
            same(b ? (double)b->position[1] : 0, 20.0 * 65536 - 2.25 * 65536, "staff burst y");
            same(b ? (double)b->position[2] : 0, 30.0 * 65536 + (double)(int64_t)(0.1f * 65536.f), "staff burst z");
        }
        // §0: the two block rules, §6.2 / §7.1 the distances (engine fog distance for both floats).
        same(collisionCell(-65536), -2, "collision: an exact negative multiple goes one lower");
        same(collisionCell(-1), -1, "collision: -1/65536 is block -1");
        same(collisionCell(65536), 1, "collision: positive truncates");
        same(lightCell(-1), 0, "light: truncates toward zero");
        same(lightCell(-65537), -1, "light: -1.00002 is block -1");
        same(particleCullRadius(640), 0.2 * (640 + 22.4), "cull radius", 1e-3);
        same(leafZoneReach(640), 160, "leaf zone reach", 1e-4);
        // §4 step 5: the sub-step is d x (1/m), the float reciprocal multiplied in.
        {
            Particle p;
            p.collides = true;
            p.velocity = {125, 0, 0};
            p.ageMs = 0;
            list.list = {p};
            list.update(20, air);
            const float s = 20 * 0.001f, d = 125.f * s, e = d * (1.f / 3.f);
            same((double)list.list[0].position[0], 3.0 * (double)(int64_t)((double)e * 65536.0), "three sub-steps of d x (1/3)");
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
                     "quest activation publishes the captive's 0x400 (lens/pose) bit to both state views");
                n.fighter.mode |= 0xa020;
            }
        same(camp.free(prisoner, {0, 0, 10}), rescue.id, "freed, it belongs to its quest");
        for (const Npc &n : camp.all())
            if (n.fighter.id == prisoner)
                same(n.fighter.mode == n.body.mode && (n.fighter.mode & 0xa420) == 0xa020, 1,
                     "freeing a captive clears only the 0x400 bit and immediately publishes the full retained word");
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
        const auto none = [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &, int32_t &) { return false; };
        const auto level5 = [](glm::dvec3) { return 5; };
        std::vector<HitRecord> hits;
        std::vector<SoundEvent> sounds;
        // Attack-routine site 1: the player resolves through the attackerless path.
        Combatant victim;
        victim.id = 1, victim.health = 100;
        int draws = 0;
        bool spikeResistance = true;
        const StaticStates::TrapStrike strike = [&](const StaticStates::Creature &c, float damage, glm::vec3 push,
                                                    bool resistance) {
            if (c.id != victim.id) {
                hits.push_back({});
                hits.back().target = c.id;
                return;
            }
            spikeResistance = resistance;
            glm::vec3 velocity(0);
            const auto out = resolveHit(victim, velocity, nullptr, trapHitInput(c.at, damage, push, resistance), [&] {
                draws++;
                return 12345;
            });
            hits.insert(hits.end(), out.records.begin(), out.records.end());
        };
        const std::vector<StaticStates::Creature> nobody;
        states.step(16, none, held);
        states.traps(16, nobody, held, level5, strike, sounds);
        same((*states.zone(0, 0))[0].state, 0, "retracted after 2 s");
        const std::vector<StaticStates::Creature> here{{1, {8, 5, 1.91}, 0}, {2, {5, 7, 1.91}, 1}};
        for (int i = 0; i < 130 && (*states.zone(0, 0))[0].state == 0; i++) {
            states.step(16, none, held);
            states.traps(16, here, held, level5, strike, sounds);
        }
        same((*states.zone(0, 0))[0].state, 1, "up for the player");
        same(std::any_of(sounds.begin(), sounds.end(), [](const SoundEvent &e) { return e.sound == 52; }), 1, "with sound 52");
        hits.clear();
        // Hazards-blast-ticks §3.3 step 7: one pass per 200 ms quotient of the trap world clock.
        for (int i = 0; i < 25; i++) {
            states.step(16, none, held);
            states.traps(16, here, held, level5, strike, sounds);
        }
        same((double)hits.size(), 2, "two passes in 400 ms");
        same(!hits.empty() && hits[0].target == 1, 1, "the player is hurt");
        same(hits.empty() ? 0 : hits[0].amount, 20, "20 a pass at level 5", 1e-4);
        same(hits.empty() ? 0 : hits[0].knockBack.x, 20, "thrown away from it", 1e-4);
        same(std::none_of(hits.begin(), hits.end(), [](const HitRecord &h) { return h.target == 2; }), 1,
             "the monster is spared");
        same(draws, 2, "one discarded draw per attackerless hit");
        same(!hits.empty() && hits[0].attacker == 0 && hits[0].kind == HIT_DAMAGE, 1, "a main record with no attacker");
        same(victim.hitFlash, 0.5, "the attackerless flash");
        same(spikeResistance, 0, "type 7 takes the armour selector");
        // Damage-taken buffs multiply; no mitigation runs on this path.
        victim.buffs.push_back({DAMAGE_TAKEN, 0.5f, 10000});
        hits.clear();
        strike({1, {8, 5, 1.91}, 0}, 20.f, {20, 0, 5}, false);
        same(hits.empty() ? 0 : hits[0].amount, 10, "damage-taken buff halves the trap hit", 1e-6);
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
        // §2.2 step 4: q = trunc(fixed difference x d), so a product below one unit skips.
        same(states.blocks(map, lo, hi, 0, -1e-6), 0, "a sub-unit approach product skips the static");
        // §2.3: the lower faces compare with <=, the upper ones strictly.
        same(states.blocks(map, {7.5, 9.5, 5.2}, {8.5, 10.5, 7.2}), 1, "touching its -x face (X - hx = pos + w/2) hits");
        same(states.blocks(map, {11.5, 9.5, 5.2}, {12.5, 10.5, 7.2}), 0, "touching its +x face does not");
    }

    // Open points 2 §1: the search walks round a stand 3.5 blocks tall; as read (§1.4, the
    // +y ledge flag opens the gate on flat ground, open point OP2-3) a table blocks it too.
    {
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        const auto route = [&](double height) {
            PathSearch search;
            search.records = [height](int32_t x, int32_t y, const route::RecordVisitor &visit) {
                if (x == 0 && y == 0)
                    visit({123, 12, {5 * 65536 + 32768, 32768, 65536}, 0, {1, 6, float(height)}, 1, {0, 0, 0}});
            };
            search.start({0.5, 0.5, 1}, {10.5, 0.5, 1}, 1, {0.96f, 0.96f, 2.16f});
            for (int i = 0; i < 40 && search.searching(); i++)
                search.rounds({0.96f, 0.96f, 2.16f}, 1, flat);
            bool through = false;
            for (const auto &c : search.state.path)
                through = through || (c[0] == 5 && c[1] > -3 && c[1] < 3);
            same(!search.state.path.empty() && search.state.path.back()[0] >= 9, 1, "the search gets there");
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

    // World objects §2.1: a landing plot's airship template.
    {
        const worldgen::AirshipTemplate t = worldgen::airshipTemplate(3, 7, 1, 0, 60, 1000, 2000);
        same(t.id, (((7ll << 16) + 3) << 8) + 1, "identifier K(zone x, zone y, index)");
        same(t.heading, 0, "heading r x 90");
        same(t.home[0] == (981ll << 16) && t.home[1] == (2000ll << 16) && t.home[2] == (126ll << 16), 1,
             "home 19 out along (r + 2) pi / 2, 66 above the top");
        same(t.target[0] == t.home[0] && t.target[1] == t.home[1] - 100 * 65536 && t.target[2] == t.home[2] + 100 * 65536,
             1, "target 100 back and 100 up");
        same(t.destination[0] == t.home[0] + 50 * 65536 && t.destination[2] == t.home[2] + 20 * 65536, 1, "destination");
        same(worldgen::airshipTemplate(3, 7, 0, 1, 60, 1000, 2000).heading, 90, "rotation 1 heads 90");
    }

    // World objects §2.4, §3, §5: ships are copied in within 512 blocks of a player and dropped
    // beyond; a body lands on one (standing, its key carried); red voxels never collide.
    {
        worldgen::Prefab model;
        model.sx = 10, model.sy = 10, model.sz = 4;
        model.rgb.assign(3 * 400, 90);
        for (int32_t x = 0; x < 10; x++) // the column x = 0 .. 9, y = 0 painted pure red
            for (int32_t z = 0; z < 4; z++) {
                uint8_t *c = &model.rgb[3 * (size_t(z * 10) * 10 + size_t(x))];
                c[0] = 255, c[1] = 0, c[2] = 0;
            }
        auto zone = std::make_shared<worldgen::Zone>();
        zone->zx = 40, zone->zy = 40;
        worldgen::AirshipTemplate t;
        t.id = 77;
        t.target = {int64_t(40 * 256 + 128) << 16, int64_t(40 * 256 + 128) << 16, int64_t(100) << 16};
        t.home = t.target;
        zone->airships.push_back(t);
        worldgen::WorldMap map;
        map.add(zone);
        Airships ships;
        ships.setModel(model);
        const Airships::Raw far{t.target[0] + (600ll << 16), t.target[1], t.target[2]};
        ships.refresh(std::span(&far, 1), map);
        same((double)ships.ships().size(), 0, "no copy beyond 512 blocks");
        const Airships::Raw nearby{t.target[0] + (200ll << 16), t.target[1], t.target[2]}; // its zone is a player zone
        ships.refresh(std::span(&nearby, 1), map);
        same(ships.ships().size() == 1 && ships.ships().begin()->second.position == t.target, 1,
             "copied within 512 blocks, posed at the target");
        // §3.2: a box inside the hull hits and takes the hull's base.
        int64_t base = 0;
        same(ships.collide(t.target.data(), glm::vec3(1), 9, base), 1, "a box in the hull collides");
        same(base, t.target[2] - 2 * 65536, "base = position z - T(half height)");
        same(!ships.ships().empty() && ships.ships().begin()->second.carried.contains(9), 1, "the key is carried");
        // §5: the red column (model y 0, world y = target - 5) never collides.
        const int64_t onRed[3] = {t.target[0], t.target[1] - int64_t(4.5 * 65536), t.target[2]};
        int64_t unused = 0;
        same(ships.collide(onRed, glm::vec3(0.5f), 10, unused), 0, "pure red voxels are empty");
        // A body dropped above the deck lands on it.
        Body b;
        b.place({40 * 256 + 128.5, 40 * 256 + 128.5, 100 + 2 + 1.08 + 1});
        b.obstacles = &ships, b.key = 11;
        const auto air = [](int32_t, int32_t, int32_t) -> uint8_t { return 0; };
        for (int i = 0; i < 60; i++)
            step(b, 16, air);
        same((b.contact & ON_GROUND) != 0, 1, "standing on the deck");
        same(b.blocks().z, 100 + 2 + 1.08, "resting on the deck top", 0.05);
        same(!ships.ships().empty() && ships.ships().begin()->second.carried.contains(11), 1, "the lander is carried");
        // §6.3: a stationary ship keeps its carried creature where it is.
        const int64_t before[3] = {b.position[0], b.position[1], b.position[2]};
        ships.step(16, false, map, [&](int64_t key, Airships::Carried &out) {
            if (key != 11)
                return false;
            out = {b.position, b.follow, &b.yaw};
            return true;
        });
        same(b.position[0] == before[0] && b.position[1] == before[1] && b.position[2] == before[2], 1,
             "a stationary ship leaves its passenger in place");
        same(!ships.ships().empty() && !ships.ships().begin()->second.carried.contains(9), 1, "absent creatures leave the set");
        // Steering §5: a body sunk into the hull is pushed out along the nearest free axis.
        Body stuck;
        stuck.place({40 * 256 + 128.5 + 4.2, 40 * 256 + 128.5, 100});
        stuck.obstacles = &ships;
        ships.unstick(stuck.position, stuck.size);
        same(stuck.blocks().x > 40 * 256 + 128 + 5, 1, "pushed out through the nearer +x side");
        same(stuck.blocks().y, 40 * 256 + 128.5, "only one axis moves", 1e-6);
        // Steering §3: a keyed creature is set on the first marker voxel (model 0, 0, 0 here,
        // the red column's first), a hair above it, and takes the ship's yaw.
        Body rider;
        float riderYaw = 33;
        same(ships.place(77, rider, riderYaw), 1, "placed on its ship");
        same(rider.position[0] == t.target[0] - 5 * 65536 && rider.position[1] == t.target[1] - 5 * 65536, 1,
             "at the marker, turned by the ship's yaw 0");
        same(rider.position[2], t.target[2] - 2 * 65536 + int64_t((rider.size.z * 0.5f + 0.001f) * 65536.f),
             "a hair above the marker by half its height");
        same(riderYaw, 0, "takes the ship's yaw");
        same(ships.place(78, rider, riderYaw), 0, "no ship, no placement");
        // Steering §1.3: from state 1 the ship heads home (here 100 blocks below its target)
        // at 10 blocks a second, and the target moves with it.
        const int64_t z0 = ships.ships().begin()->second.state.target[2];
        ships.step(16, true, map, [](int64_t, Airships::Carried &) { return false; });
        const Airships::Ship &moving = ships.ships().begin()->second;
        same(moving.velocity.z, 0, "the template's home is its target here, so it has arrived", 0);
        same(moving.course, 2, "arrival at home moves on to course 2");
        same(moving.state.target[2], z0, "no velocity, no move");
        // §1.5: course 2 then flies at 10 blocks a second toward home + (0, 50, 20) (heading 0).
        ships.step(16, true, map, [](int64_t, Airships::Carried &) { return false; });
        const glm::vec3 v = ships.ships().begin()->second.velocity;
        same(glm::length(v), 10, "course 2 speed", 1e-4);
        same(v.x == 0 && v.y > 9 && v.z > 3.5f, 1, "toward the waypoint ahead and above home");
        same((double)(ships.ships().begin()->second.state.target[1] - t.target[1]) / 65536.0, v.y * 0.016, "the target moved",
             1e-4);
        ships.refresh(std::span(&far, 1), map);
        same((double)ships.ships().size(), 0, "dropped beyond 512 blocks of every player");
    }

    // Airship pilot behaviour §5: the pilot (behaviour 8) faces a player within 8 blocks and
    // otherwise stands still; it never walks.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        worldgen::Random random{3};
        Npcs npcs{random};
        Npc pilot;
        pilot.fighter.id = 900, pilot.fighter.controller = 3, pilot.fighter.characterClass = 0x88;
        pilot.fighter.health = maxHealth(pilot.fighter);
        pilot.behaviour = 8;
        pilot.body.place({10.5, 10.5, 1 + 1.08 + 0.01});
        pilot.home = pilot.body.blocks();
        npcs.npcs().push_back(pilot);
        Combatant me;
        me.id = 1, me.health = maxHealth(me);
        Body body;
        body.place({14.5, 10.5, 1 + 1.08 + 0.01});
        for (int i = 0; i < 4; i++)
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
        const Npc &faced = npcs.all().front();
        same((faced.body.mode & AIMING) != 0 && faced.body.look.x > 3.9f, 1, "faces the player 4 blocks away");
        same(glm::length(faced.body.intent), 0, "does not walk toward it");
        body.place({30.5, 10.5, 1 + 1.08 + 0.01});
        for (int i = 0; i < 4; i++)
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
        same((npcs.all().front().body.mode & AIMING) == 0 && glm::length(npcs.all().front().body.intent) == 0, 1,
             "beyond 8 blocks it stops facing and stays put");
    }

    // Physics §2, §4.2, §4.4 on the NPC side of the world step: the step's action changes and
    // yaw reach the creature, the unenlarged separation push, the skill fallback and equipment
    // checks, and the landing thud only while a local player exists.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        const auto standing = [](uint64_t id, glm::dvec3 at) {
            Npc n;
            n.fighter.id = id, n.fighter.controller = 3, n.fighter.characterClass = 0x88;
            n.fighter.health = maxHealth(n.fighter);
            n.behaviour = 8; // stands still
            n.body.place(at);
            n.home = n.body.blocks();
            return n;
        };
        Combatant me;
        me.id = 1, me.health = maxHealth(me);
        Body body;
        body.place({60.5, 60.5, 1 + 1.08 + 0.01});
        {
            worldgen::Random random{3};
            Npcs npcs{random};
            Npc sitter = standing(900, {10.5, 10.5, 1 + 1.08 + 0.01});
            sitter.fighter.action = 83;
            sitter.fighter.level = 6, sitter.fighter.powerTier = 2;
            sitter.fighter.mode = GLIDER | LANTERN;
            sitter.body.intent = {80, 0, 0};
            npcs.npcs().push_back(sitter);
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
            const Npc &n = npcs.all().front();
            same(n.fighter.action, 0, "intent ends an NPC's sitting (action 83) on the fighter");
            same(n.body.skillFallback, 6 / 2 + 2 + 1, "an NPC's skill fallback is level / 2 + tier + 1");
            same((n.fighter.mode & (GLIDER | LANTERN)) != 0, 0, "an NPC without glider or lantern loses both modes");
        }
        {
            worldgen::Random random{3};
            Npcs npcs{random};
            npcs.npcs().push_back(standing(900, {10.5, 10.5, 1 + 1.08 + 0.01}));
            npcs.npcs().push_back(standing(901, {11.0, 10.5, 1 + 1.08 + 0.01}));
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
            // 5 · s′x / sx = 5 blocks/s for 16 ms (the enlarged s′x would give 0.163).
            same(10.5 - npcs.all().front().body.blocks().x, 5 * 0.016, "NPC separation scales by the unenlarged box",
                 2e-3);
        }
        for (const bool local : {false, true}) {
            worldgen::Random random{3};
            Npcs npcs{random};
            Npc falling = standing(900, {10.5, 10.5, 1 + 1.08 + 0.05});
            falling.body.velocity = {0, 0, -10};
            npcs.npcs().push_back(falling);
            const NpcFrame f = npcs.step(16, map, {{me, body, 0, me.actionMs, true, false, local}}, 43200000, 0, floor);
            int thuds = 0;
            for (const SoundEvent &s : f.sounds)
                if (s.sound == 32 && s.volume == 0.5f && s.pitch >= 1 && s.pitch <= 1.2f)
                    thuds++;
            same(thuds, local ? 1 : 0,
                 local ? "an NPC landing hard thuds while a local player exists" : "no NPC thud without a local player");
        }
    }

    // The shared angle step: within a quarter turn it reaches the target; beyond it folds
    // (120 moves as 60, 180 does not move).
    {
        same(angleStep(0.f, 90.f, 1.f), 90, "a quarter turn is reached", 1e-4);
        same(angleStep(0.f, 120.f, 1.f), 60, "120 folds to 60", 1e-4);
        same(angleStep(0.f, 180.f, 1.f), 0, "180 does not move", 1e-4);
        same(angleStep(10.f, 40.f, 0.5f), 25, "half the way", 1e-4);
    }

    // Remote smoothing (alpha-remote-creature-interpolation.md): a snap puts the drawn pose on
    // the record; a smoothing receipt leaves it and lets it close the gap; a dead creature
    // stays frozen.
    {
        auto air = [](int32_t, int32_t, int32_t) -> uint8_t { return 0; };
        net::CreatureRecord record = net::CreatureRecord::nativeDefaults();
        record.setPosition(0, 100ll << 16), record.setPosition(1, 100ll << 16), record.setPosition(2, 100ll << 16);
        record.setHealth(50.f);
        net::RemoteCreatures remotes;
        remotes.receive(7, record, true);
        same(remotes.find(7)->drawn[0], 100ll << 16, "a snap draws at the record");
        record.setPosition(0, 104ll << 16); // a jump of 4 blocks, smoothed
        remotes.receive(7, record, false);
        same(remotes.find(7)->drawn[0], 100ll << 16, "a smoothing receipt leaves the drawn pose");
        remotes.step(16, air, {});
        const double moved = double(remotes.find(7)->drawn[0] - (100ll << 16)) / 65536.0;
        same(moved > 0.5 && moved < 4.0, 1, "the drawn pose closes part of the gap in one update");
        record.setHealth(0.f);
        remotes.receive(7, record, false);
        const int64_t frozen = remotes.find(7)->drawn[0];
        remotes.step(16, air, {});
        same(remotes.find(7)->drawn[0], frozen, "a dead creature's drawn pose stays frozen");
        remotes.retain({});
        same(remotes.find(7) == nullptr, 1, "unlisted creatures are forgotten");

        // Aim §3.7/§3.8 on a received creature: facing its aim turns the yaw toward the aim's
        // heading from the received value, and with action 0 the drawn aim is the aim.
        record.setHealth(50.f);
        record.bytes[0x114] = 0x04; // the face-aim mode bit
        const float aim[3] = {1.f, 0.f, 0.f}, pitch = 12.f;
        std::memcpy(&record.bytes[0x150], aim, sizeof aim);
        std::memcpy(&record.bytes[0x48], &pitch, sizeof pitch);
        remotes.receive(8, record, true);
        same(remotes.find(8)->headPitch, 12.f, "the head pitch is received");
        remotes.step(16, air, {});
        const net::RemoteCreatures::Remote &faced = *remotes.find(8);
        same(faced.drawnLook == glm::vec3(1, 0, 0), true, "with action 0 the drawn aim is the aim");
        same(faced.rotation.z < 0.f && faced.rotation.z > -90.f, true, "the yaw turns toward the aim's heading");
        same(faced.headPitch < 12.f && faced.headPitch > 0.f, true, "the head pitch turns toward the level aim");
    }

    // Steering §2.1: the angle helper reaches b within a quarter turn, and stays on the near
    // side of a beyond it.
    {
        Airships probe; // (the helper is exercised through a ship's yaw follow)
        auto zone = std::make_shared<worldgen::Zone>();
        zone->zx = 40, zone->zy = 40;
        worldgen::AirshipTemplate t;
        t.id = 5;
        t.target = {int64_t(40 * 256 + 128) << 16, int64_t(40 * 256 + 128) << 16, int64_t(100) << 16};
        t.home = t.target;
        t.targetYaw = 60;
        zone->airships.push_back(t);
        worldgen::WorldMap map;
        map.add(zone);
        worldgen::Prefab model;
        model.sx = model.sy = model.sz = 2;
        model.rgb.assign(3 * 8, 90);
        probe.setModel(model);
        probe.refresh(std::span(&t.target, 1), map);
        const float yaw0 = probe.ships().begin()->second.yaw;
        same(yaw0, 60, "copied at the target yaw");
        probe.step(16, false, map, [](int64_t, Airships::Carried &) { return false; });
        same(probe.ships().begin()->second.yaw, 60, "already facing its target yaw", 1e-4);
    }

    // Height §1.2, §1.3, §1.5: the step-up and step-down snaps land on the exact fixed targets
    // and keep the drawn height (z - offset) continuous; the offset then decays per millisecond.
    {
        // Ground below z = 1, a one-block ledge from y = 3 to y = 5, ground again beyond.
        auto ledge = [](int32_t, int32_t y, int32_t z) -> uint8_t { return z < 1 || (z < 2 && y >= 3 && y < 5) ? 1 : 0; };
        Body b;
        b.place({0.5, 0.5, 1 + 1.08 + 0.01});
        step(b, 16, ledge);
        b.intent = {0, 80, 0};
        const int64_t half = (int64_t)(b.size.z * 0.5f * 65536.f);
        bool up = false, down = false;
        for (int i = 0; i < 200 && !(up && down); i++) {
            const int64_t z0 = b.position[2];
            const double drawn0 = (double)z0 / 65536.0 - b.bob;
            step(b, 16, ledge);
            const int64_t moved = b.position[2] - z0;
            if (!up && moved > 32768) {
                up = true;
                // (later sub-steps of the frame may still fall a little before landing)
                same((double)b.position[2], 1 * 65536 + half + 66191 - 512, "step-up lands 1.01 above the bottom's layer", 512);
                same(b.bob > 0.6f, 1, "step-up offset carries the rise (after 16 ms of decay)");
                same((double)b.position[2] / 65536.0 - b.bob, drawn0, "drawn height near-continuous over step-up", 0.3);
            } else if (up && !down && moved < -32768) {
                down = true;
                same(b.position[2], 0 * 65536 + half + 66191, "step-down lands 1.01 above the layer 1.2 below");
                same(b.bob < -0.6f, 1, "step-down offset carries the drop (after 16 ms of decay)");
                same((double)b.position[2] / 65536.0 - b.bob, drawn0, "drawn height near-continuous over step-down", 0.3);
            }
        }
        same(up, 1, "walked up the ledge");
        same(down, 1, "walked down the ledge");
        // Decay: below zero at 0.01, otherwise 0.02, once per millisecond in binary32.
        Body still;
        still.place({0.5, 0.5, 1 + 1.08 + 0.01});
        step(still, 16, ledge);
        for (const float start : {-0.5f, 0.75f}) {
            still.bob = start;
            float want = start;
            const float rate = 0.f > start ? 0.01f : 0.02f;
            for (int ms = 0; ms < 16; ms++)
                want = (0.f - want) * rate + want;
            step(still, 16, ledge);
            same(std::bit_cast<uint32_t>(still.bob), std::bit_cast<uint32_t>(want), "offset decay bit-exact");
        }
        // A step of 0 ms makes no repetition.
        still.bob = 0.25f;
        step(still, 0, ledge);
        same(still.bob, 0.25, "zero step keeps the offset", 0);
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
        // Travel §3.4 step 6, §3.6: a wall crash still applies the lift and the wind that step;
        // its stun (1500 ms) lets go of the wall in the same step. A ground crash skips them.
        {
            Body b;
            b.place({0.5, 0.5, 100});
            b.local = true;
            b.mode = GLIDER | RUN | CLIMB;
            b.skills[3] = 1;
            b.contact = ON_WALL;
            b.wallNormal = {1, 0, 0};
            b.velocity = {-9, 0, -1};
            const float wz = wind(b.position).z;
            step(b, 16, air);
            same(b.crashed && b.stunMs == 1500 && !(b.mode & CLIMB), 1, "wall crash: stun 1500, wall let go");
            same(b.velocity.z, -1.048 + 0.48 + wz * 0.016, "wall crash keeps the lift and the wind", 1e-4);
            Body g;
            g.place({0.5, 0.5, 100});
            g.local = true;
            g.mode = GLIDER | RUN;
            g.skills[3] = 1;
            g.contact = ON_GROUND;
            g.bank = 60;
            g.velocity = {0, 0, -1};
            step(g, 16, air);
            same(g.crashed, 1, "ground crash at bank 60");
            same(g.velocity.z, -1 - 0.48, "a ground crash has no lift or wind", 1e-4);
        }
        // Travel §0 Blend and §6.4 head pitch.
        same(blendWeight(0.001, 16), 0.015880, "Blend weight over 16 ms", 1e-6);
        same(blendWeight(0.001, 0), 0, "no weight over 0 ms", 0);
        same(stepHeadPitch(10, false, 16), (float)std::exp((double)(16.f * -0.01f)) * 10.f,
             "head pitch x float(exp(double(float(dt) x -0.01f)))");
        same(stepHeadPitch(0, true, 1), 0.225, "climbing: toward 45 at 0.005 per ms", 1e-6);
        // Rulings walk-flag-pitch §2.2 in the shared facing block: aiming, the decay comes first
        // and the aim blend reads the decayed pitch; on the climb hold neither runs.
        {
            Body body;
            body.mode = AIMING;
            Combatant fighter;
            float yaw = 0, pitch = 10;
            glm::vec3 drawn(0, 1, 0);
            faceAfterStep(yaw, pitch, drawn, body, fighter, 0, 16);
            const float decayed = stepHeadPitch(10, false, 16);
            same(pitch < decayed && pitch > 0.f, 1, "decayed, then blended toward the level aim");
            body.mode = AIMING | CLIMB;
            body.contact = ON_WALL;
            pitch = 10;
            faceAfterStep(yaw, pitch, drawn, body, fighter, 0, 16);
            same(pitch, stepHeadPitch(10, true, 16), "climb hold: toward 45 only");
        }
        // Travel §6.3, §6.5 reversal: exhausted, a held key cannot hold the wall (the drain clears
        // 0x01 before the vertical stage, so full gravity applies); with no key the player hangs.
        {
            const auto wall = [](int32_t x, int32_t, int32_t) -> uint8_t { return x < 0 ? 1 : 0; };
            Body b;
            b.place({0.49, 0.5, 100});
            b.local = true;
            b.mode = CLIMB;
            b.contact = ON_WALL;
            b.stamina = 0.005f;
            b.intent = {-80, 0, 0}; // a key held (the input ran on the previous step's cleared flag)
            step(b, 16, wall);
            same(!(b.mode & CLIMB) && b.stamina == 0.f, 1, "exhausted with a key: 0x01 cleared, stamina 0");
            same(b.velocity.z < -0.4f, 1, "and the player falls");
            Body h;
            h.place({0.49, 0.5, 100});
            h.local = true;
            h.mode = CLIMB;
            h.contact = ON_WALL;
            h.stamina = 0.005f;
            step(h, 16, wall);
            same((h.mode & CLIMB) != 0 && h.velocity.z == 0.f && h.stamina == 0.005f, 1, "no key: hangs, stamina kept");
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
        // Player control §8.2: every axis probe is tested; a corner sets both components.
        {
            auto corner = [](int32_t x, int32_t y, int32_t) -> uint8_t { return x >= 1 || y >= 1 ? 1 : 0; };
            Body b;
            b.place({0.5, 0.5, 100});
            step(b, 16, corner);
            same((b.contact & ON_WALL) != 0, 1, "corner touches a wall");
            same(b.wallNormal.x, -1, "corner normal x");
            same(b.wallNormal.y, -1, "corner normal y");
            step(b, 16, air);
            same(glm::dot(b.wallNormal, b.wallNormal), 0, "normal reset without a wall");
        }
        // Player control §8.3: no ground friction in action 54 before P1 + P2.
        {
            const auto slide = [&](int32_t action) {
                Body b;
                b.place({0.5, 0.5, 1 + 1.08 + 0.01});
                step(b, 16, floor);
                b.action = action, b.actionMs = 0, b.windUpMs = 400, b.activeMs = 100;
                b.velocity = {5, 0, 0};
                step(b, 16, floor);
                return b.velocity.x;
            };
            same(slide(54) > slide(0), 1, "action 54 withholds ground friction");        }
        // Travel §8: a 30-block fall takes a quarter of the player's health.
        {
            Body b;
            b.place({0.5, 0.5, 1 + 30 + 1.08 + 0.01});
            b.local = true;
            for (int i = 0; i < 200 && !(b.contact & ON_GROUND); i++)
                step(b, 16, floor);
            same(b.fallHurt, 0.25, "30-block fall (stopped up to a substep above the floor)", 2e-2);
            // §4.2: the damage scales by the maximum health; the landing sound is any creature's.
            Body heavy;
            heavy.place({0.5, 0.5, 1 + 30 + 1.08 + 0.01});
            heavy.local = true;
            heavy.maxHealth = 200;
            Body other = heavy;
            other.local = false;
            for (int i = 0; i < 200 && !(heavy.contact & ON_GROUND); i++)
                step(heavy, 16, floor), step(other, 16, floor);
            same(heavy.fallHurt, 50, "30-block fall takes a quarter of 200", 4);
            same(other.fallHurt == 0 && other.thudded, 1, "a non-local creature thuds but takes no fall damage");
        }
        // Re-certified physics regressions.
        {
            // §0: the box conversion truncates, then goes one lower when negative.
            same(boxBlock(0), 0, "box block of 0");
            same(boxBlock(65536), 1, "box block of 1.0");
            same(boxBlock(-1), -1, "box block just below 0");
            same(boxBlock(-65536), -2, "box block of exactly -1.0 goes one lower");
            // §3.2 (reversal): action 54 charges at 40 only before P1 + X.
            const auto charged = [&](float ms) {
                Body b;
                b.place({0.5, 0.5, 100});
                b.action = 54, b.actionMs = ms, b.windUpMs = 400, b.activeMs = 100, b.durationMs = 420;
                b.velocity = {30, 0, 0};
                step(b, 16, air);
                return b;
            };
            same(charged(0).velocity.x, 30, "action 54 before P1 + X: S = 40", 1e-4);
            same(charged(600).velocity.x, 6, "action 54 after P1 + X: S = 6", 1e-4);
            // §2 item 2: the slam needs time < duration as well.
            same(charged(450).velocity.z > -1, 1, "no slam past the duration");
            Body slam = charged(-1);
            slam.action = 54, slam.actionMs = 450, slam.durationMs = 1000;
            step(slam, 16, air);
            same(slam.velocity.z, -60 - 0.48, "slam inside P1 < time < duration, time < P1 + X", 1e-3);
            // §5.4 (reversal): all four axis probes run; N may be diagonal.
            {
                auto walls = [](int32_t x, int32_t y, int32_t) -> uint8_t { return x >= 2 || y < 1 ? 1 : 0; };
                Body b;
                b.place({1.5, 1.5, 100});
                step(b, 16, walls);
                same(b.wallNormal.x == -1 && b.wallNormal.y == 1 && (b.contact & ON_WALL), 1, "diagonal N (-1, +1)");
            }
            // §5.4: the probe columns truncate toward zero (-1.18 is column -1, not -2).
            {
                auto distant = [](int32_t x, int32_t, int32_t) -> uint8_t { return x == -2 ? 1 : 0; };
                Body b;
                b.place({-0.5, 0.5, 100});
                step(b, 16, distant);
                same((b.contact & ON_WALL) != 0, 0, "truncating probe columns miss block -2");
            }
            // §5.4: with no axis hit, the corner columns the creature faces.
            {
                auto post = [](int32_t x, int32_t y, int32_t) -> uint8_t { return x == 1 && y == 1 ? 1 : 0; };
                Body b;
                b.place({0.5, 0.5, 100});
                b.yaw = 0; // facing +y
                step(b, 16, post);
                same(b.wallNormal.x == -1 && b.wallNormal.y == -1 && (b.contact & ON_WALL), 1, "corner probe N (-1, -1)");
                Body away;
                away.place({0.5, 0.5, 100});
                away.yaw = 180; // facing -y: the (+, +) corner is behind
                step(away, 16, post);
                same((away.contact & ON_WALL) != 0, 0, "no corner probe behind the creature");
            }
            // §4.4: overlap is max >= min on one side and min < max on the other; the push scales
            // by the other's unenlarged x; a zero-length u is skipped.
            {
                const glm::vec3 box(1, 1, 2);
                const int64_t me[3] = {0, 0, 0};
                const int64_t right[3] = {3 * 32768, 0, 0}, left[3] = {-3 * 32768, 0, 0};
                same(separationPush(box, me, box, true, right, false).x, -5, "touching at my max: pushed by 5 x 1/1");
                same(separationPush(box, me, box, true, left, false).x, 0, "touching at my min: no overlap");
                same(separationPush(box, me, glm::vec3(0.5f, 1, 2), true, me, false).x, 0, "zero-length u skipped");
                const int64_t close[3] = {32768, 0, 0};
                same(separationPush(box, me, glm::vec3(0.5f, 1, 2), true, close, true).x, -0.25, "unenlarged 0.5, stunned 0.1",
                     1e-6);
            }
            // §2 item 4: intent ends sitting and the lens mode; §3.2: a dodge cancels the action
            // except a dodge attack within its duration; action 81 stands, then becomes 83.
            {
                Body sit;
                sit.place({0.5, 0.5, 100});
                sit.action = 83, sit.mode = LENS, sit.intent = {80, 0, 0};
                step(sit, 16, air);
                same(sit.action == 0 && !(sit.mode & LENS), 1, "intent ends sitting and mode 0x400");
                Body roll;
                roll.place({0.5, 0.5, 100});
                roll.action = 3, roll.actionMs = 300, roll.dodgeMs = 300;
                step(roll, 16, air);
                same(roll.action, 0, "a dodge cancels a swing");
                roll.action = 50, roll.actionMs = 300, roll.durationMs = 800, roll.dodgeMs = 300;
                step(roll, 16, air);
                same(roll.action, 50, "a dodge attack within its duration stays");
                Body eat;
                eat.place({0.5, 0.5, 100});
                eat.action = 81, eat.actionMs = 500, eat.useLimitMs = 1000;
                eat.velocity = {5, 0, 0};
                step(eat, 16, air);
                same(eat.velocity.x == 0 && eat.action == 81, 1, "eating: S = 0 within the use length");
                eat.actionMs = 1500;
                step(eat, 16, air);
                same(eat.action, 83, "eating past the use length becomes 83");
            }
            // §2 item 12: a running action lets go of the wall; item 18.2: the equipment checks.
            {
                Body b;
                b.place({0.5, 0.5, 100});
                b.mode = CLIMB | LANTERN;
                b.action = 3, b.actionMs = 300, b.durationMs = 600;
                b.lanternEquipped = false;
                step(b, 16, air);
                same((b.mode & (CLIMB | LANTERN)) != 0, 0, "running action clears 0x01; no lantern clears 0x200");
                Body g;
                g.place({0.5, 0.5, 100});
                g.mode = GLIDER;
                g.skills[3] = 1;
                g.gliderEquipped = false;
                step(g, 16, air);
                same((g.mode & GLIDER) != 0, 0, "no glider equipped clears 0x10");
            }
            // §5.1: controller kind 6 is never in water; §5.3: type 101 pitches up in action 51.
            {
                Body bomb;
                bomb.place({0.5, 0.5, 5});
                bomb.controller = 6;
                step(bomb, 16, sea);
                same((bomb.contact & IN_WATER) != 0, 0, "kind 6 never in water");
                Body flyer;
                flyer.place({0.5, 0.5, 100});
                flyer.type = 101, flyer.action = 51, flyer.actionMs = 100;
                step(flyer, 16, air);
                same(flyer.swimPitch > 0, 1, "type 101 pitches toward 60 in action 51");
            }
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
        // Re-certified §7.1: the built-in defaults cameraSpeed 50, cameraSmoothness 80 and
        // minTimeStep 9; §7.3: k at 80 is 0.014427; the map zoom divides by 0.9 on wheel up.
        const GameOptions options;
        same(c.options.speed == 50 && c.options.smoothness == 80, 1, "camera option defaults 50 and 80");
        same(options[GameOptions::CAMERA_SMOOTHNESS] == 80 && options[GameOptions::MIN_TIME_STEP] == 9, 1,
             "options.cfg defaults: smoothness 80, minTimeStep 9");
        camera_follow::State s;
        s.targetAngles = {1, 0, 0};
        camera_follow::smooth(s, 1, 80);
        same(s.angles[0], 0.014427, "smoothness 80: one millisecond moves k = 0.014427", 2e-6);
        s.targetScale = 1;
        camera_follow::wheel(s, 1, true);
        same(s.targetScale, 1 / 0.9, "map zoom: wheel up divides by 0.9", 1e-6);
        camera_follow::wheel(s, -1, true);
        same(s.targetScale, 1, "map zoom: wheel down multiplies by 0.9", 1e-6);
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
        same(out.sounds[0].sound, 5, "unarmed impact: an empty striking list selects -1, identifier 5 (attack-routine M5)");
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

    // Hazards-blast-ticks §4.2: a blast that removes blocks draws its sound pitch first,
    // before any victim draw; one that removes none makes no such draw.
    for (const bool removes : {true, false}) {
        auto zone = std::make_shared<worldgen::Zone>();
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({15.5, 10.5, 1 + 1.08 + 0.01});
        worldgen::Random random{7};
        Npcs npcs{random};
        const uint64_t bomb = npcs.placeBomb({10.5, 10.5, 1.5}, 3);
        int asked = 0;
        std::optional<worldgen::Random> atQuery;
        npcs.removesBlocks = [&](glm::dvec3 centre, int32_t radius) {
            asked++;
            same(centre.x == 10.5 && centre.y == 10.5 && radius == 8, 1, "asked at the bomb, radius 8");
            atQuery = random; // the generator state just before the pitch draw
            return removes;
        };
        int blasts = 0;
        for (int i = 0; i < 20000 && blasts == 0; i++) {
            const NpcFrame f = npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, floor);
            for (const auto &e : f.explosions)
                if (e.bomb == bomb) {
                    blasts++;
                    same(e.removes, removes, "the removal flag follows the block query");
                    if (removes) {
                        const float expected = 0.5f + 0.4f * (float)atQuery->draw() / 32767.f;
                        same(e.pitch, expected, "the pitch is the draw right after the block query");
                    } else {
                        same(e.pitch, 0, "no pitch without removed blocks");
                    }
                }
        }
        same(blasts, 1, "one blast");
        same(asked, 1, "one block query per blast");
    }

    // Strike-call-sites K3: a striker wider than 4 asks once per strike frame for the cells
    // within min(trunc(R), 8) of the strike centre; when any go, the sound pitch is the draw
    // right after the query (before any target); none, no draw and no crater.
    for (const bool removes : {true, false}) {
        Combatant big;
        big.id = 41;
        big.action = 17;
        const Phases timing = phases(big, 17);
        big.actionMs = timing.windUp + timing.active / 2;
        Body body;
        body.size = {5.6f, 5.6f, 5.6f};
        body.place({0, 0, 3});
        worldgen::Random random{7};
        Npcs npcs{random};
        int asked = 0;
        glm::dvec3 at{0};
        int32_t radius = 0;
        std::optional<worldgen::Random> atQuery;
        npcs.removesBlocks = [&](glm::dvec3 centre, int32_t r) {
            asked++, at = centre, radius = r;
            atQuery = random;
            return removes;
        };
        NpcFrame frame;
        PlayerView view{big, body, 0, big.actionMs};
        npcs.dispatchPlayerActions({&view, 1}, 16, [](int32_t, int32_t, int32_t) { return uint8_t(0); }, frame);
        same(asked, 1, "one block query per strike frame");
        same(radius, 8, "R = 1.5 x 5.6 truncated and capped at 8");
        same(at.y, 1.5 * 5.6, "the strike centre, forward by the reach", 1e-4);
        same((double)frame.craters.size(), removes ? 1 : 0, "a crater only when cells go");
        if (removes && !frame.craters.empty()) {
            const float expected = (float)atQuery->draw() * 0.4f / 32767.f + 0.5f;
            same(frame.craters[0].pitch, expected, "the pitch is the draw right after the query");
            same(frame.craters[0].striker.y, 0, "the sound is at the striker", 1e-9);
        }
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
        // Open points 2 §2: on z the statics are tested after a cell hit too (a rug lying on the
        // ground under a standing creature is touched); x and y are not.
        int rugAxes = 0;
        const ObjectTest rug = [&](glm::dvec3, glm::dvec3, int axis, double, const IgnoredStatics *) {
            rugAxes |= 1 << axis;
            return true;
        };
        Body still;
        still.place({20.5, 0.5, 1 + 1.08});
        for (int i = 0; i < 5; i++)
            step(still, 16, flat, rug);
        same(rugAxes == 4 && (still.contact & ON_OBJECT) != 0, 1, "a z cell hit still tests the statics");
        const int64_t mine[3] = {65536, 0, 0}, theirs[3] = {32768, 0, 0};
        const glm::vec3 push = separationPush({0.96f, 0.96f, 2.16f}, mine, {0.96f, 0.96f, 2.16f}, true, theirs, false);
        same(push.x, 5 * 0.96 / 0.96, "pushed away at 5 s'x/sx blocks a second (unenlarged s')", 1e-4);
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
    // Path search §5: the search on flat ground from (0, 0, 10) to (3, 2, 10). (The direct
    // walk is covered by RouteGeometryTests against the held approach contract.)
    {
        const glm::vec3 box(0.96f, 0.96f, 2.16f);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z <= 9 ? 1 : 0; };
        PathSearch search;
        search.start({0.5, 0.5, 10.1}, {3.5, 2.5, 10.1}, 2, box);
        search.round(box, flat);
        same(search.state.path.size() == 2 && search.state.path[1] == PathSearch::Cell{1, 1, 10}, 1, "round 1: best (1, 1)");
        search.round(box, flat);
        same(search.state.path.back() == PathSearch::Cell{2, 2, 10}, 1, "round 2: best (2, 2)");
        search.round(box, flat);
        same(search.state.path.back() == PathSearch::Cell{3, 2, 10}, 1, "round 3: the goal");
        // Path search §3, §4.6 (re-certified): the "as Companion" loop stops only when the
        // last cell is strictly within r (squared < r^2): at r = 1 the cell (2, 2), squared
        // distance 1, does not stop it, so the loop reaches the goal in the same update.
        {
            PathSearch strict;
            strict.start({0.5, 0.5, 10.1}, {3.5, 2.5, 10.1}, 1, box);
            strict.round(box, flat);
            strict.rounds(box, 1, flat);
            same(strict.state.path.back() == PathSearch::Cell{3, 2, 10}, 1, "strict stop: past (2, 2) to the goal");
        }
        // §4.2 step 1, §4.5 (re-certified): 501 open cells abandon the search; the rebuild in
        // the same round leaves the single goal cell as the path and the ignored set empty.
        {
            PathSearch abandoned;
            abandoned.start({0.5, 0.5, 10.1}, {3.5, 2.5, 10.1}, 2, box);
            for (int32_t n = 0; n < 501; n++) {
                abandoned.state.records[{100 + n, 0, 10}] = {1, 1, 2, {100 + n, 0, 10}};
                abandoned.state.pending.insert({100 + n, 0, 10});
            }
            abandoned.round(box, flat);
            same(!abandoned.searching() && abandoned.state.path.size() == 1 &&
                 abandoned.state.path[0] == PathSearch::Cell{3, 2, 10} && abandoned.state.exclusions.empty(), 1,
                 "abandoned: the path is the goal cell");
            abandoned.rounds(box, 2, flat);
            same(!abandoned.searching() && abandoned.state.path.size() == 1, 1,
                 "no rounds without a node map (tested once before the loop)");
        }
        // §6: a creature follows a search around a wall to its goal.
        auto blocked = [](int32_t x, int32_t y, int32_t z) -> uint8_t { return z <= 9 || (x == 3 && y >= -4 && y <= 4 && z <= 12) ? 1 : 0; };
        Body walker;
        walker.place({0.5, 0.5, 10 + 1.08 + 0.01});
        PathSearch route;
        route.start({0.5, 0.5, 10.1}, {6.5, 0.5, 10.1}, 1, walker.size);
        bool arrived = false;
        for (int i = 0; i < 1000 && !arrived; i++) {
            route.rounds(walker.size, 1, blocked);
            if (route.follow(walker, 16, blocked)) route.state.rebuild();
            route.state.counter += 16;
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
        bool tamed = false, contactSeen = false;
        for (int i = 0; i < 1500 && !tamed; i++) { // up to 24 s
            const NpcFrame f = npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
            // Held created-actor combat contract: the feeding branch records the feeder as contact.
            for (const Npc &n : npcs.npcs())
                contactSeen |= n.fighter.controller != 5 && n.fighter.relations.contact == me.id;
            for (const HitEmission &h : f.playerHits)
                bites += h.record.kind == 6;
            for (const auto &t : f.tamed) {
                tamed = true;
                same(t.owner, 1, "tamed for the player");
                same(t.level, 3, "the level capped at the owner's");
            }
        }
        same(tamed, 1, "the collie is tamed");
        same(contactSeen, 1, "the feeding branch records the feeder as the creature's contact");
        same(bites, 5, "by five bites");
        const Npc *pet = npcs.petOf(me);
        same(pet && pet->fighter.controller == 5 && pet->fighter.ownerId == 1, 1, "a pet of the player");
        same(pet ? pet->fighter.multipliers[0] : 0, 100 * (1 + 0.5f * skillFactor(2)), "health multiplier 108.33", 1e-3);
    }
    // Held created-actor combat contract §6.2: after five bites without taming, the last kind-7
    // record is applied with zero value and time, then a kind-8 record from the feeder, each
    // also an event; and an owned-flag creature without an owner identifier is still fed.
    for (const bool ownerless : {false, true}) {
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
        me.health = maxHealth(me);
        me.equipment[12].type = 20, me.equipment[12].subtype = 19;
        me.action = 82;
        Body body;
        body.place({274.5, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.setNamedWorld(true);
        npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
        Npc *collie = npcs.npcs().empty() ? nullptr : &npcs.npcs().front();
        same(collie != nullptr, 1, "the feeding fixture spawns its creature");
        if (!collie)
            continue;
        if (ownerless) {
            collie->fighter.controller = 5;
            collie->fighter.ownerId = 0;
            bool fed = false;
            for (int i = 0; i < 1500 && !fed; i++) {
                npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
                fed = collie->fighter.action == 110;
            }
            same(fed, 1, "an owned-flag creature without an owner identifier is fed");
            continue;
        }
        bool near = false;
        for (int i = 0; i < 1500 && !near; i++) {
            npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
            near = collie->fighter.relations.contact == me.id;
        }
        same(near, 1, "the creature reaches the feeder");
        collie->bites = 5;
        collie->tameMs = 0;
        collie->tameProgress = 0;
        collie->fighter.action = 110;
        collie->fighter.actionMs = 9000; // past recovery, under the selection prefix's 10000 ms reset
        collie->fighter.buffs = {{7, .4f, 3000, 99}, {7, .6f, 4000, 98}};
        // Behaviour thinks every 32 ms, so a few 16 ms steps reach the continuation.
        std::vector<BuffEvent> records;
        for (int i = 0; i < 4 && records.empty(); i++) {
            const NpcFrame f = npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
            for (const BuffEvent &e : f.buffs)
                if (e.record.kind == 7 || e.record.kind == 8)
                    records.push_back(e);
        }
        same(records.size(), 2, "two record applications are emitted");
        if (records.size() == 2) {
            same(records[0].source == collie->fighter.id && records[0].target == collie->fighter.id, 1,
                 "events name the creature twice");
            same(records[0].record.kind, 7, "the kind-7 record comes first");
            same(records[0].record.source, 98, "the last kind-7 entry wins");
            same(records[0].record.value, 0, "its value is zeroed");
            same(records[0].record.timeMs, 0, "its time is zeroed");
            same(records[1].record.kind, 8, "then the kind-8 record");
            same(records[1].record.timeMs, 20000, "lasting 20000 ms");
            same(records[1].record.source, 1, "from the feeder");
        }
        same(collie->fighter.hasBuff(8), 1, "the creature is pacified");
    }
    // Cast-point contract §1.1, §2.2: during a close spell (0x1E-0x21) a magic user latches its
    // target's follow point while (clock - step) <= the anticipation time, then aims at it.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 4, s.level = 1; // the hunting fixture's creature
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({270.5, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
        Npc *mage = npcs.npcs().empty() ? nullptr : &npcs.npcs().front();
        same(mage != nullptr, 1, "the cast-point fixture spawns its creature");
        if (mage) {
            mage->fighter.characterClass = 3;
            mage->fighter.mana = 1;
            for (int i = 0; i < 1500 && mage->target != me.id; i++)
                npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
            same(mage->target, 1, "the magic user targets the player");
            bool latched = false, held = false;
            const int64_t sentinel[3] = {body.position[0] + (5ll << 16), body.position[1], body.position[2]};
            for (int i = 0; i < 400 && !(latched && held); i++) {
                const bool late = latched;
                if (!late) {
                    std::copy(sentinel, sentinel + 3, body.follow);
                    mage->fighter.action = 30, mage->fighter.actionMs = 0;
                } else {
                    mage->fighter.action = 30;
                    mage->fighter.actionMs = (float)phases(mage->fighter, 30).windUp + 40;
                }
                npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
                if (!late && mage->castPoint[0] == sentinel[0])
                    latched = true;
                if (late && mage->fighter.action == 30 && mage->target == me.id) {
                    const float dx = (float)(mage->castPoint[0] - mage->body.position[0]) / 65536.f;
                    held = std::fabs(mage->body.look.x - dx) < 1e-3f;
                }
            }
            same(latched, 1, "the cast point copies the target's follow point");
            same(held, 1, "past the anticipation time the aim is the held cast point");
            same(body.follow[0], body.position[0], "a live player's follow point is its position after the update");

            // Aim-triplet contract §3.3: a creature's shot flies along its normalized aim.
            int shots = 0, along = 0;
            for (int i = 0; i < 1500 && shots < 3; i++) {
                // The player sidesteps every update, so the aim (written at the 32 ms
                // behaviour cadence) differs from the player's current offset.
                body.position[1] += ((i / 10) % 2 ? -1 : 1) * (1ll << 14);
                const NpcFrame f = npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor);
                for (const Projectile &p : f.shots)
                    if (p.shooter == mage->fighter.id && glm::dot(p.velocity, p.velocity) > 0) {
                        shots++;
                        const glm::vec3 aim = mage->body.look;
                        along += glm::length(glm::normalize(p.velocity) - glm::normalize(aim)) < 1e-4f;
                    }
            }
            same(shots > 0, 1, "the magic user shoots");
            same(along, shots, "every shot flies along the shooter's aim");
        }
    }
    // Approach-path contract §6, §8 with combat §5.2: once a wall hides the target, the far route
    // requests a path with the lowered endpoints; when the wall goes, the direct walk drops it.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 4, s.level = 1;
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        bool wall = false;
        auto ground = [&](int32_t x, int32_t, int32_t z) -> uint8_t {
            if (z < 1)
                return 1;
            return wall && x == 274 && z < 6 ? 1 : 0;
        };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({277.5, 10.5, 1 + 1.08 + 0.01});
        body.contact |= ON_GROUND;
        Npcs npcs;
        const auto step = [&] { return npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, ground); };
        step();
        Npc *chaser = npcs.npcs().empty() ? nullptr : &npcs.npcs().front();
        same(chaser != nullptr, 1, "the approach fixture spawns its creature");
        if (chaser) {
            chaser->fighter.characterClass = 1; // melee reach, so the ledge is out of range
            for (int i = 0; i < 1500 && chaser->target != me.id; i++)
                step();
            same(chaser->target, 1, "the creature targets the player");
            wall = true;
            const search::State &path = chaser->path.state;
            for (int i = 0; i < 200 && path.records.empty() && path.path.empty(); i++)
                step();
            same(!path.records.empty() || !path.path.empty(), 1, "the blocked walk requests a path");
            same(path.radius, chaser->body.size.x + body.size.x, "the arrival radius is both widths");
            const int64_t lowered = int64_t((body.size.z * .5f - std::bit_cast<float>(uint32_t(0x3dcccccd))) * 65536.f);
            same(path.goal[2], body.position[2] - lowered, "the goal is lowered by half the target height less 0.1");
            same(path.goal[0], body.position[0], "the goal keeps the target's x");
            wall = false;
            bool dropped = false;
            for (int i = 0; i < 20 && !dropped; i++) {
                step();
                dropped = path.records.empty() && path.path.empty();
            }
            same(dropped, 1, "a clear direct walk drops the search");
        }
    }
    // Aim-triplet contract §3.7, §3.8: an aiming creature turns toward its drawn aim at rate 0.1
    // per millisecond (16 ms closes about 81% of a small gap) instead of snapping to it.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 4, s.level = 1;
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({268.4, 10.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        const auto step = [&] { return npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor); };
        step();
        Npc *turner = npcs.npcs().empty() ? nullptr : &npcs.npcs().front();
        same(turner != nullptr, 1, "the facing fixture spawns its creature");
        if (turner) {
            turner->fighter.characterClass = 1;
            for (int i = 0; i < 1500 && !(turner->target == me.id && (turner->body.mode & AIMING)); i++)
                step();
            same((turner->body.mode & AIMING) != 0, 1, "the creature in reach aims at the player");
            const auto heading = [&] {
                const glm::vec3 d = turner->drawnLook;
                return float(std::atan2(-double(d.x), double(d.y)) * 180.0 / 3.141592653589793);
            };
            const auto gap = [&] {
                float g = turner->yaw - heading();
                while (g > 180) g -= 360;
                while (g < -180) g += 360;
                return std::fabs(g);
            };
            turner->yaw = heading() + 60;
            step();
            same(gap() > 5 && gap() < 20, 1, "one 16 ms update closes most, not all, of a 60 degree gap");
            for (int i = 0; i < 60; i++)
                step();
            same(gap() < 2, 1, "the creature ends up facing its drawn aim");
        }
    }
    // Aim-triplet contract §3.1: a beam's first pulse comes on the update that crosses the
    // wind-up, then every 200 ms of (clock - wind-up). A pulse that meets nobody leaves sparks.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 266ll << 16, s.y = 10ll << 16, s.z = 1ll << 16;
        s.type = 4, s.level = 1;
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto floor = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 1 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        body.place({300.5, 10.5, 1 + 1.08 + 0.01}); // far off the beam's line
        Npcs npcs;
        const auto step = [&] { return npcs.step(16, map, {{me, body, 90, me.actionMs}}, 43200000, 0, floor); };
        step();
        Npc *caster = npcs.npcs().empty() ? nullptr : &npcs.npcs().front();
        same(caster != nullptr, 1, "the beam fixture spawns its creature");
        if (caster) {
            const Phases timing = phases(caster->fighter, 95);
            same(timing.windUp > 8 && timing.active > 300, 1, "the beam has a wind-up and an active window");
            const auto pulse = [&](int32_t clock, float mana = 1.f, int32_t action = 95) {
                caster->thinkMs = 0; // no behaviour decision this update
                caster->fighter.action = action;
                caster->fighter.actionMs = clock;
                caster->fighter.mana = mana;
                caster->body.look = glm::vec3(0, 10, 0);
                return !step().bursts.empty();
            };
            same(pulse(timing.windUp - 8), 1, "the update crossing the wind-up pulses");
            same(caster->fighter.mana < 0.71f, 1, "action 0x5F pays 0.3 mana at its first crossing");
            same(pulse(timing.windUp + 20), 0, "no pulse inside the same 200 ms period");
            caster->fighter.deferred = {99};
            same(pulse(timing.windUp + 190), 1, "the next 200 ms boundary pulses");
            // Open points 1 §3.1 step 4: a tick clears the evaded set (+0x11B4) too.
            same(caster->fighter.deferred.empty(), 1, "the beam tick clears the evaded set");
            same(pulse(timing.windUp + timing.active), 0, "no pulse once the active window is over");
            // Beam-leftovers contract §1.4: mana driven below zero skips the rest of the case.
            same(pulse(timing.windUp - 8, 0.2f), 0, "a first crossing without the mana loses its pulse");
            same(pulse(timing.windUp + 190, 0.f), 1, "with mana at zero the 200 ms repeats still pulse");
            const int32_t rayWindUp = phases(caster->fighter, 28).windUp;
            same(pulse(rayWindUp - 8, 0.f, 28), 0, "action 0x1C with no mana left drains below zero and stops");
            same(pulse(rayWindUp - 8, 1.f, 28), 1, "action 0x1C with mana pulses");
        }
    }
    // Pets §10.2 runs Companion (client 0x03ACE0, held created-companion navigation): with the
    // owner behind a wall, a pet starts a search toward the owner's ground with radius 2.
    {
        auto zone = std::make_shared<worldgen::Zone>();
        // The direct walk reads zone columns: ground below z = 1, as the block lookup says.
        for (int32_t x = 0; x < worldgen::Zone::SIZE; x++)
            for (int32_t y = 0; y < worldgen::Zone::SIZE; y++)
                zone->column(x, y).bottom = 1;
        worldgen::WorldMap map;
        map.add(zone);
        bool wall = false;
        auto ground = [&](int32_t x, int32_t, int32_t z) -> uint8_t {
            return z < 1 || (wall && x == 36 && z < 6) ? 1 : 0;
        };
        Combatant me;
        me.id = 1;
        me.level = 5;
        me.health = maxHealth(me);
        me.equipment[12].type = 19, me.equipment[12].subtype = 19, me.equipment[12].level = 3;
        Body body;
        body.place({30.5, 30.5, 1 + 1.08 + 0.01});
        Npcs npcs;
        npcs.setNamedWorld(true);
        auto step = [&] { return npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, ground); };
        step();
        same(npcs.petOf(me) != nullptr, 1, "the walled pet fixture makes a pet");
        wall = true;
        for (int32_t y = 0; y < worldgen::Zone::SIZE; y++)
            zone->column(36, y).blocks.assign(5, worldgen::Block{200, 200, 200, worldgen::SOLID});
        body.place({42.5, 30.5, 1 + 1.08 + 0.01});
        const Npc *pet = nullptr;
        bool searching = false;
        for (int i = 0; i < 10 && !searching; i++) {
            step();
            pet = npcs.petOf(me);
            searching = pet && pet->path.state.radius == 2.f && !pet->path.state.records.empty();
        }
        same(searching, 1, "a blocked pet searches toward its owner with radius 2");
        if (pet) {
            same(pet->path.state.goal[0], body.position[0], "the search goal keeps the owner's x");
            // The held goal scan moves the owner's raw height by whole blocks (down to ground,
            // then up to the first open cell), keeping its fraction; it is not a cell floor.
            same(pet->path.state.goal[2], body.position[2] - 65536, "the goal is one block below the owner, fraction kept");
        }
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
        me.relations.contact = 77, me.relations.counts[77] = 1;
        npcs.callPet(me, body.blocks());
        pet = npcs.petOf(me);
        same(pet && glm::length(glm::dvec2(pet->body.blocks() - body.blocks())) < 0.01, 1, "T brings it from 270 blocks");
        same(me.relations.contact == 0 && me.relations.counts.empty(), 1,
             "server simulation §8 kind 8: the reset clears the player's target and hit-counter map");
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
        same(npcs.all()[0].inventory.groups.size() == 1 && npcs.all()[0].inventory.groups[0].size() == 1 && itemType(npcs.all()[0].inventory.groups[0][0].item) == 0 &&
             itemSubtype(npcs.all()[0].inventory.groups[0][0].item) == 0, 1, "the potion is used up without erasing its payload row");
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

    // Server simulation §2.1, §3 steps 2, 7, 14, §5.4, §6.3, §8 kind 2: spawn-object state, the
    // inn rest, removal from threat maps, target clean-up and the silent mission-target kill.
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn s;
        s.x = 300ll << 16, s.y = 10ll << 16, s.z = 10ll << 16;
        s.type = 2;
        s.controller = 3;
        // §3 step 7: the 10:00 entry overwrites the active block (300, 230).
        s.schedule = {{300ll << 16, 230ll << 16, 10ll << 16, 10 * 3600000}};
        zone->spawns.push_back(s);
        worldgen::WorldMap map;
        map.add(zone);
        auto flat = [](int32_t, int32_t, int32_t z) -> uint8_t { return z < 10 ? 1 : 0; };
        Combatant me;
        me.id = 1;
        me.health = maxHealth(me);
        Body body;
        const auto at = [&](double x, double y) { body.place({x, y, 10 + 1.08 + 0.01}); };
        Npcs npcs;
        const auto step = [&](double time, int32_t day = 0) {
            return npcs.step(16, map, {{me, body, 0, me.actionMs}}, time, day, flat);
        };
        const double night = 1 * 3600000.0;
        at(500, 120); // in the zone, beyond 200 blocks of both positions
        step(11 * 3600000.0);
        same(npcs.all().empty(), 1, "nobody near the scheduled position at 11:00");
        at(300, 240); // within 200 of the scheduled position, 230 from the raw one
        step(night);
        same(npcs.all().size() == 1 && std::abs(npcs.all()[0].home.y - 230) < 0.01, 1,
             "after midnight the spawn keeps its last scheduled active block");
        const uint64_t id = Npcs::spawnId(1, 0, 0);

        // §8 kind 2: the inn deletes spawn NPCs (their countdown set to 0) and keeps a bomb.
        npcs.markFatalBacking(npcs.all()[0].fighter, map, 0); // countdown 1,200,000, died today
        const uint64_t bomb = npcs.placeBomb(body.blocks() + glm::dvec3(5, 0, 0), 1);
        npcs.restAll(map);
        same(npcs.all().size() == 1 && npcs.all()[0].fighter.id == bomb, 1, "the rest deletes the spawn NPC only");
        npcs.npcs().clear(); // (the bomb is not under test)
        step(night);
        same(npcs.all().size(), 1, "its countdown was reset, so it comes back the same day");

        // §3 step 14: a player whose target names a dead creature forgets it.
        me.relations.contact = id;
        npcs.npcs()[0].fighter.health = 0;
        step(night);
        same(me.relations.contact, 0, "the target naming a dead creature is cleared");
        // §6.3: the removed corpse leaves the player's threat and hit-counter maps.
        me.relations.threats[id] = 3, me.relations.counts[id] = 1;
        for (int i = 0; i < 70 && !npcs.all().empty(); i++)
            step(night);
        same(npcs.all().empty() && !me.relations.threats.count(id) && !me.relations.counts.count(id), 1,
             "a deleted corpse is stripped from every threat and hit-counter map");

        // §5.4: a despawned NPC leaves the player's threat map (its hit counter stays).
        step(night, 1); // (the next day: back from its spawn)
        same(npcs.all().size(), 1, "back the next day");
        me.relations.threats[id] = 2, me.relations.counts[id] = 4;
        at(300, 500); // beyond 220 blocks; the 3 x 3 round zone (1, 1) still holds (1, 0)
        step(night, 1);
        same(npcs.all().empty() && !me.relations.threats.count(id) && me.relations.counts.count(id), 1,
             "despawn strips threat maps only");

        // §3 step 2: a completed mission's target nobody fights dies silently and stays a corpse.
        at(300, 240);
        step(night, 1);
        same(npcs.all().size(), 1, "the creature is back");
        Missions missions;
        missions.edit(0, 0).mission.kind = 1;
        missions.edit(0, 0).mission.state = 2;
        npcs.npcs()[0].looks.flags |= 0x2000;
        npcs.npcs()[0].fighter.relations.threats.clear();
        bool routine = false;
        for (int i = 0; i < 100; i++) {
            npcs.killCompletedMissionTargets(missions);
            const NpcFrame f = step(night, 1);
            routine |= !f.deaths.empty() || !f.kills.empty() ||
                       std::any_of(f.sounds.begin(), f.sounds.end(), [](const SoundEvent &e) { return e.sound == 27; });
        }
        same(npcs.all().size() == 1 && npcs.all()[0].fighter.health == 0 && !routine, 1,
             "no death routine runs and the zeroed timer keeps the corpse");
    }

    // Hostility test §2.4: identity, role 6, one role 1, differing exemption, toggle bit 0x20.
    {
        Combatant player, other, monster, deer, object, tamed;
        player.controller = other.controller = 0;
        monster.controller = deer.controller = 1;
        monster.type = 12, deer.type = 19; // 19 is in the exempt list, 12 is not
        object.controller = 6;
        tamed.controller = 5;
        same(hostilityTest(object, object), 0, "the same object is never hostile, even role 6");
        same(hostilityTest(player, object) && hostilityTest(object, monster), 1, "role 6 is hostile to all");
        same(hostilityTest(player, monster) && hostilityTest(monster, player), 1, "exactly one role 1");
        same(hostilityTest(tamed, monster), 1, "role 5 against role 1");
        same(hostilityTest(player, other) || hostilityTest(tamed, player), 0, "non-1 roles are not hostile");
        same(hostilityTest(monster, deer), 1, "role 1 against an exempt role 1");
        deer.appearanceFlags = 0x0800; // bit 11 removes the exemption
        same(hostilityTest(monster, deer), 0, "two unexempt role 1");
        other.mode = 0x20;
        same(hostilityTest(player, other) && hostilityTest(other, player), 1, "toggle bit 0x20 on either");
        player.health = -5;
        same(hostilityTest(player, monster), 1, "health is not read");

        // Target relation §5: role 5 never turns on role 0 (even toggled), the reverse falls to
        // the toggle; role 6 is never a target; no identity rule (a toggled creature relates to
        // itself).
        tamed.mode = 0x20;
        same(hostile(tamed, player), 0, "role 5 against role 0 is never");
        same(hostile(player, tamed), 1, "role 0 against a toggled role 5 is the toggle");
        same(hostile(monster, object) || hostile(object, monster), 0, "role 6 is never a target");
        same(hostile(monster, player) && hostile(player, monster), 1, "one role 1 in either order");
        same(hostile(tamed, tamed), 1, "the same creature relates by its toggle");
        same(hostile(object, object), 0, "role 6 against itself");
    }

    // Combat helper tables (alpha-combat-helper-tables.md): the less obvious rows, pinned.
    {
        Combatant c;
        c.level = 5;
        // §3: B for 0x59 is scaled 600 with a bracelet (second-slot subtype 0x0C), else 1200.
        c.equipment[MAIN_HAND] = Item{3, 0x0C, 1, 0, 5, 0};
        const int32_t withBracelet = phases(c, 0x59).windUp;
        c.equipment[MAIN_HAND] = Item{3, 0x0F, 1, 0, 5, 0};
        same(std::abs(phases(c, 0x59).windUp - 2 * withBracelet) <= 1, 1, "0x59 wind-up halves with a bracelet");
        // §6.2 row 12: a two-hander chains 0x39 -> 0x43 -> 0x3A within R0 + 200, else 0x39.
        c.action = 0x39, c.actionMs = duration(c, 0x39) + 199;
        same(primaryAttack(c), 0x43, "0x39 chains to 0x43 within R0 + 200");
        c.action = 0x43, c.actionMs = duration(c, 0x43) + 199;
        same(primaryAttack(c), 0x3A, "0x43 chains to 0x3A within R0 + 200");
        c.actionMs = duration(c, 0x43) + 200;
        same(primaryAttack(c), 0x39, "past R0 + 200 it starts over");
        // §6.3 row 6: a two-hander's special alternates with the swing count's parity.
        c.combo = 3;
        same(secondaryAttack(c), 0x41, "odd count: 0x41");
        c.combo = -2;
        same(secondaryAttack(c), 0x42, "even count: 0x42");
        // §6.4: the ranged codes cost nothing with a kind-9 effect, 0.3 without.
        same(manaCost(c, 0x2E), 0.3f, "0x2E costs 0.3");
        // §8 step 4: a class (4, 1) roll (0x05) keeps its lockout and ignores it.
        Combatant rogue;
        rogue.characterClass = 4, rogue.specialization = 1, rogue.mana = 1;
        rogue.equipment[MAIN_HAND] = Item{3, 5, 1, 0, 5, 0};
        rogue.dodgeMs = 300;
        rogue.actionMs = 100000;
        same(startSecondaryAttack(rogue) && rogue.action == 5 && rogue.dodgeMs == 300, 1,
             "the exempt roll starts through a lockout and keeps it");
        rogue.characterClass = 1, rogue.action = 0, rogue.dodgeMs = 300;
        same(startSecondaryAttack(rogue), 0, "a lockout stops any other special");
    }

    // Beam-height §2.2: with a shield in the second hand, bar entry 1 is 8; holding it starts the
    // block at clock 0 and letting go turns it into 0x68 (104) at clock 0, with no further test.
    {
        Combatant s;
        s.characterClass = 1, s.level = 5;
        s.equipment[MAIN_HAND] = Item{3, 0, 1, 0, 5, 0};
        s.equipment[OFF_HAND] = Item{3, SHIELD, 1, 0, 5, 0};
        same(abilitySlots(s)[1], 8, "shield bar entry 1 is 8");
        Body body;
        useAbilities(s, body, {false, true, false, false, false, false}, glm::vec3(0));
        same(s.action, 8, "holding entry 8 blocks");
        s.actionMs = 900;
        s.mana = 0;
        useAbilities(s, body, {false, false, false, false, false, false}, glm::vec3(0));
        same(s.action, 104, "release becomes 0x68");
        same(s.actionMs, 0, "release clock 0");
    }

    // Beam-height §4.1: a beam effect record (kind 1, 5 repetitions) makes 5 particles from 35
    // draws, in the order r1..r6, u per particle.
    {
        Burst beam = Burst::at({0, 0, 0}, {0, 0, 0}, {1, 0.2f, 0.5f, 1}, 0.1f, 5, FIRE);
        int calls = 0;
        Particles list;
        list.expand(beam, [&] { return (calls++ * 1009) % 32768; });
        same(calls, 35, "beam record draws 35");
        same(list.list.size(), 5, "five beam particles");
        const auto r = [](int k) { return (float)((k * 1009) % 32768) / 32767.f; };
        same(list.list[1].velocity.z, (r(7) - 0.5f) * 6.f, "second particle z from draw 8", 1e-6);
        same(list.list[1].colour.g, r(13), "second particle green from draw 14", 1e-7);
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
        same(inv.canCarry(herb), 1, "capacity does not normalize the supplied quantity field");
        same(inv.canCarry(inv.bags[2][0].item), 0, "fifty matching stored items is the limit");
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

    // World items (re-certified) §4.3: the settling pass lowers generated records with tag bit 0
    // clear onto solid ground (through water); tag-1 loot stays. §2.4: the tumble runs while
    // T != 0, the hop only while T > 0. §2 step 5: the mana-cube filter.
    {
        worldgen::Zone zone;
        zone.zx = 0, zone.zy = 0;
        for (auto &column : zone.columns)
            column.bottom = 5; // implicit solid below z = 5, air above
        zone.items.push_back({20, 20, 12, 11, 15, 0, 1});              // vegetation, tag 2
        worldgen::ItemDrop loot;
        loot.x = 30ll << 16, loot.y = 30ll << 16, loot.z = 12ll << 16; // scale 0.06, tag 1
        loot.item.type = 11;
        zone.drops.push_back(loot);
        loot.scale = 0.1f; // tag 2
        zone.drops.push_back(loot);
        WorldItems world;
        world.generated(zone);
        const std::vector<WorldItem> &list = *world.zone(0, 0);
        same(list[0].origin == 1 && list[0].position[2] == (12ll << 16), 1, "tag-1 loot is not moved");
        same(list[1].origin == 2 && list[1].position[2] == (5ll << 16), 1, "tag-2 loot settles on the ground");
        same(list[2].position[2], 5ll << 16, "a vegetation item settles block by block");

        WorldItem w;
        w.item[0] = 5;
        const glm::vec3 size(4, 4, 4);
        WorldItem rest = w;
        const glm::mat4 still = worldItemPose(rest, size, 0);
        w.landingMs = -3600; // whole turns of both tumble angles (3600 and 2520 degrees)
        same(std::abs(worldItemPose(w, size, 0)[3][2] - still[3][2]) < 1e-3f, 1, "no hop while T <= 0");
        w.landingMs = 3600;
        same(std::abs(worldItemPose(w, size, 0)[3][2] - still[3][2]) > 1, 1, "the hop while T > 0");
        w.landingMs = -40; // a received, overshot timer
        same(worldItemPose(w, size, 0) != still, 1, "the tumble still turns the item while T != 0");

        ItemRecord cube{};
        cube[0] = 25;
        const int32_t number = 107;
        std::memcpy(cube.data() + 4, &number, 4);
        same(manaCubeFor(cube, 107), 1, "the cube of counter 107");
        same(manaCubeFor(cube, 207), 1, "a counter above 100 reads as 100 + counter mod 100");
        same(manaCubeFor(cube, 106), 0, "another player's mana cube is hidden");
        ItemRecord other{};
        other[0] = 3;
        same(manaCubeFor(other, 0), 1, "other types pass");
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
        statics.step(25000, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &, int32_t &) { return false; }, {});
        use(1);
        same((*statics.zone(0, 0))[1].state, 0, "the door opens");
        (void)statics.takeChanged();
        statics.step(500, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &, int32_t &) { return false; }, {});
        same(openFraction(1, (*statics.zone(0, 0))[1]), 0.5, "half open after half a second", 1e-6);
        use(2);
        same(body.blocks().z, 101.4, "seated on the stool", 1e-4);
        same(yaw, 270, "facing (1 + 2) x 90");
        same(me.action, 83, "sitting");
        std::map<StaticStates::Key, const worldgen::Zone *> held{{{0, 0}, &zone}};
        const auto where = [&](uint64_t, glm::dvec3 &at, glm::vec3 &intent, int32_t &action, int32_t &stun) {
            at = body.blocks(), intent = body.intent, action = me.action, stun = me.hitStun;
            return true;
        };
        statics.step(16, where, held);
        same((*statics.zone(0, 0))[2].occupant, 1, "still seated");
        // Open points 1 §1.6: a stunned occupant releases the seat (its action unchanged), and
        // only a non-remote world releases.
        me.hitStun = 500;
        statics.step(16, where, held, false);
        same((*statics.zone(0, 0))[2].occupant, 1, "a remote world keeps the occupant");
        statics.step(16, where, held);
        same((*statics.zone(0, 0))[2].occupant, 0, "hit-stun frees the stool");
        same(me.action, 83, "the release leaves the action alone");
        me.hitStun = 0;
        use(2);
        same((*statics.zone(0, 0))[2].occupant, 1, "seated again");
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
        // Loot rolls §2: once hi - lo + 1 exceeds 32767 the result is lo + d (first draws 440
        // and 175 from seeds 123 and 42).
        {
            worldgen::Random a(123), b(42);
            const int16_t la = rerollLevel(a, 1000), lb = rerollLevel(b, 1000);
            same(la - lb, 440 - 175, "wide re-roll is lo + d");
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

        // Crafting §5.3 step 2: over-removal works per bag with the same n in every bag. Bag 0
        // holds 2, 5, 2 and bag 1 holds 1, 4 equal cubes; a need of 3 empties the 2, takes 3 of
        // the 5 and leaves the later 2; in bag 1 it empties the 1 and takes 3 of the 4.
        {
            Inventory bags = Inventory::fresh();
            for (auto &bag : bags.bags)
                for (Stack &s : bag)
                    s = {};
            ItemRecord cube{};
            cube[0] = 11, cube[1] = 10, cube[0x0d] = 1;
            creatures::setItemLevel(cube, 1);
            bags.bags[0][0] = {2, cube}, bags.bags[0][1] = {5, cube}, bags.bags[0][2] = {2, cube};
            bags.bags[1][0] = {1, cube}, bags.bags[1][1] = {4, cube};
            Recipe use;
            use.product[0] = 1, use.product[1] = 5;
            use.ingredients = {{cube, 3}};
            same(craft(bags, use, [] { return 1; }).made.has_value(), 1, "made");
            // (The product may land in an emptied slot, so count only cubes.)
            const auto cubes = [&](size_t b, size_t s) {
                const Stack &st = bags.bags[b][s];
                return st.count > 0 && sameItem(st.item, cube) ? st.count : 0;
            };
            same(cubes(0, 0) == 0 && cubes(0, 1) == 2 && cubes(0, 2) == 2, 1, "bag 0: 2 emptied, 5 -> 2, the later 2 untouched");
            same(cubes(1, 0) == 0 && cubes(1, 1) == 1, 1, "bag 1: the same n again (1 emptied, 4 -> 1)");
        }
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

    // Head-pitch-and-pick contract: the pick's light-scaled reach, the heal target and the
    // healing stream's world case.
    {
        namespace ca = controller_aim;
        same(ca::daylight(43200000), 1, "noon gives full daylight");
        same(ca::daylight(0) < 1e-6f, 1, "midnight gives no daylight");
        same(ca::pickRange(255, 1.f), 256, "full light reaches 256");
        same(ca::pickRange(0, 1.f), std::bit_cast<float>(0x41CCCCCDu), "darkness reaches 25.6");
        same(ca::pickRange(51, 1.f) < 256.f && ca::pickRange(51, 1.f) > 25.6f, 1, "dim light reaches between");
        Combatant caster;
        caster.id = 7;
        caster.action = 0x22, caster.actionMs = 0;
        ca::AimPoint aim;
        aim.nearest = 9;
        ca::writeHealTarget(caster, aim, false);
        same(caster.healTarget, 9, "action 0x22 before its wind-up end targets the nearest straddling creature");
        ca::writeHealTarget(caster, aim, true);
        same(caster.healTarget, 7, "Left Shift targets the caster itself");
        caster.actionMs = float(phases(caster, 0x22).windUp) + 1;
        caster.healTarget = 0;
        ca::writeHealTarget(caster, aim, false);
        same(caster.healTarget, 0, "after the wind-up the target is kept");

        // The world case: a pulse on an eighth boundary of the active window heals the target and
        // pays the action cost.
        Combatant healer;
        healer.id = 1;
        healer.characterClass = 3, healer.specialization = 1;
        healer.health = maxHealth(healer);
        healer.mana = 1;
        healer.action = 0x22;
        const Phases t = phases(healer, 0x22);
        const int32_t period = int32_t(float(t.active) * 0.125f);
        healer.actionMs = t.windUp + period - 8;
        Body body;
        body.place({0, 0, 3});
        Npcs npcs;
        Npc friendNpc;
        friendNpc.fighter.id = 2;
        friendNpc.fighter.controller = 3;
        friendNpc.fighter.health = maxHealth(friendNpc.fighter) * 0.5f;
        friendNpc.body.place({1, 0, 3});
        friendNpc.home = friendNpc.body.blocks();
        npcs.npcs().push_back(friendNpc);
        healer.healTarget = 2;
        PlayerView view{healer, body, 0, healer.actionMs};
        NpcFrame frame;
        const float before = npcs.all()[0].fighter.health;
        npcs.dispatchPlayerActions({&view, 1}, 16, [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); },
                                   frame);
        same(npcs.all()[0].fighter.health > before, 1, "the healing stream heals its target on a pulse");
        same(healer.mana, 1.f - manaCost(healer, 0x22), "the pulse pays the action cost", 1e-6);
        const float healed = npcs.all()[0].fighter.health;
        view.actionMsBefore = healer.actionMs = t.windUp + period + 20;
        npcs.dispatchPlayerActions({&view, 1}, 16, [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); },
                                   frame);
        same(npcs.all()[0].fighter.health, healed, "no heal inside the same period");
    }
    // Stat-queries contract §6: the B, C and D tables for every action 0..0x69 (R0 = B + C + D).
    {
        struct Entry {
            int32_t ms;
            bool fixed;
        };
        using Table = std::vector<std::pair<std::vector<int>, Entry>>;
        const Table B = {
            {{0x00, 0x08, 0x0B, 0x1C, 0x32, 0x37, 0x3C, 0x3D, 0x3E, 0x60, 0x62, 0x68}, {0, true}},
            {{0x30, 0x65}, {100, true}}, {{0x47, 0x48}, {200, true}}, {{0x36}, {400, true}}, {{0x69}, {5000, true}},
            {{0x06, 0x07, 0x17, 0x18, 0x19, 0x1B, 0x24, 0x3B, 0x3F, 0x40}, {50, false}},
            {{0x02, 0x03, 0x04, 0x05, 0x0A, 0x0E, 0x11, 0x12, 0x13, 0x14, 0x15}, {100, false}},
            {{0x0C, 0x0D, 0x0F, 0x10, 0x27, 0x28, 0x29, 0x2A, 0x43}, {200, false}},
            {{0x01, 0x09, 0x1A, 0x41, 0x42, 0x44, 0x45, 0x46, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E}, {300, false}},
            {{0x26, 0x2C, 0x5E}, {500, false}}, {{0x25, 0x2B, 0x59}, {1200, false}},
            {{0x1E, 0x20, 0x39, 0x3A, 0x5D}, {800, false}}, {{0x5B, 0x5F}, {1000, false}}, {{0x2D, 0x2E}, {1200, false}},
            {{0x1F, 0x21, 0x22}, {1600, false}}, {{0x57}, {5000, false}}};
        const Table C = {
            {{0x00, 0x31}, {0, true}}, {{0x36}, {100, true}}, {{0x0A}, {200, true}},
            {{0x32, 0x4C, 0x4D, 0x4E, 0x60}, {500, true}}, {{0x1E, 0x20, 0x49, 0x5D}, {600, true}},
            {{0x44, 0x45}, {1000, true}}, {{0x1F, 0x21}, {1200, true}}, {{0x5F}, {2000, true}}, {{0x47}, {3000, true}},
            {{0x48, 0x56}, {5000, true}}, {{0x5B}, {6000, true}}, {{0x16, 0x17}, {50, false}},
            {{0x03, 0x04, 0x25, 0x2B, 0x3E}, {100, false}}, {{0x06, 0x07, 0x12, 0x13}, {150, false}},
            {{0x01, 0x02, 0x09, 0x0D, 0x0E, 0x0F, 0x43}, {200, false}},
            {{0x05, 0x0C, 0x10, 0x11, 0x14, 0x15, 0x41, 0x42}, {400, false}}, {{0x2D, 0x2E, 0x37, 0x5E}, {500, false}},
            {{0x1A, 0x22}, {1200, false}}};
        const Table D = {
            {{0x30}, {0, true}}, {{0x00, 0x32, 0x60}, {100, true}}, {{0x47, 0x48}, {200, true}}, {{0x36}, {400, true}},
            {{0x0A}, {600, true}}, {{0x17}, {10, false}},
            {{0x0B, 0x16, 0x1A, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
              0x3C, 0x3D, 0x43, 0x5E, 0x5F, 0x68},
             {100, false}},
            {{0x07, 0x0E, 0x12, 0x41, 0x42}, {200, false}}, {{0x03, 0x04, 0x05, 0x39, 0x3A, 0x3E}, {300, false}},
            {{0x0F}, {400, false}}, {{0x44, 0x45, 0x5D}, {800, false}}};
        const auto lookup = [](const Table &t, int action, Entry fallback) {
            for (const auto &[actions, e] : t)
                if (std::find(actions.begin(), actions.end(), action) != actions.end())
                    return e;
            return fallback;
        };
        Combatant c;
        c.multipliers[1] = 2; // halves every scaled time, so fixed and scaled differ
        const float q = attackSpeed(c) * c.multipliers[1];
        const auto ms = [&](Entry e) { return e.fixed ? e.ms : int32_t(float(e.ms) / q); };
        int wrong = 0;
        for (int action = 0; action <= 0x69; action++) {
            const Phases p = phases(c, action);
            const int32_t b = ms(lookup(B, action, {400, false})), cc = ms(lookup(C, action, {300, false}));
            const int32_t d = ms(lookup(D, action, {500, false}));
            if (p.windUp != b || p.active != cc || p.recovery != d) {
                wrong++;
                std::printf("  action 0x%02X: engine %d/%d/%d, contract %d/%d/%d\n", action, p.windUp, p.active,
                            p.recovery, b, cc, d);
            }
        }
        same(wrong, 0, "every action's B, C and D follow the held tables");
        c.equipment[MAIN_HAND].subtype = BRACELET; // +0xAA9 = 12, kind not tested
        same(phases(c, 0x25).windUp, int32_t(600.f / q), "B for 0x25 with subtype 12 in the second hand is scaled 600");
    }
    // Fatal-disposal contract §5: the loot helper's draw counts.
    {
        const auto consumed = [](worldgen::Random used, worldgen::Random fresh, int count) {
            for (int i = 0; i < count; i++)
                fresh.draw();
            return used.draw() == fresh.draw() && used.draw() == fresh.draw();
        };
        worldgen::Random a(31), b(31);
        worldgen::weaponItem(a, 10, 1, -1);
        same(consumed(a, b, 16), 1, "generator P (weapons) with selector -1 makes 16 draws");
        worldgen::Random c(32), d(32);
        worldgen::armourItem(c, 10, 1, -1);
        same(consumed(c, d, 24), 1, "generator Q (armour) with selector -1 makes 24 draws");
        // L2 for a boss: the r mod 4 draw is still made. A non-hostile kind with worn gear and
        // no listed race: L1 1, L2 1 + 2 + 1 + 1, a ground drop with no zone 1, L4 1.
        LootVictim boss;
        boss.controller = 2;
        boss.type = 1;
        boss.appearanceFlags = 0x200;
        boss.equipment[6][0] = 4;
        WorldItems none;
        worldgen::Random e(33), f(33);
        dropLoot(boss, none, 0, [](int32_t, int32_t, int32_t) { return uint8_t(1); }, e);
        same(consumed(e, f, 8), 1, "a boss's worn-item stage still makes its r mod 4 draw");
    }
    // Controller aim contract (alpha-controller-aim-writers.md).
    {
        namespace ca = controller_aim;
        Combatant me;
        me.id = 1;
        me.controller = 0;
        // §4: the flags block. Idle (action 0, R0 100): face-aim and walking for 300 ms.
        me.action = 0;
        me.actionMs = 299;
        uint32_t m = ca::flags(RUN, me, {});
        same((m & AIMING) != 0 && !(m & RUN), 1, "idle under R0+200: face-aim set, bit 0x40 cleared");
        me.actionMs = 301;
        m = ca::flags(AIMING, me, {});
        same(!(m & AIMING) && (m & RUN) != 0, 1, "idle past R0+200: face-aim cleared, running");
        me.actionMs = 50;
        m = ca::flags(0, me, {true, true});
        same(!(m & AIMING) && !(m & RUN) && (m & CLIMB) != 0, 1, "Left Shift: no face-aim, walking; Left Ctrl: bit 0x01");
        me.action = 0x22; // excluded
        m = ca::flags(0, me, {});
        same(!(m & AIMING) && (m & RUN) != 0, 1, "an excluded action keeps the face-aim bit clear");
        me.action = 0x1C, me.actionMs = 100000; // sustained
        same((ca::flags(0, me, {}) & AIMING) != 0, 1, "a sustained action keeps the face-aim bit past R0+200");
        me.action = 0x58, me.actionMs = 0;
        m = ca::flags(RUN, me, {});
        same(!(m & AIMING) && !(m & RUN), 1, "action 0x58: no face-aim, but bit 0x40 cleared");
        me.action = 0, me.actionMs = 5000;
        same((ca::flags(0, me, {false, false, true, false}) & RUN) == 0 && (ca::flags(0, me, {false, false, true, true}) & RUN) != 0,
             1, "gliding route: bit 0x40 is the lift");

        // §3.4: free control.
        const ca::Fixed3 at{10 << 16, 20 << 16, 3 << 16};
        const glm::vec3 offset = ca::freeAim(false, 0, {13 << 16, 16 << 16, 3 << 16}, at);
        same(offset.x == 3 && offset.y == -4 && offset.z == 0, 1, "free aim: the offset to the aim point");
        const glm::vec3 forward = ca::freeAim(true, 90, {}, at);
        same(forward.x, -1, "Left Shift aim: forward of yaw 90, x", 1e-6);
        same(forward.y, 0, "Left Shift aim: forward of yaw 90, y", 1e-6);

        // §3.2: the pick. A view looking along +y from the origin (view x = x, y = z, depth = y).
        ca::CameraState cam;
        cam.forward = {0, 1, 0};
        cam.view = camera_view::identity();
        cam.view[1] = {0, 0, 1, 0};
        cam.view[2] = {0, 1, 0, 0};
        const visibility::Scene empty{[](int32_t, int32_t, int32_t) { return uint8_t(0); },
                                      [](int32_t, int32_t, const visibility::RecordVisitor &) {}};
        Combatant ahead, behind, aside, friendly;
        ahead.id = 2, ahead.controller = 1;
        behind.id = 3, behind.controller = 1;
        aside.id = 4, aside.controller = 1;
        friendly.id = 5, friendly.controller = 3;
        const auto fixed = [](double x, double y, double z) {
            return ca::Fixed3{int64_t(x * 65536), int64_t(y * 65536), int64_t(z * 65536)};
        };
        const ca::Fixed3 player = fixed(0, -2, 0);
        me.action = 0, me.actionMs = 0;
        const std::vector<ca::Candidate> crowd = {{&behind, fixed(0, -20, 0), glm::vec3(1)},
                                                   {&aside, fixed(10, 20, 0), glm::vec3(1)},
                                                   {&ahead, fixed(0, 20, 0), glm::vec3(1)}};
        const ca::AimPoint picked = ca::aimPoint(cam, player, me, crowd, empty);
        same(picked.hovered, 2, "the pick hovers the creature on the screen centre, not behind or aside");
        same(picked.point == fixed(0, 20, 0), 1, "the aim point moves to its depth from the eye");
        const float L = visibility::trace(cam.anchor, {0, 1, 0}, 60.f, 0, 1, empty);
        const ca::Fixed3 rayPoint = L >= 0.f ? camera_view::toFixed(glm::vec3(0, L, 0)) : fixed(0, 100, 0);
        const std::vector<ca::Candidate> calm = {{&friendly, fixed(0, 20, 0), glm::vec3(1)}};
        const ca::AimPoint unpicked = ca::aimPoint(cam, player, me, calm, empty);
        same(unpicked.hovered == 5 && unpicked.point == rayPoint, 1, "an unadmitted creature is hovered but keeps the ray point");
        const ca::AimPoint behindPlayer = ca::aimPoint(cam, fixed(0, 30, 0), me, crowd, empty);
        same(behindPlayer.point == rayPoint, 1, "a creature behind the player along the view does not move the aim");

        // §3.2: the close spell's point is held once clock - elapsed passes the wind-up.
        me.action = 0x1E;
        const float B = float(phases(me, 0x1E).windUp);
        ca::Fixed3 held{}, point = fixed(1, 1, 1);
        me.actionMs = B;
        ca::freezeSpellPoint(point, held, me, 16);
        same(held == fixed(1, 1, 1), 1, "spell point stored up to the wind-up");
        point = fixed(2, 2, 2);
        me.actionMs = B + 17;
        ca::freezeSpellPoint(point, held, me, 16);
        same(point == fixed(1, 1, 1), 1, "spell point held after the wind-up");
    }

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
