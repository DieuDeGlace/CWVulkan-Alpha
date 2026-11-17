#include "StaticStates.h"
#include "WorldItems.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <utility>

float openFraction(int32_t kind, const StaticState &s) {
    const float t = (float)s.timerMs;
    if (s.state == 0)
        return std::min(t * 0.001f, 1.f);
    const float rate = kind == 7 || kind == 8 ? 0.01f : 0.001f; // traps close ten times faster
    return std::max(1 - t * rate, 0.f);
}

void StaticStates::generated(const worldgen::Zone &zone) {
    const Key key{zone.zx, zone.zy};
    if (zones.count(key))
        return;
    std::vector<StaticState> &list = zones[key];
    for (const worldgen::StaticObject &o : zone.statics) {
        StaticState s;
        s.state = o.state;
        s.timerMs = o.timer;
        s.stateWord = o.stateWord;
        for (const worldgen::ItemRecord &item : o.contents)
            s.contents.push_back(creatures::toRecord(item));
        list.push_back(std::move(s));
    }
}

void StaticStates::restore(int32_t zx, int32_t zy, const std::vector<uint8_t> &states) {
    const auto it = zones.find({zx, zy});
    if (it == zones.end() || states.size() != it->second.size())
        return; // (a save of a different generator output is left alone)
    for (size_t i = 0; i < states.size(); i++)
        it->second[i].state = states[i];
    // §4.5: contents are not saved; a chest saved as state 2 keeps its generated loot.
}

const std::vector<StaticState> *StaticStates::zone(int32_t zx, int32_t zy) const {
    const auto it = zones.find({zx, zy});
    return it == zones.end() ? nullptr : &it->second;
}

void StaticStates::visibilityRecords(const worldgen::WorldMap &map, int32_t zx, int32_t zy,
                                    const creatures::visibility::RecordVisitor &visit) const {
    const auto *source = map.zone(zx, zy);
    if (!source)
        return;
    const auto *states = zone(zx, zy);
    size_t index = 0;
    for (const auto &object : source->statics) {
        const uint8_t state = states && index < states->size() ? (*states)[index].state : object.state;
        const creatures::visibility::Record record{
            object.kind, {object.x, object.y, object.z}, {object.scale[0], object.scale[1], object.scale[2]},
            object.rotation, state};
        visit(record);
        index++;
    }
}

std::vector<uint8_t> StaticStates::stateBytes(int32_t zx, int32_t zy) const {
    std::vector<uint8_t> out;
    if (const auto *list = zone(zx, zy))
        for (const StaticState &s : *list)
            out.push_back(s.state);
    return out;
}

void StaticStates::routeRecords(const worldgen::WorldMap &map, int32_t cellX, int32_t cellY,
                               const creatures::route::RecordVisitor &visit) const {
    if (cellX < 0 || cellY < 0 || cellX >= 0x200000 || cellY >= 0x200000)
        return;
    const Key key{cellX / 32, cellY / 32};
    const auto held = map.zones().find(key);
    if (held == map.zones().end())
        return;
    const auto &source = held->second;
    auto &index = spatial[key];
    // Compare ownership without locking a weak pointer on every bucket query.
    if (index.address != source.get() || index.source.owner_before(source) || source.owner_before(index.source)) {
        index.buckets.build(*source);
        index.source = source;
        index.address = source.get();
    }
    const auto *states = zone(key.first, key.second);
    for (const uint32_t ordinal : index.buckets.at(cellX % 32, cellY % 32)) {
        const auto &object = source->statics[ordinal];
        const uint8_t state = states && ordinal < states->size() ? (*states)[ordinal].state : object.state;
        const creatures::route::Record record{
            reinterpret_cast<uintptr_t>(&object), object.kind, {object.x, object.y, object.z}, object.rotation,
            {object.scale[0], object.scale[1], object.scale[2]}, state, {key.first, key.second, int32_t(ordinal)}};
        if (!visit(record))
            break;
    }
}

std::array<uint8_t, 88> StaticStates::record(const worldgen::Zone &zone, int32_t index) const {
    std::array<uint8_t, 88> r{};
    const auto *list = this->zone(zone.zx, zone.zy);
    if (!list || index < 0 || index >= (int32_t)list->size() || index >= (int32_t)zone.statics.size())
        return r;
    const worldgen::StaticObject &o = zone.statics[index];
    const StaticState &s = (*list)[index];
    const auto put = [&](size_t at, const void *v, size_t n) { std::memcpy(r.data() + at, v, n); };
    put(0x00, &zone.zx, 4);
    put(0x04, &zone.zy, 4);
    put(0x08, &index, 4);
    put(0x10, &o.kind, 4);           // word 0
    const int64_t position[3] = {o.x, o.y, o.z};
    put(0x18, position, 24);         // words 2-7
    put(0x30, &o.rotation, 4);       // word 8
    put(0x34, o.scale, 12);          // words 9-11
    r[0x40] = s.state;               // byte 0x30
    put(0x44, &s.timerMs, 4);        // word 13
    put(0x48, &s.stateWord, 4);      // word 14, retained without assigning semantics
    put(0x50, &s.occupant, 8);       // words 16-17
    return r;
}

std::optional<StaticStates::Key> StaticStates::apply(const uint8_t *r, const worldgen::WorldMap *resident) {
    int32_t zx, zy, index;
    std::memcpy(&zx, r, 4);
    std::memcpy(&zy, r + 4, 4);
    std::memcpy(&index, r + 8, 4);
    const Key key{zx, zy};
    if (resident && !resident->containsNetworkZone(key))
        return std::nullopt;
    const auto it = zones.find(key);
    if (it == zones.end() || index < 0 || size_t(index) >= it->second.size())
        return std::nullopt;
    StaticState &s = it->second[index];
    s.state = r[0x40];
    std::memcpy(&s.timerMs, r + 0x44, 4);
    std::memcpy(&s.stateWord, r + 0x48, 4);
    std::memcpy(&s.occupant, r + 0x50, 8);
    return it->first;
}

bool StaticStates::fatal(const worldgen::WorldMap &map, const creatures::BackingReference &backing,
                          creatures::NpcFrame &frame) {
    if (backing.x < 0 || backing.x >= 65536 || backing.y < 0 || backing.y >= 65536)
        return false;
    const auto *held = map.zone(backing.x, backing.y);
    const auto found = zones.find({backing.x, backing.y});
    if (!held || found == zones.end())
        return false;
    auto &states = found->second;
    for (size_t index = 0; index < held->statics.size() && index < states.size(); index++) {
        const auto &object = held->statics[index];
        auto &state = states[index];
        if (object.kind == 5 && object.descriptorIndex == backing.index && state.state != 0) {
            set(object, state, 0, frame.sounds);
            changed.insert(found->first);
            frame.staticStates.push_back(record(*held, (int32_t)index));
            return true;
        }
    }
    return false;
}

void StaticStates::appendUpdates(const worldgen::WorldMap &map, std::span<const std::array<uint8_t, 88>> immediate,
                                 const std::set<Key> &changedZones, const std::vector<Key> &requestedZones,
                                 std::vector<std::vector<uint8_t>> &output) const {
    std::map<std::array<int32_t, 3>, const std::array<uint8_t, 88> *> latest;
    for (const auto &snapshot : immediate) {
        output.emplace_back(snapshot.begin(), snapshot.end());
        std::array<int32_t, 3> key;
        std::memcpy(key.data(), snapshot.data(), 12);
        latest[key] = &snapshot;
    }
    const auto appendZone = [&](const Key &key, bool requested) {
        const auto [x, y] = key;
        const auto *held = map.zone(x, y);
        const auto *current = zone(x, y);
        if (!held || !current)
            return; // engine omission policy for incomplete retained storage
        const size_t count = std::min({held->statics.size(), current->size(), size_t(INT32_MAX) + 1});
        for (size_t index = 0; index < count; index++) {
            const auto snapshot = record(*held, (int32_t)index);
            if (!requested) {
                const auto previous = latest.find({x, y, (int32_t)index});
                if (previous != latest.end() && *previous->second == snapshot)
                    continue;
            }
            output.emplace_back(snapshot.begin(), snapshot.end());
        }
    };
    for (const auto &key : changedZones)
        appendZone(key, false);
    for (const auto &key : requestedZones)
        if (map.containsNetworkZone(key))
            appendZone(key, true);
}

void StaticStates::traps(int32_t dtMs, const std::vector<Creature> &unordered,
                         const std::map<Key, const worldgen::Zone *> &held, const std::function<int32_t(glm::dvec3)> &level,
                         const TrapStrike &strike, std::vector<creatures::SoundEvent> &sounds) {
    // Hazards-blast-ticks contract §3 (site 1). The world creature collection is visited in
    // ascending signed id.
    std::vector<Creature> creatures = unordered;
    std::stable_sort(creatures.begin(), creatures.end(), [](const Creature &a, const Creature &b) {
        return std::bit_cast<int64_t>(a.id) < std::bit_cast<int64_t>(b.id);
    });
    const auto raw = [](double blocks) { return (int64_t)(blocks * 65536.0); };
    const auto wrapSub = [](int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); };
    // The raw squared length of object minus creature below n blocks squared (signed 64-bit).
    const auto within = [&](const worldgen::StaticObject &o, glm::dvec3 c, int64_t n) {
        const int64_t d[3] = {wrapSub(o.x, raw(c.x)), wrapSub(o.y, raw(c.y)), wrapSub(o.z, raw(c.z))};
        uint64_t sum = 0;
        for (int k = 0; k < 3; k++)
            sum += uint64_t(std::bit_cast<int64_t>(uint64_t(d[k]) * uint64_t(d[k])) / 65536);
        return std::bit_cast<int64_t>(sum) < n * 65536;
    };
    // §3.1/§3.3 step 7: types 6 and 7 fire when the advanced world clock and the clock one
    // frame later lie in different 200 ms quotients (32-bit, truncating).
    hazardClockMs = std::bit_cast<int32_t>(uint32_t(hazardClockMs) + uint32_t(dtMs));
    const int32_t later = std::bit_cast<int32_t>(uint32_t(hazardClockMs) + uint32_t(dtMs));
    const bool tick = later / 200 != hazardClockMs / 200;
    for (auto &[key, list] : zones) {
        const auto z = held.find(key);
        if (z == held.end())
            continue;
        for (size_t i = 0; i < list.size() && i < z->second->statics.size(); i++) {
            const worldgen::StaticObject &o = z->second->statics[i];
            if (o.kind < 6 || o.kind > 8)
                continue;
            StaticState &s = list[i];
            const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
            // §1.2: the arming cycle.
            if (s.timerMs > 2000 && s.state != 0) {
                set(o, s, 0, sounds);
                changed.insert(key);
            } else if (s.timerMs > 2000) {
                for (const Creature &c : creatures)
                    if (c.controller == 0 && within(o, c.at, 900)) {
                        set(o, s, 1, sounds);
                        changed.insert(key);
                        break;
                    }
            }
            // §1.3: the hits.
            if (s.state == 0 || !(o.kind == 8 ? s.timerMs == 0 : tick))
                continue;
            // Step 8.5: D = binary32(2^((L - 1) x 0.25)) x 10, then x 5 for type 8.
            const float exponent = float(std::bit_cast<int32_t>(uint32_t(level(at)) - 1u)) * 0.25f;
            float amount = float(std::pow(2.0, double(exponent))) * 10.f;
            if (o.kind == 8)
                amount *= 5.f;
            for (const Creature &c : creatures) {
                if (!within(o, c.at, 25) || c.controller == 1) // range first, then role 1
                    continue;
                // Step 8.4: P1 = (creature - object) in presentation units, z 0, normalised
                // above 0, x 20, z 5; type 8 doubles the whole vector.
                glm::vec3 u(float(wrapSub(raw(c.at.x), o.x)) * (1.f / 65536.f),
                            float(wrapSub(raw(c.at.y), o.y)) * (1.f / 65536.f), 0.f);
                const float squared = (u.x * u.x + u.y * u.y) + u.z * u.z;
                if (squared > 0.f) {
                    const float inverse = 1.f / float(std::sqrt(double(squared)));
                    u = {u.x * inverse, u.y * inverse, u.z * inverse};
                }
                glm::vec3 k(u.x * 20.f, u.y * 20.f, 5.f);
                if (o.kind == 8)
                    k *= 2.f;
                strike(c, amount, k, o.kind == 6);
            }
        }
    }
}

void StaticStates::set(const worldgen::StaticObject &o, StaticState &s, uint8_t to,
                       std::vector<creatures::SoundEvent> &sounds) {
    if (s.state == to)
        return;
    s.timerMs = std::max(std::bit_cast<int32_t>(1000u - uint32_t(s.timerMs)), 0);
    s.state = to;
    glm::dvec3 at;
    int32_t sound = 54;
    if (o.kind == 5) {
        sound = 51;
        at = {double(float(o.x) * (1.f / 65536.f)), double(float(o.y) * (1.f / 65536.f)),
              double(float(o.z) * (1.f / 65536.f))};
    } else {
        at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
        if (o.kind == 6)
            sound = to != 0 ? 53 : 0;
        else if (o.kind == 7)
            sound = to != 0 ? 52 : 0;
    }
    if (sound)
        sounds.push_back({at, sound, 1, 1});
}

static const worldgen::Zone *nearestStaticZone(const worldgen::WorldMap &world, const int64_t position[3]) {
    constexpr int64_t fixed = 65536;
    constexpr int64_t bucketSize = 8 * fixed;
    constexpr int64_t bucketsPerZone = worldgen::ZONE / 8;
    constexpr int64_t maxDelta = 4 * fixed;
    constexpr int64_t maxDistance = 16 * fixed;
    const auto floor = [](int64_t value, int64_t divisor) {
        return value / divisor - (value % divisor < 0);
    };
    const int64_t bx = floor(position[0], bucketSize), by = floor(position[1], bucketSize);
    const int64_t zx0 = floor(bx - 1, bucketsPerZone), zy0 = floor(by - 1, bucketsPerZone);
    const int64_t zx1 = floor(bx + 1, bucketsPerZone), zy1 = floor(by + 1, bucketsPerZone);
    const worldgen::Zone *nearest = world.zone(0, 0);
    int64_t bestDistance = maxDistance + 1;
    std::pair<int64_t, int64_t> bestBucket{};
    // Scan each intersecting zone once. Bucket order breaks equal-distance
    // ties, then the order of statics within the bucket.
    for (int64_t zx = zx0; zx <= zx1; ++zx) {
        for (int64_t zy = zy0; zy <= zy1; ++zy) {
            if (!std::in_range<int32_t>(zx) || !std::in_range<int32_t>(zy))
                continue;
            const auto *zone = world.zone((int32_t)zx, (int32_t)zy);
            if (!zone)
                continue;
            for (const auto &object : zone->statics) {
                const std::pair bucket{floor(object.x, bucketSize), floor(object.y, bucketSize)};
                if (bucket.first < bx - 1 || bucket.first > bx + 1 ||
                    bucket.second < by - 1 || bucket.second > by + 1)
                    continue;
                const auto delta = [](int64_t a, int64_t b) {
                    return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b));
                };
                const int64_t dx = delta(object.x, position[0]), dy = delta(object.y, position[1]),
                              dz = delta(object.z, position[2]);
                if (dx < -maxDelta || dx > maxDelta || dy < -maxDelta || dy > maxDelta ||
                    dz < -maxDelta || dz > maxDelta)
                    continue;
                const int64_t distance = dx * dx / fixed + dy * dy / fixed + dz * dz / fixed;
                if (distance <= maxDistance &&
                    (distance < bestDistance || (distance == bestDistance && bucket < bestBucket))) {
                    nearest = zone;
                    bestDistance = distance;
                    bestBucket = bucket;
                }
            }
        }
    }
    return nearest;
}

void StaticStates::use(const worldgen::Zone &zone, int32_t index, creatures::Combatant &c, creatures::Body &body,
                       float &yaw, WorldItems &items, int32_t day, const creatures::BlockType &block,
                       const std::function<int()> &draw, std::vector<creatures::SoundEvent> &sounds,
                       std::vector<std::array<uint8_t, 88>> *snapshots, bool authoritative,
                       const worldgen::WorldMap *localWorld) {
    if (c.action == 79)
        c.action = 0, c.stealth = 0;
    const auto it = zones.find({zone.zx, zone.zy});
    if (it == zones.end() || index < 0 || index >= (int32_t)it->second.size() || index >= (int32_t)zone.statics.size())
        return;
    const worldgen::StaticObject &o = zone.statics[index];
    if (o.kind == 45) {
        if (localWorld && o.descriptorIndex >= 0) {
            const auto *nearest = nearestStaticZone(*localWorld, body.position);
            if (nearest && size_t(o.descriptorIndex) < nearest->statics.size()) {
                const auto &destination = nearest->statics[o.descriptorIndex];
                sounds.push_back({body.blocks(), 47, 1, 1});
                body.position[0] = destination.x;
                body.position[1] = destination.y;
                body.position[2] = std::bit_cast<int64_t>(uint64_t(destination.z) + 2 * 65536);
            }
        }
        return;
    }
    StaticState &s = it->second[index];
    const Key key{zone.zx, zone.zy};
    const auto publish = [&] {
        changed.insert(key);
        if (snapshots)
            snapshots->push_back(record(zone, index));
    };
    // Step 4: seats and beds.
    const bool seat = o.kind == 16 || o.kind == 18, bed = o.kind == 19 || o.kind == 68 || o.kind == 69;
    if (seat || bed) {
        if (s.occupant != 0 || (c.action != 0 && c.action != 83 && c.action != 84))
            return;
        if (authoritative) {
            s.occupant = c.id;
            const int64_t lift = (int64_t)((body.size.z * 0.5f + o.scale[2]) * 65536.f);
            body.position[0] = o.x;
            body.position[1] = o.y;
            body.position[2] = std::bit_cast<int64_t>(uint64_t(o.z) + uint64_t(lift));
            body.velocity = glm::vec3(0);
            yaw = (float)(90 * (seat ? o.rotation + 2 : o.rotation));
            c.action = seat ? 83 : 84;
            publish();
        }
        body.intent = glm::vec3(0);
        return;
    }
    if (!authoritative)
        return;
    if (o.kind == 10 && s.state == 2) {
        // Chest caller draws Y, X, then an angle the shared drop helper will
        // replace. The helper draws the timer, then the angle if the zone exists.
        for (const creatures::ItemRecord &item : s.contents) {
            const float y = 2.f - (float)draw() * 4.f / 32767.f;
            const float x = 2.f - (float)draw() * 4.f / 32767.f;
            (void)draw();
            const int64_t at[3] = {std::bit_cast<int64_t>(uint64_t(o.x) + uint64_t((int64_t)(x * 65536.f))),
                                   std::bit_cast<int64_t>(uint64_t(o.y) + uint64_t((int64_t)(y * 65536.f))), o.z};
            items.drop(item, at, 1.f, day, block, draw);
        }
        s.contents.clear();
    }
    switch (o.kind) {
    case 1: case 2: case 3: case 9: case 10:
        set(o, s, s.state == 0 ? 1 : 0, sounds);
        publish();
        break;
    default:
        return;
    }
    // A lever then sets its partner, the link index +0x178 in the same zone, to closed while
    // the lever is closed, else open. Both snapshots carry the lever's own triple and state.
    if (o.kind == 9 && o.descriptorIndex >= 0 && o.descriptorIndex < (int32_t)zone.statics.size() &&
        o.descriptorIndex < (int32_t)it->second.size()) {
        set(zone.statics[(size_t)o.descriptorIndex], it->second[(size_t)o.descriptorIndex], s.state != 0 ? 1 : 0,
            sounds);
        if (snapshots)
            snapshots->push_back(record(zone, index));
    }
}

void StaticStates::step(int32_t dt, const Occupant &where, const std::map<Key, const worldgen::Zone *> &held,
                        bool release, const Place &place) {
    for (auto &[key, list] : zones) {
        const auto z = held.find(key);
        for (size_t i = 0; i < list.size(); i++) {
            StaticState &s = list[i];
            s.timerMs += dt; // §6.3: every static of each processed zone, no flag test
            if (!s.occupant || z == held.end() || i >= z->second->statics.size())
                continue;
            const worldgen::StaticObject &o = z->second->statics[i];
            glm::dvec3 at;
            glm::vec3 intent;
            int32_t action = 0, stun = 0;
            const glm::dvec3 base = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
            const auto free = [&] {
                if (release) {
                    s.occupant = 0;
                    changed.insert(key);
                }
            };
            if (!where(s.occupant, at, intent, action, stun) || stun > 0) {
                free();
                continue;
            }
            // §6.4: the occupant is first placed back on the seat (yaw (rotation + 2) x 90,
            // action 83) or bed (yaw rotation x 90, action 84), so the action test cannot fail
            // for these kinds and a non-zero +0x40 intent is the effective release.
            const bool seat = o.kind == 16 || o.kind == 18, bed = o.kind == 19 || o.kind == 68 || o.kind == 69;
            if ((seat || bed) && place) {
                const int32_t sitting = seat ? 83 : 84;
                if (const auto placed = place(s.occupant, base + glm::dvec3(0, 0, o.scale[2]),
                                              90.f * (float)(seat ? o.rotation + 2 : o.rotation), sitting))
                    at = *placed, action = sitting;
            }
            if (glm::dot(at - base, at - base) > 16 || glm::dot(intent, intent) != 0 || (action != 83 && action != 84))
                free();
            // (The native second placement, without the action change, repeats the first.)
        }
    }
}

creatures::Npcs::Seats StaticStates::seats(const worldgen::WorldMap &map, glm::ivec3 block, double timeOfDayMs) const {
    creatures::Npcs::Seats out;
    // The house the block is in: in the zone of block / 256 (truncating), the first house in list
    // order with a storey cell covering it (both ends included). The engine's houses are 3 x 3
    // cells, so the native swap of the x/y cell counts for an odd orientation changes nothing;
    // their storey cells were listed through the oriented cell view, a outer, b, then level.
    const int32_t hzx = block.x / worldgen::ZONE, hzy = block.y / worldgen::ZONE;
    if (const worldgen::Zone *zone = map.zone(hzx, hzy))
        for (const worldgen::House &h : zone->houses) {
            const bool inside = std::any_of(h.storeys.begin(), h.storeys.end(), [&](const std::array<int32_t, 3> &c) {
                const int32_t x = block.x - h.ox - 13 * c[0], y = block.y - h.oy - 13 * c[1], z = block.z - h.oz - 7 * c[2];
                return x >= 0 && x <= 13 && y >= 0 && y <= 13 && z >= 0 && z <= 6;
            });
            if (!inside)
                continue;
            const auto seat = [&](int32_t k) {
                const worldgen::StaticObject &o = zone->statics[(size_t)k];
                return creatures::Seat{hzx, hzy, k, glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0};
            };
            out.house = true;
            for (int32_t k : h.seats)
                out.keep.push_back(seat(k));
            for (int32_t k : h.beds)
                out.keep.push_back(seat(k));
            const bool day = timeOfDayMs >= 21600000 && timeOfDayMs <= 82799999;
            for (int32_t k : day ? h.seats : h.beds)
                out.pick.push_back(seat(k));
            return out;
        }
    // Outdoors: the 3 x 3 cells of 8 x 8 blocks round the block (cell = block / 8, zone = cell /
    // 32, both truncating; cells outside 0 ... 2^21 - 1 skipped).
    const int32_t cx = block.x / 8, cy = block.y / 8;
    constexpr int32_t CELLS = 1 << 21;
    struct Found {
        int32_t cellX, cellY;
        creatures::Seat seat;
    };
    std::vector<Found> found;
    std::set<std::pair<int32_t, int32_t>> zonesSeen;
    for (int32_t i = -1; i <= 1; i++)
        for (int32_t j = -1; j <= 1; j++) {
            if (cx + i < 0 || cy + j < 0 || cx + i >= CELLS || cy + j >= CELLS)
                continue;
            const int32_t zx = (cx + i) / 32, zy = (cy + j) / 32;
            if (!zonesSeen.insert({zx, zy}).second)
                continue;
            const worldgen::Zone *zone = map.zone(zx, zy);
            if (!zone)
                continue;
            for (int32_t k = 0; k < (int32_t)zone->statics.size(); k++) {
                const worldgen::StaticObject &o = zone->statics[(size_t)k];
                if (o.kind != 16 && o.kind != 18 && o.kind != 68)
                    continue;
                const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                const int32_t ox = worldgen::floorDiv((int32_t)std::floor(at.x), 8);
                const int32_t oy = worldgen::floorDiv((int32_t)std::floor(at.y), 8);
                if (std::abs(ox - cx) <= 1 && std::abs(oy - cy) <= 1)
                    found.push_back({ox, oy, {zx, zy, k, at}});
            }
        }
    std::stable_sort(found.begin(), found.end(), [](const Found &a, const Found &b) {
        return a.cellX != b.cellX ? a.cellX < b.cellX : a.cellY < b.cellY;
    });
    for (const Found &f : found)
        out.keep.push_back(f.seat);
    out.pick = out.keep;
    return out;
}


bool StaticStates::free(const creatures::Seat &seat) const {
    const auto *list = zone(seat.zx, seat.zy);
    return list && seat.index >= 0 && (size_t)seat.index < list->size() && (*list)[(size_t)seat.index].occupant == 0;
}

bool StaticStates::blocks(const worldgen::WorldMap &map, glm::dvec3 lo, glm::dvec3 hi, int axis, double move,
                          const creatures::IgnoredStatics *ignored) const {
    const glm::dvec3 centre = 0.5 * (lo + hi);
    const int32_t zx0 = worldgen::floorDiv((int32_t)std::floor(lo.x) - 16, worldgen::ZONE);
    const int32_t zx1 = worldgen::floorDiv((int32_t)std::floor(hi.x) + 16, worldgen::ZONE);
    const int32_t zy0 = worldgen::floorDiv((int32_t)std::floor(lo.y) - 16, worldgen::ZONE);
    const int32_t zy1 = worldgen::floorDiv((int32_t)std::floor(hi.y) + 16, worldgen::ZONE);
    for (int32_t zx = zx0; zx <= zx1; zx++)
        for (int32_t zy = zy0; zy <= zy1; zy++) {
            const worldgen::Zone *zone = map.zone(zx, zy);
            if (!zone)
                continue;
            const std::vector<StaticState> *live = this->zone(zx, zy);
            for (size_t k = 0; k < zone->statics.size(); k++) {
                const worldgen::StaticObject &o = zone->statics[k];
                if (o.kind == 6 || o.kind == 7 || o.kind == 9)
                    continue;
                if (ignored && ignored->count({zx, zy, (int32_t)k})) // (path search §4.1 step 3)
                    continue;
                const uint8_t state = live && k < live->size() ? (*live)[k].state : o.state;
                if ((o.kind == 1 || o.kind == 2 || o.kind == 3 || o.kind == 5 || o.kind == 8) && state == 0)
                    continue;
                const bool turned = ((o.rotation % 2) + 2) % 2 == 1;
                const double sx = turned ? o.scale[1] : o.scale[0], sy = turned ? o.scale[0] : o.scale[1];
                const glm::dvec3 c = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                // Open points 2 §2.2 step 4: q = trunc(float(fixed-point difference) x d); q >= 0
                // (moving away, level, or below one fixed-point unit) skips the static.
                if (axis >= 0) {
                    const float difference = (float)((centre[axis] - c[axis]) * 65536.0);
                    if (std::trunc(difference * (float)move) >= 0)
                        continue;
                }
                // §2.3: X - hx <= pos + w/2 and pos - w/2 < X + hx on each axis (z from the base).
                const glm::dvec3 olo(c.x - sx / 2, c.y - sy / 2, c.z), ohi(c.x + sx / 2, c.y + sy / 2, c.z + o.scale[2]);
                if (olo.x <= hi.x && lo.x < ohi.x && olo.y <= hi.y && lo.y < ohi.y && olo.z <= hi.z && lo.z < ohi.z)
                    return true;
            }
        }
    return false;
}


void StaticStates::activateRouteRecord(const worldgen::WorldMap &map, const creatures::route::Record &record) {
    const auto &reference = record.reference;
    const auto *source = map.zone(reference[0], reference[1]);
    const auto found = zones.find({reference[0], reference[1]});
    if (!source || reference[2] < 0 || size_t(reference[2]) >= source->statics.size() || found == zones.end() ||
        size_t(reference[2]) >= found->second.size() ||
        uintptr_t(&source->statics[size_t(reference[2])]) != record.identity)
        return; // engine ownership guard for a replaced/unloaded snapshot
    StaticState &state = found->second[size_t(reference[2])];
    state.state = 0;
    state.timerMs = 0;
    changed.insert({reference[0], reference[1]});
}
