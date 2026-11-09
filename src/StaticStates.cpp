#include "StaticStates.h"
#include "WorldItems.h"
#include <algorithm>
#include <cstring>

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

std::vector<uint8_t> StaticStates::stateBytes(int32_t zx, int32_t zy) const {
    std::vector<uint8_t> out;
    if (const auto *list = zone(zx, zy))
        for (const StaticState &s : *list)
            out.push_back(s.state);
    return out;
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
    put(0x44, &s.timerMs, 4);        // word 13 (word 14 unknown, 0)
    put(0x50, &s.occupant, 8);       // words 16-17
    return r;
}

void StaticStates::apply(const uint8_t *r) {
    int32_t zx, zy, index;
    std::memcpy(&zx, r, 4);
    std::memcpy(&zy, r + 4, 4);
    std::memcpy(&index, r + 8, 4);
    const auto it = zones.find({zx, zy});
    if (it == zones.end() || index < 0 || index >= (int32_t)it->second.size())
        return;
    StaticState &s = it->second[index];
    s.state = r[0x40];
    std::memcpy(&s.timerMs, r + 0x44, 4);
    std::memcpy(&s.occupant, r + 0x50, 8);
}

void StaticStates::traps(int32_t dtMs, double clockMs, const std::vector<Creature> &creatures,
                         const std::map<Key, const worldgen::Zone *> &held, const std::function<int32_t(glm::dvec3)> &level,
                         std::vector<creatures::HitRecord> &hits, std::vector<creatures::SoundEvent> &sounds) {
    // §1.4: squared distance in blocks below `n`, in whole 1/65536 steps.
    const auto within = [](glm::dvec3 a, glm::dvec3 b, int64_t n) {
        int64_t sum = 0;
        for (int k = 0; k < 3; k++) {
            const int64_t d = (int64_t)(a[k] * 65536.0) - (int64_t)(b[k] * 65536.0);
            sum += d * d / 65536;
        }
        return sum < n * 65536;
    };
    const bool tick = std::floor((clockMs + dtMs) / 100) != std::floor(clockMs / 100);
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
                    if (c.controller == 0 && within(c.at, at, 900)) {
                        set(o, s, 1, sounds);
                        changed.insert(key);
                        break;
                    }
            }
            // §1.3: the hits.
            if (s.state == 0 || !(o.kind == 8 ? s.timerMs == 0 : tick))
                continue;
            const float amount = (o.kind == 8 ? 50.f : 10.f) * std::pow(2.f, (level(at) - 1) / 4.f);
            for (const Creature &c : creatures) {
                if (c.controller == 1 || !within(c.at, at, 25))
                    continue;
                glm::vec2 u = glm::vec2(c.at - at);
                if (glm::dot(u, u) > 0)
                    u = glm::normalize(u);
                glm::vec3 k(20 * u.x, 20 * u.y, 5);
                if (o.kind == 8)
                    k *= 2;
                creatures::HitRecord h;
                h.target = c.id;
                h.amount = amount;
                h.knockBack = k;
                hits.push_back(h);
            }
        }
    }
}

void StaticStates::set(const worldgen::StaticObject &o, StaticState &s, uint8_t to,
                       std::vector<creatures::SoundEvent> &sounds) {
    if (s.state == to)
        return;
    s.timerMs = std::max(1000 - s.timerMs, 0);
    s.state = to;
    const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
    int32_t sound = 54;
    if (o.kind == 5)
        sound = 51;
    else if (o.kind == 6)
        sound = to != 0 ? 53 : 0;
    else if (o.kind == 7)
        sound = to != 0 ? 52 : 0;
    if (sound)
        sounds.push_back({at, sound, 1, 1});
}

void StaticStates::use(const worldgen::Zone &zone, int32_t index, creatures::Combatant &c, creatures::Body &body,
                       float &yaw, WorldItems &items, int32_t day, const creatures::BlockType &block,
                       const std::function<int()> &draw, std::vector<creatures::SoundEvent> &sounds) {
    const auto it = zones.find({zone.zx, zone.zy});
    if (it == zones.end() || index < 0 || index >= (int32_t)it->second.size() || index >= (int32_t)zone.statics.size())
        return;
    // Step 1: using anything ends sneaking.
    if (c.action == 79)
        c.action = 0, c.stealth = 0;
    const worldgen::StaticObject &o = zone.statics[index];
    StaticState &s = it->second[index];
    const Key key{zone.zx, zone.zy};
    // Step 4: seats and beds.
    const bool seat = o.kind == 16 || o.kind == 18, bed = o.kind == 19 || o.kind == 68 || o.kind == 69;
    if (seat || bed) {
        if (s.occupant == 0 && (c.action == 0 || c.action == 83 || c.action == 84)) {
            s.occupant = c.id;
            body.place(glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0 +
                       glm::dvec3(0, 0, body.size.z / 2 + o.scale[2]));
            body.velocity = glm::vec3(0);
            yaw = 90.f * (float)(seat ? o.rotation + 2 : o.rotation);
            c.action = seat ? 83 : 84;
            c.actionMs = 0;
            changed.insert(key);
        }
        body.intent = glm::vec3(0);
        return;
    }
    // Step 5: chests drop their contents, then doors, windows, levers and chests toggle.
    if (o.kind == 10 && s.state == 2) {
        const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
        for (const creatures::ItemRecord &item : s.contents) {
            const float rx = (float)draw() / 32767.f, ry = (float)draw() / 32767.f;
            items.drop(item, at + glm::dvec3(2 - 4 * rx, 2 - 4 * ry, 0), 1, day, block, draw);
        }
        s.contents.clear();
    }
    switch (o.kind) {
    case 1: case 2: case 3: case 9: case 10:
        set(o, s, s.state == 0 ? 1 : 0, sounds);
        changed.insert(key);
        break;
    default:
        return;
    }
    // (A lever would also set its linked static, +0x178, but no generator sets a link: open
    // point I-3.)
}

void StaticStates::step(int32_t dt, const std::function<bool(uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &)> &where,
                        const std::map<Key, const worldgen::Zone *> &held) {
    for (auto &[key, list] : zones) {
        const auto z = held.find(key);
        for (size_t i = 0; i < list.size(); i++) {
            StaticState &s = list[i];
            s.timerMs += dt;
            if (!s.occupant || z == held.end() || i >= z->second->statics.size())
                continue;
            const worldgen::StaticObject &o = z->second->statics[i];
            glm::dvec3 at;
            glm::vec3 intent;
            int32_t action;
            const glm::dvec3 seat = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
            const bool stays = where(s.occupant, at, intent, action) && glm::dot(at - seat, at - seat) <= 16 &&
                               glm::dot(intent, intent) == 0 && (action == 83 || action == 84);
            if (!stays) {
                s.occupant = 0;
                changed.insert(key);
            }
        }
    }
}

creatures::Npcs::Seats StaticStates::seats(const worldgen::WorldMap &map, glm::ivec3 block, double timeOfDayMs) const {
    creatures::Npcs::Seats out;
    // The house the block is in: the first with a storey cell covering it (both ends included).
    const int32_t hzx = worldgen::floorDiv(block.x, worldgen::ZONE), hzy = worldgen::floorDiv(block.y, worldgen::ZONE);
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
    const int32_t cx = worldgen::floorDiv(block.x, 8), cy = worldgen::floorDiv(block.y, 8);
    struct Found {
        int32_t cellX, cellY;
        creatures::Seat seat;
    };
    std::vector<Found> found;
    std::set<std::pair<int32_t, int32_t>> zonesSeen;
    for (int32_t i = -1; i <= 1; i++)
        for (int32_t j = -1; j <= 1; j++) {
            const int32_t zx = worldgen::floorDiv((cx + i) * 8, worldgen::ZONE), zy = worldgen::floorDiv((cy + j) * 8, worldgen::ZONE);
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

void StaticStates::searchStatics(const worldgen::WorldMap &map, int32_t ix0, int32_t ix1, int32_t iy0, int32_t iy1,
                                 const std::function<void(const creatures::SearchStatic &)> &visit) const {
    const int32_t zx0 = worldgen::floorDiv(ix0 * 8, worldgen::ZONE), zx1 = worldgen::floorDiv(ix1 * 8, worldgen::ZONE);
    const int32_t zy0 = worldgen::floorDiv(iy0 * 8, worldgen::ZONE), zy1 = worldgen::floorDiv(iy1 * 8, worldgen::ZONE);
    for (int32_t zx = zx0; zx <= zx1; zx++)
        for (int32_t zy = zy0; zy <= zy1; zy++) {
            const worldgen::Zone *zone = map.zone(zx, zy);
            if (!zone)
                continue;
            for (int32_t k = 0; k < (int32_t)zone->statics.size(); k++) {
                const worldgen::StaticObject &o = zone->statics[(size_t)k];
                const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                const int32_t ix = worldgen::floorDiv((int32_t)std::floor(at.x), 8), iy = worldgen::floorDiv((int32_t)std::floor(at.y), 8);
                if (ix < ix0 || ix > ix1 || iy < iy0 || iy > iy1)
                    continue;
                const bool turned = ((o.rotation % 2) + 2) % 2 == 1;
                creatures::SearchStatic s;
                s.at = at;
                s.half = 0.5 * glm::dvec2(turned ? o.scale[1] : o.scale[0], turned ? o.scale[0] : o.scale[1]);
                s.height = o.scale[2];
                s.kind = o.kind;
                s.ref = {zx, zy, k};
                const std::vector<StaticState> *live = zone ? this->zone(zx, zy) : nullptr;
                const uint8_t state = live && (size_t)k < live->size() ? (*live)[(size_t)k].state : o.state;
                s.passable = o.kind == 6 || o.kind == 7 || o.kind == 9 ||
                             ((o.kind == 1 || o.kind == 2 || o.kind == 3 || o.kind == 5 || o.kind == 8) && state == 0);
                visit(s);
            }
        }
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
                if (axis >= 0 && (centre[axis] - c[axis]) * move >= 0)
                    continue; // moving away from it, or level with its reference point
                const glm::dvec3 olo(c.x - sx / 2, c.y - sy / 2, c.z), ohi(c.x + sx / 2, c.y + sy / 2, c.z + o.scale[2]);
                if (lo.x < ohi.x && hi.x > olo.x && lo.y < ohi.y && hi.y > olo.y && lo.z < ohi.z && hi.z > olo.z)
                    return true;
            }
        }
    return false;
}

void StaticStates::openDoors(const worldgen::WorldMap &map, glm::dvec3 lo, glm::dvec3 hi) {
    const int32_t zx0 = worldgen::floorDiv((int32_t)std::floor(lo.x) - 16, worldgen::ZONE);
    const int32_t zx1 = worldgen::floorDiv((int32_t)std::floor(hi.x) + 16, worldgen::ZONE);
    const int32_t zy0 = worldgen::floorDiv((int32_t)std::floor(lo.y) - 16, worldgen::ZONE);
    const int32_t zy1 = worldgen::floorDiv((int32_t)std::floor(hi.y) + 16, worldgen::ZONE);
    for (int32_t zx = zx0; zx <= zx1; zx++)
        for (int32_t zy = zy0; zy <= zy1; zy++) {
            const worldgen::Zone *zone = map.zone(zx, zy);
            const auto live = zones.find({zx, zy});
            if (!zone || live == zones.end())
                continue;
            for (size_t k = 0; k < zone->statics.size() && k < live->second.size(); k++) {
                const worldgen::StaticObject &o = zone->statics[k];
                StaticState &s = live->second[k];
                if ((o.kind != 1 && o.kind != 2) || s.state == 0)
                    continue;
                const bool turned = ((o.rotation % 2) + 2) % 2 == 1;
                const double sx = turned ? o.scale[1] : o.scale[0], sy = turned ? o.scale[0] : o.scale[1];
                const glm::dvec3 c = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                if (lo.x < c.x + sx / 2 && hi.x > c.x - sx / 2 && lo.y < c.y + sy / 2 && hi.y > c.y - sy / 2 &&
                    lo.z < c.z + o.scale[2] && hi.z > c.z) {
                    s.state = 0;
                    s.timerMs = 0;
                    changed.insert({zx, zy});
                }
            }
        }
}
