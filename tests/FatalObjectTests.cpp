#include "StaticStates.h"
#include "net/Protocol.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
template<class T> T read(const std::array<uint8_t, 88> &record, size_t offset) {
    T result;
    std::memcpy(&result, record.data() + offset, sizeof result);
    return result;
}
worldgen::StaticObject gate(int32_t association, uint8_t state = 1, int32_t timer = 250) {
    worldgen::StaticObject object;
    object.kind = 5;
    object.descriptorIndex = association;
    object.state = state;
    object.timer = timer;
    object.stateWord = 0xbaadf00du;
    object.x = (int64_t(1) << 56) + 3 * 65536;
    object.y = -(int64_t(1) << 56) - 5 * 65536;
    object.z = 3 * 65536;
    return object;
}
}

int main() {
    check(worldgen::StaticObject{}.descriptorIndex == -1 && worldgen::StaticObject{}.stateWord == 0,
          "new object associations retain the independent negative default and written common-state word");
    {
        auto held = std::make_shared<worldgen::Zone>(2, 3);
        auto otherKind = gate(7);
        otherKind.kind = 1;
        held->statics = {otherKind, gate(99), gate(7, 0), gate(7, 2), gate(7, 1, 900)};
        worldgen::WorldMap map;
        map.add(held);
        StaticStates states;
        states.generated(*held);
        NpcFrame frame;
        NpcFrame::Cursor cursor;
        check(states.fatal(map, {2, 3, 7}, frame), "a retained association admits the first qualifying object");
        const auto *current = states.zone(2, 3);
        check(current && (*current)[0].state == 1 && (*current)[1].state == 1 && (*current)[2].state == 0 &&
              (*current)[3].state == 0 && (*current)[3].timerMs == 750 && (*current)[3].stateWord == 0xbaadf00du &&
              (*current)[4].state == 1,
              "kind, association and nonzero state all gate the ordered scan without changing later matches");
        check(frame.staticStates.size() == 1 && read<int32_t>(frame.staticStates[0], 0) == 2 &&
              read<int32_t>(frame.staticStates[0], 4) == 3 && read<int32_t>(frame.staticStates[0], 8) == 3 &&
              frame.staticStates[0][0x40] == 0 && read<int32_t>(frame.staticStates[0], 0x44) == 750 &&
              read<uint32_t>(frame.staticStates[0], 0x48) == 0xbaadf00du,
              "the immediate snapshot carries its static-array index, changed state/timer and uninterpreted word");
        check(frame.sounds.size() == 1 && frame.sounds[0].sound == 51 && frame.sounds[0].pitch == 1 &&
              frame.sounds[0].volume == 1 &&
              frame.sounds[0].position.x == double(float(held->statics[3].x) * (1.f / 65536.f)) &&
              frame.sounds[0].position.y == double(float(held->statics[3].y) * (1.f / 65536.f)),
              "gate sound uses the object's raw position narrowed to binary32 before scaling, without a proximity test");
        check(states.takeChanged() == std::set<StaticStates::Key>{{2, 3}}, "the actual zone is marked for persistence");
        const auto added = frame.addedSince(cursor);
        check(added.staticStates.size() == 1 && frame.addedSince(cursor).staticStates.empty(),
              "frame views drain the immediate object output once");
        check(states.fatal(map, {2, 3, 7}, frame) && read<int32_t>(frame.staticStates[1], 8) == 4 &&
              states.zone(2, 3)->at(4).timerMs == 100,
              "the next invocation skips cleared state and selects the next matching record");
        check(!states.fatal(map, {2, 3, 7}, frame) && frame.staticStates.size() == 2 && frame.sounds.size() == 2,
              "exhausted matches leave output unchanged");

        std::vector<std::vector<uint8_t>> published;
        states.appendUpdates(map, frame.staticStates, {{2, 3}}, {}, published);
        check(published.size() == 5 && published[0] == std::vector<uint8_t>(frame.staticStates[0].begin(), frame.staticStates[0].end()) &&
              published[1] == std::vector<uint8_t>(frame.staticStates[1].begin(), frame.staticStates[1].end()),
              "publication preserves both immediate records in order and suppresses identical later zone records");
        net::WorldUpdate update;
        update.sections[5] = published;
        const auto decoded = net::WorldUpdate::decode(update.encode());
        check(decoded && decoded->sections[5] == published, "complete immediate and final static records survive the wire codec");
        published.clear();
        states.appendUpdates(map, frame.staticStates, {}, {{2, 3}}, published);
        check(published.size() == 7, "an explicit zone request retains its complete response after immediate output");
        states.step(20, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &) { return false; }, {{{2, 3}, held.get()}});
        published.clear();
        states.appendUpdates(map, frame.staticStates, {{2, 3}}, {}, published);
        check(published.size() == 7, "later state changes remain publishable after an earlier immediate snapshot");

        auto wire = states.record(*held, 3);
        const uint32_t replacement = 0xfedcba98u;
        std::memcpy(wire.data() + 0x48, &replacement, 4);
        states.apply(wire.data());
        check(read<uint32_t>(states.record(*held, 3), 0x48) == replacement && held->statics[3].descriptorIndex == 7,
              "received state-word bits survive later output without rewriting the retained descriptor association");
        const size_t capacity = frame.staticStates.capacity();
        frame.reset();
        cursor = {};
        check(frame.staticStates.empty() && frame.staticStates.capacity() == capacity && frame.addedSince(cursor).staticStates.empty(),
              "frame reset clears object output while retaining buffer capacity");
        auto copied = *held;
        for (int index = 0; index < 128; index++) copied.statics.push_back({});
        check(copied.statics[3].descriptorIndex == 7 && copied.statics[3].stateWord == 0xbaadf00du,
              "full engine object copies and storage growth preserve association and common state independently");
    }
    for (const auto &[old, expected] : std::array<std::pair<int32_t, int32_t>, 9>{{
         {-5, 1005}, {0, 1000}, {999, 1}, {1000, 0}, {1001, 0}, {INT32_MAX, 0},
         {INT32_MIN, 0}, {INT32_MIN + 1000, 0}, {INT32_MIN + 1001, INT32_MAX}}}) {
        auto held = std::make_shared<worldgen::Zone>(0, 0);
        held->statics = {gate(-1, 0xff, old)};
        worldgen::WorldMap map;
        map.add(held);
        StaticStates states;
        states.generated(*held);
        NpcFrame frame;
        check(states.fatal(map, {0, 0, -1}, frame) && states.zone(0, 0)->at(0).timerMs == expected,
              "negative associations remain eligible and timer inversion uses defined 32-bit wrap before its signed clamp");
    }
    for (const auto &[x, y, admitted] : std::array<std::tuple<int32_t, int32_t, bool>, 6>{{
         {0, 0, true}, {65535, 65535, true}, {-1, 0, false}, {65536, 0, false},
         {0, -1, false}, {0, 65536, false}}}) {
        auto held = std::make_shared<worldgen::Zone>(x, y);
        held->statics = {gate(0)};
        worldgen::WorldMap map;
        map.add(held);
        StaticStates states;
        states.generated(*held);
        NpcFrame frame;
        check(states.fatal(map, {x, y, 0}, frame) == admitted && frame.staticStates.size() == size_t(admitted),
              "cached-coordinate admission includes both endpoints and rejects out-of-range engine zones");
    }
    {
        worldgen::WorldMap map;
        auto held = std::make_shared<worldgen::Zone>(2, 3);
        held->statics = {gate(7)};
        map.add(held);
        StaticStates states;
        NpcFrame frame;
        check(!states.fatal(map, {2, 3, 7}, frame), "missing retained state does not manufacture an object record");
        std::vector<std::vector<uint8_t>> published;
        states.appendUpdates(map, {}, {{2, 3}}, {}, published);
        check(published.empty(), "publication omits a held zone with no retained state instead of manufacturing a zero record");
        states.generated(*held);
        held->statics.push_back(gate(8));
        states.appendUpdates(map, {}, {{2, 3}}, {}, published);
        check(published.size() == 1 && !states.fatal(map, {2, 3, 8}, frame),
              "provided storage mismatch publishes only the initialized prefix and never invents later object state");
        check(!states.fatal(map, {2, 4, 7}, frame) && !states.fatal(map, {2, 3, 99}, frame),
              "missing cached zones and missing associations produce no object event");
    }
    {
        auto held = std::make_shared<worldgen::Zone>(2, 3);
        held->statics = {gate(7)};
        worldgen::WorldMap map;
        map.add(held);
        StaticStates states;
        states.generated(*held);
        Npcs world;
        Npc target;
        target.fighter.id = 72;
        target.fighter.controller = 1;
        target.fighter.health = 1;
        target.fighter.actionMs = 77;
        target.fighter.backing = {2, 3, 7};
        target.fighter.linkedId = 73;
        target.body.place({10, 10, 3}); // current position is unrelated to the cached object zone
        world.npcs().push_back(target);
        target.fighter.id = 73;
        target.fighter.health = 17;
        world.npcs().push_back(target);
        NpcFrame frame;
        int calls = 0;
        world.onFatalObject = [&](const Combatant &fighter, NpcFrame &events) {
            check(fighter.health == 0 && fighter.actionMs == 0 && events.sounds.size() == 1 && events.sounds[0].sound == 27,
                  "object processing follows first death presentation and clock reset");
            states.fatal(map, fighter.backing, events);
        };
        world.onFatalLoot = [&](const Combatant &, const Body &, std::span<const ItemRecord>) {
            calls++;
            check(states.zone(2, 3)->at(0).state == 0 && frame.staticStates.size() == 1 &&
                  frame.sounds.size() == 2 && frame.sounds[1].sound == 51 && world.all()[1].fighter.health == 17,
                  "the actual fatal loot boundary observes changed object state, sound and snapshot before linked health changes");
        };
        HitRecord hit;
        hit.attacker = 999;
        hit.target = 72;
        hit.amount = 1;
        world.applyWorldHit(hit, {}, frame);
        world.applyWorldHit(hit, {}, frame);
        check(calls == 1 && frame.staticStates.size() == 1 && world.all()[1].fighter.health == 0 && frame.deaths.empty(),
              "fatal application completes object/loot/link work without replay or waiting for the corpse pass");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
