#include "creatures/RouteGeometry.h"
#include "creatures/SpatialBuckets.h"
#include "StaticStates.h"
#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

using namespace creatures::route;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
struct Fixture {
    Position start{8 * 65536, 8 * 65536, 100 * 65536}, target = start;
    Dimensions size{2, 2, 2};
    uint16_t flags = 0;
    int columns = 0;
    std::vector<float> roots;
    struct Sample { Position position; int axis; Dimensions direction; bool terrain; };
    std::vector<Sample> spatial;
    Scene scene;
    Fixture() {
        scene.column = [&](int32_t, int32_t) -> std::optional<Column> {
            columns++;
            return Column{-10000, 0, {}};
        };
        scene.spatial = [&](const Position &position, int axis, const Dimensions &direction, bool terrain) {
            spatial.push_back({position, axis, direction, terrain});
            return false;
        };
        scene.squareRoot = [&](float value) { roots.push_back(value); return float(std::sqrt(double(value))); };
    }
    Subject subject() { return {start, size, flags, {}}; }
    bool run(float radius = 1) { return direct(subject(), target, radius, scene); }
};
void proximityAndProgress() {
    {
        Fixture f;
        f.target[0] += 32768;
        check(f.run() && f.columns == 0 && f.spatial.empty() && f.roots.empty(),
              "strict initial proximity returns before geometry or square-root queries");
    }
    {
        Fixture f;
        f.target[0] += 65536;
        check(f.run() && f.columns == 0 && f.spatial.empty() && f.roots == std::vector<float>{1.5625f},
              "horizontal equality skips the initial shortcut but passes the first iteration after its budget query");
    }
    {
        Fixture f;
        f.target[2] += 3 * 65536;
        check(!f.run() && !f.spatial.empty(), "vertical equality with height plus radius is strictly rejected");
        Fixture below;
        below.target[2] += 3 * 65536 - 1;
        check(below.run() && below.columns == 0, "one raw unit below the vertical boundary admits the initial shortcut");
    }
    {
        Fixture f;
        f.target[0] += 1000 * 65536;
        const auto original = f.start;
        check(f.run() && f.start == original && f.spatial.size() == 153,
              "capped budget permits51 three-axis simulations and progress success without target arrival or actor mutation");
        check(f.roots.size() == 53 && f.roots[0] == 1000000.f,
              "long routes retain cap, budget and each iteration's separate normalization calls");
        for (size_t i = 0; i + 2 < f.spatial.size(); i += 3) {
            check(f.spatial[i].axis == 0 && f.spatial[i + 1].axis == 1 && f.spatial[i + 2].axis == 2 &&
                  f.spatial[i].direction == f.spatial[i + 2].direction &&
                  f.spatial[i + 2].position[2] == f.spatial[i + 1].position[2] - 46530,
                  "axis order retains one direction triplet and exact fixed vertical increment for the whole pass");
        }
    }
    {
        Fixture f;
        f.target[0] += 5 * 65536;
        f.flags = 0x100;
        f.scene.column = [&](int32_t, int32_t) -> std::optional<Column> { f.columns++; return std::nullopt; };
        check(!f.run(0) && f.columns == 18 && f.spatial.size() == 6,
              "missing terrain blocks each tentative axis and swimmer mode skips horizontal raised scans");
        for (const auto &sample : f.spatial)
            check(sample.axis == 2 && sample.terrain, "the third axis still calls the spatial service with retained terrain blockage");
    }
}
void raisedBoxes() {
    std::vector<std::array<uint8_t, 4>> cells(20, {0, 0, 0, 0});
    for (int z = -4; z <= 0; z++) cells[size_t(z + 4)][3] = 1;
    {
        Fixture f;
        f.start[2] = 98304;
        f.target = f.start;
        f.target[0] += 2 * 65536;
        f.scene.column = [&](int32_t, int32_t) -> std::optional<Column> {
            f.columns++;
            return Column{-4, int32_t(cells.size()), std::as_bytes(std::span(cells))};
        };
        check(f.run(0) && !f.spatial.empty() && f.spatial[0].axis == 1 && f.spatial[0].position[2] == 197918 &&
              f.start[2] == 98304,
              "a clear raised box replaces privateZ with converted feet plus half-height plus132382 raw units");
    }
    {
        Fixture f;
        f.start[2] = 98304;
        f.target = f.start;
        f.target[2] += 3 * 65536;
        cells[5][3] = 1; // first cell of either raised inner-Z run blocks
        f.scene.column = [&](int32_t, int32_t) -> std::optional<Column> {
            f.columns++;
            return Column{-4, int32_t(cells.size()), std::as_bytes(std::span(cells))};
        };
        check(!f.run(0) && f.columns == 9 && f.spatial.size() == 1 && f.spatial[0].axis == 2,
              "raised blockage retains all three queries in each innerZ run before undoing horizontal movement");
    }
}
void recordConsumers() {
    Position start{};
    Dimensions size{2, 2, 2};
    uint16_t flags = 0;
    int exclusions = 0;
    bool ignored = false;
    Subject subject{start, size, flags, [&](uintptr_t identity) {
        exclusions++;
        check(identity == 101, "record exclusion receives the spatial identity, not coordinates or an actor key");
        return ignored;
    }};
    const Dimensions forward{1, 0, 0}, backward{-1, 0, 0};
    Record record{101, 4, {65536, 0, -65536}, 0, {2, 2, 2}, 0};
    check(recordBlocks(subject, start, 0, forward, record), "ungated selector4 blocks despite a zero state byte");
    for (const int32_t selector : {7, 6, 9}) {
        record.selector = selector;
        exclusions = 0;
        check(!recordBlocks(subject, start, 0, forward, record) && exclusions == 0,
              "excluded selectors return before pointer-set membership");
    }
    record.selector = 1;
    check(!recordBlocks(subject, start, 0, forward, record), "gated selector1 requires a nonzero state");
    record.state = 0x80;
    check(recordBlocks(subject, start, 0, forward, record), "state admission tests nonzero rather than a single bit");
    ignored = true;
    check(!recordBlocks(subject, start, 0, forward, record) && endpointOverlap(subject, start, record),
          "movement excludes the stored pointer while setup overlap has no pointer-membership gate");
    ignored = false;
    record.position[0] = 1;
    check(!recordBlocks(subject, start, 0, Dimensions{1.f / 65536.f, 0, 0}, record) &&
          recordBlocks(subject, start, 0, forward, record),
          "a small negative directional product truncates to zero before its signed test");
    record.position[0] = 2 * 65536;
    check(recordBlocks(subject, start, 0, forward, record), "record lower-edge equality is admitted");
    record.position[0] = -2 * 65536;
    check(!recordBlocks(subject, start, 0, backward, record), "subject lower-edge equality with record upper edge is rejected");
    record.position[0] = 3 * 65536;
    record.size = {1, 8, 2};
    record.orientation = 0;
    check(!recordBlocks(subject, start, 0, forward, record), "even orientation keeps the narrow first footprint");
    record.orientation = -1;
    check(recordBlocks(subject, start, 0, forward, record), "negative odd orientation swaps horizontal dimensions locally");
}

void bucketAndEndpointOrder() {
    Position position{8 * 65536, 8 * 65536, 5 * 65536};
    Dimensions size{2, 2, 2}, forward{1, 0, 0};
    uint16_t flags = 0;
    std::vector<uintptr_t> tested;
    Subject subject{position, size, flags, [&](uintptr_t identity) { tested.push_back(identity); return false; }};
    std::vector<std::array<int32_t, 2>> buckets;
    std::vector<Record> records{
        {1, 7, {}, 0, {1, 1, 1}, 1},
        {2, 4, {7 * 65536, 8 * 65536, 4 * 65536}, 0, {2, 2, 2}, 1},
        {3, 1, {}, 0, {1, 1, 1}, 0}
    };
    const RecordLookup rejected = [&](int32_t x, int32_t y, const RecordVisitor &visit) {
        buckets.push_back({x, y});
        for (const auto &record : records)
            if (!visit(record)) break;
    };
    check(scanRecords(subject, position, 0, forward, true, rejected) &&
          buckets == std::vector<std::array<int32_t, 2>>{{0, 0}} && tested == std::vector<uintptr_t>{2, 3},
          "retained terrain traverses all rejected records in the first bucket before stopping the neighborhood");
    buckets.clear(); tested.clear();
    check(!scanRecords(subject, position, 0, forward, false, rejected) && buckets.size() == 9 &&
          buckets[0] == std::array<int32_t, 2>{0, 0} && buckets[1] == std::array<int32_t, 2>{0, 1} &&
          buckets[3] == std::array<int32_t, 2>{1, 0} && buckets.back() == std::array<int32_t, 2>{2, 2},
          "ordinary unblocked scans retain X-outer Y-inner neighborhood and per-bucket record order");
    buckets.clear();
    const RecordLookup empty = [&](int32_t x, int32_t y, const RecordVisitor &) { buckets.push_back({x, y}); };
    check(scanRecords(subject, position, 2, forward, true, empty) && buckets.size() == 1,
          "an empty or missing first bucket does not search for a later nonempty bucket when terrain already blocks");
    buckets.clear();
    Position origin{};
    check(scanRecords(subject, origin, 2, forward, true, empty) && buckets.empty(),
          "an invalid first bucket checks retained terrain without querying another bucket");
    buckets.clear(); tested.clear();
    records.insert(records.begin(), {4, 4, {9 * 65536, 8 * 65536, 4 * 65536}, 0, {2, 2, 2}, 0});
    check(scanRecords(subject, position, 0, forward, false, rejected) && buckets.size() == 1 &&
          tested == std::vector<uintptr_t>{4}, "the first blocking record exits before another node or bucket");

    std::set<uintptr_t> excluded;
    const auto insert = [&](uintptr_t identity) { excluded.insert(identity); };
    const std::vector<Record> endpoints{
        {101, 4, {10 * 65536, 8 * 65536 + 32768, 5 * 65536}, 0, {1, 1, 2}, 0},
        {102, 4, {19 * 65536, 8 * 65536 + 32768, 5 * 65536}, 0, {1, 1, 2}, 0}
    };
    const RecordLookup endpointRecords = [&](int32_t x, int32_t y, const RecordVisitor &visit) {
        for (const auto &record : endpoints)
            if (publicationCell(record.position[0]) == x && publicationCell(record.position[1]) == y)
                if (!visit(record)) break;
    };
    Position first{8 * 65536 + 6554, 8 * 65536 + 13107, 5 * 65536 + 123};
    Position second{17 * 65536 + 16384, first[1], first[2]};
    endpointExclusions(subject, first, endpointRecords, insert);
    check(excluded == std::set<uintptr_t>{101}, "setup overlaps use snapped grid-plus-half-cell centers rather than raw endpoints");
    endpointExclusions(subject, second, endpointRecords, insert);
    endpointExclusions(subject, first, endpointRecords, insert);
    check(excluded == std::set<uintptr_t>{101, 102}, "both endpoint scans accumulate a unique identity union without consuming records");
}

void actualBuckets() {
    auto zone = std::make_shared<worldgen::Zone>(1, 2);
    const auto append = [&](int64_t x, int64_t y, int32_t selector = 4) {
        worldgen::StaticObject record;
        record.x = x; record.y = y; record.z = 10 * 65536;
        record.kind = selector; record.state = 1;
        zone->statics.push_back(record);
    };
    append(256 * 65536ll, 512 * 65536ll, 1);
    append(256 * 65536ll, 512 * 65536ll, 7);
    zone->statics.back().state = 0;
    zone->statics.back().scale[0] = 1000;
    append(264 * 65536ll, 520 * 65536ll);
    append(512 * 65536ll, 512 * 65536ll); // exact upper owning-chunk boundary
    append(256 * 65536ll - 1, 512 * 65536ll);
    append(256 * 65536ll, 768 * 65536ll);
    creatures::SpatialBuckets index;
    index.build(*zone);
    check(std::vector<uint32_t>(index.at(0, 0).begin(), index.at(0, 0).end()) == std::vector<uint32_t>{0, 1} &&
          std::vector<uint32_t>(index.at(1, 1).begin(), index.at(1, 1).end()) == std::vector<uint32_t>{2} &&
          index.at(1, 0).empty() && index.at(0, 1).empty(),
          "actual bucket indexing preserves source order, ignores kind/state/width and rejects out-of-owning-chunk centers");
    worldgen::Zone negative{0, 0};
    worldgen::StaticObject near;
    near.x = -1; near.y = -524287;
    negative.statics.push_back(near);
    near.x = -524288;
    negative.statics.push_back(near);
    index.build(negative);
    check(index.at(0, 0).size() == 1 && index.at(0, 0)[0] == 0 && publicationCell(-524287) == 0 &&
          publicationCell(-524288) == -1, "publication preserves its double/truncation chain at negative cell-zero boundaries");

    worldgen::WorldMap map;
    map.add(zone);
    StaticStates states;
    states.generated(*zone);
    std::vector<Record> received;
    states.routeRecords(map, 32, 64, [&](const Record &record) { received.push_back(record); return true; });
    check(received.size() == 2 && received[0].identity == reinterpret_cast<uintptr_t>(&zone->statics[0]) &&
          received[0].reference == std::array<int32_t, 3>{1, 2, 0} && received[1].reference[2] == 1 &&
          received[1].state == 0, "production lookup retains actual record identity and bucket payload independently of state admission");
    received.clear();
    auto state = states.record(*zone, 0);
    state[0x40] = 0;
    states.apply(state.data());
    states.routeRecords(map, 32, 64, [&](const Record &record) { received.push_back(record); return false; });
    check(received.size() == 1 && received[0].state == 0,
          "the same bucket adopts live state changes and honors immediate visitor stop without reclassifying membership");
    auto replacement = std::make_shared<worldgen::Zone>(*zone);
    replacement->statics[0].x += 8 * 65536;
    map.add(replacement);
    received.clear();
    states.routeRecords(map, 32, 64, [&](const Record &record) { received.push_back(record); return true; });
    check(received.size() == 1 && received[0].reference[2] == 1,
          "a replacement snapshot rebuilds the index rather than reusing old membership");
    map.remove(1, 2);
    received.clear();
    states.routeRecords(map, 32, 64, [&](const Record &record) { received.push_back(record); return true; });
    check(received.empty(), "an unloaded chunk has no ghost bucket references");
    states.forget(1, 2);
}

void engineCompanion() {
    for (const uint8_t state : {uint8_t(0), uint8_t(1)}) {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        for (int32_t x = 270; x <= 294; x++)
            for (int32_t y = 16; y <= 24; y++) {
                auto &column = zone->column(x, y);
                column.bottom = 98;
                column.blocks.assign(16, worldgen::Block{});
                column.blocks[0].typeByte = 1;
            }
        worldgen::StaticObject object;
        object.kind = 1; object.state = state;
        object.x = 281 * 65536ll; object.y = 20 * 65536ll + 32768; object.z = 98 * 65536ll;
        object.scale[0] = 1; object.scale[1] = 2; object.scale[2] = 4;
        zone->statics.push_back(object);
        worldgen::WorldMap map; map.add(zone);
        StaticStates states; states.generated(*zone);
        creatures::Npcs world;
        int bucketCalls = 0;
        world.routeRecords = [&](int32_t x, int32_t y, const RecordVisitor &visit) {
            bucketCalls++;
            states.routeRecords(map, x, y, visit);
        };
        creatures::Npc child;
        child.fighter.id = UINT64_MAX;
        child.fighter.controller = 3;
        child.fighter.characterClass = 0;
        child.fighter.type = 37;
        child.fighter.ownerId = child.follows = 1;
        child.fighter.appearanceFlags = child.looks.flags = 0x0829;
        child.fighter.health = creatures::maxHealth(child.fighter);
        child.fighter.hitStun = -3000;
        child.looks.box = child.body.size = glm::vec3(.8f);
        child.body.place({276.5, 20.5, 100});
        child.home = child.body.blocks(); child.zone = {1, 0};
        child.spawn = creatures::Npc::NO_SPAWN;
        child.behaviour = 7; child.thinkMs = 16;
        world.npcs().push_back(child);
        creatures::Combatant owner;
        owner.id = 1; owner.health = creatures::maxHealth(owner);
        creatures::Body body;
        body.place({286.5, 20.5, 100});
        const creatures::BlockType block = [&](int32_t x, int32_t y, int32_t z) { return map.read(x, y, z).type(); };
        world.step(16, map, {{owner, body, 0, 0, false}}, 1000, 0, block);
        check(world.all().size() == 1 && bucketCalls > 0, "actual created-companion behavior uses the resident spatial service");
        if (world.all().size() != 1)
            continue;
        const auto &after = world.all()[0];
        if (state == 0)
            check(after.path.state.radius == 0 && after.path.nodeCount() == 0 && after.body.intent.x == 30,
                  "an open spatial record permits real direct steering and clears search state");
        else
            check(after.path.state.radius == 2 && after.path.nodeCount() > 0,
                  "a closed record sends real companion behavior into the separate terrain/path continuation");
    }
}
} // namespace
int main() {
    proximityAndProgress(); raisedBoxes(); recordConsumers(); bucketAndEndpointOrder(); actualBuckets(); engineCompanion();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
