#include "Quests.h"
#include "Missions.h"
#include "creatures/Names.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace quests {

namespace {

constexpr double PI = 3.141592653589793;

// FNV-1a over the bytes of a few numbers: the board's seeds.
uint32_t mix(std::initializer_list<uint64_t> values) {
    uint32_t h = 2166136261u;
    for (uint64_t v : values)
        for (int k = 0; k < 8; k++) {
            h ^= (uint8_t)(v >> (8 * k));
            h *= 16777619u;
        }
    return h;
}

std::u16string wide(const std::string &s) { return std::u16string(s.begin(), s.end()); }

// Townspeople of the playable races (models §4.3 types 0-3, 7-10).
int32_t townRace(worldgen::Random &random) {
    static const int32_t races[8] = {0, 1, 2, 3, 7, 8, 9, 10};
    return races[random.draw() % 8];
}

worldgen::Spawn spawnAt(const Quest &q, glm::dvec2 at, int32_t z, int32_t type, int32_t controller, uint8_t role,
                        float yaw) {
    worldgen::Spawn s;
    s.x = (int64_t)(at.x * 65536.0), s.y = (int64_t)(at.y * 65536.0), s.z = (int64_t)z * 65536;
    s.type = type;
    s.controller = controller;
    s.level = q.level;
    s.powerTier = (uint8_t)q.tier;
    s.characterClass = controller == 6 ? 0 : 1;
    s.yaw = yaw;
    s.quest = q.id;
    s.questRole = role;
    return s;
}

} // namespace

bool isGiver(uint64_t creatureId, uint8_t characterClass) {
    return characterClass == 0x88 && mix({creatureId}) % 3 == 0;
}

Quest offer(uint32_t worldSeed, uint64_t giver, int32_t giverType, glm::dvec3 giverAt, int32_t day, int32_t level,
            int32_t tier) {
    worldgen::Random random(mix({worldSeed, giver, (uint64_t)(uint32_t)day}));
    Quest q;
    q.id = mix({giver, (uint64_t)(uint32_t)day, 0x51u}) | 1u;
    q.giver = giver;
    q.giverType = giverType;
    q.giverAt = giverAt;
    q.day = day;
    q.level = std::max(level, 1);
    q.tier = tier;
    q.kind = (Kind)(1 + random.draw() % 4);
    const double distance = 120 + random.draw() % 181;
    const double angle = random.draw() * 2 * PI / 32767.0;
    q.site = giverAt + glm::dvec3(std::cos(angle) * distance, std::sin(angle) * distance, 0);
    q.enemyType = missionCreature(random);
    q.allyType = townRace(random);
    switch (q.kind) {
    case RESCUE:
        q.enemies = 4 + random.draw() % 3;
        q.prisoners = 1 + random.draw() % 3;
        q.required = q.prisoners;
        break;
    case BATTLE:
        q.allies = 4 + random.draw() % 3;
        q.enemies = 5 + random.draw() % 3;
        q.required = q.enemies;
        break;
    case HUNT:
        q.enemies = 5 + random.draw() % 5;
        q.required = q.enemies;
        break;
    case CHAMPION: // the leader, and its followers (who do not count)
        q.enemies = 2 + random.draw() % 3;
        q.required = 1;
        break;
    }
    return q;
}

std::vector<worldgen::Spawn> creatures(const Quest &q, const std::function<int32_t(int32_t, int32_t)> &ground) {
    worldgen::Random random(q.id);
    std::vector<worldgen::Spawn> out;
    const glm::dvec2 centre(q.site);
    const auto at = [&](glm::dvec2 p) { return ground((int32_t)std::floor(p.x), (int32_t)std::floor(p.y)); };
    const auto ring = [&](int count, double radius, double turn) {
        std::vector<std::pair<glm::dvec2, float>> points;
        for (int k = 0; k < count; k++) {
            const double a = turn + 2 * PI * k / count;
            points.push_back({centre + radius * glm::dvec2(std::cos(a), std::sin(a)), (float)(a * 180 / PI - 90)});
        }
        return points;
    };
    // (A quest read back from a save makes only what is left to do.)
    const int left = std::max(q.required - q.count, 0);
    switch (q.kind) {
    case RESCUE: // the prisoners sit in the middle, the guards stand round them
        for (const auto &[p, yaw] : ring(left, left > 1 ? 2.0 : 0.0, 0))
            out.push_back(spawnAt(q, p, at(p), townRace(random), 6, PRISONER, yaw + 180));
        for (const auto &[p, yaw] : ring(q.enemies, 6.0, 0.4))
            out.push_back(spawnAt(q, p, at(p), q.enemyType, 1, ENEMY, yaw));
        break;
    case BATTLE: { // two lines 16 blocks apart, facing each other
        for (int k = 0; k < q.allies; k++) {
            const glm::dvec2 p = centre + glm::dvec2(-8, 2.5 * (k - (q.allies - 1) / 2.0));
            out.push_back(spawnAt(q, p, at(p), townRace(random), 3, ALLY, -90));
        }
        for (int k = 0; k < q.enemies; k++) {
            const glm::dvec2 p = centre + glm::dvec2(8, 2.5 * (k - (q.enemies - 1) / 2.0));
            out.push_back(spawnAt(q, p, at(p), q.enemyType, 1, ENEMY, 90));
        }
        break;
    }
    case HUNT: // scattered round the place
        for (int k = 0; k < left; k++) {
            const double a = random.draw() * 2 * PI / 32767.0, r = 3 + random.draw() % 12;
            const glm::dvec2 p = centre + r * glm::dvec2(std::cos(a), std::sin(a));
            out.push_back(spawnAt(q, p, at(p), q.enemyType, 1, ENEMY, (float)(random.draw() % 360)));
        }
        break;
    case CHAMPION: // the champion in the middle, its followers round it
        if (left > 0) {
            worldgen::Spawn leader = spawnAt(q, centre, at(centre), q.enemyType, 1, LEADER, 0);
            leader.level = q.level + CHAMPION_LEVELS;
            out.push_back(leader);
            for (const auto &[p, yaw] : ring(q.enemies, 4.0, 0.3))
                out.push_back(spawnAt(q, p, at(p), q.enemyType, 1, ENEMY, yaw));
        }
        break;
    }
    return out;
}

std::string direction(glm::dvec3 from, glm::dvec3 to) {
    static const char *const NAMES[8] = {"east", "north-east", "north", "north-west",
                                         "west", "south-west", "south", "south-east"};
    const glm::dvec2 d = glm::dvec2(to - from);
    if (glm::dot(d, d) < 1)
        return "here";
    // (+y is north on Alpha's map.)
    double a = std::atan2(d.y, d.x);
    if (a < 0)
        a += 2 * PI;
    return NAMES[(int)std::floor(a / (PI / 4) + 0.5) % 8];
}

std::string giverName(const Quest &q) { return creatures::generatedTitle(q.giver, q.giverType); }

std::string offerText(const Quest &q, glm::dvec3 giverAt, const std::string &named) {
    const std::string enemies = named.empty() ? "monsters" : named;
    const std::string where = std::to_string((int)glm::length(glm::dvec2(q.site - giverAt))) + " blocks " +
                              direction(giverAt, q.site);
    switch (q.kind) {
    case RESCUE:
        return "I am " + giverName(q) + ". " + enemies + " took " +
               std::string(q.prisoners == 1 ? "one of our people" : std::to_string(q.prisoners) + " of our people") +
               "! They keep them in a camp " + where + " of here. Please set them free!";
    case BATTLE:
        return "I am " + giverName(q) + ". Our fighters are about to clash with a band of " + enemies + " " + where +
               " of here. Go and fight beside them!";
    case CHAMPION:
        return "I am " + giverName(q) + ". A champion of the " + enemies + " and its followers camp " + where +
               " of here. Slay the champion!";
    case HUNT:
    default:
        return "I am " + giverName(q) + ". A pack of " + std::to_string(q.enemies) + " " + enemies + " roams " + where +
               " of here. Hunt them down for us!";
    }
}

std::u16string objectiveText(const Quest &q, glm::dvec3 playerAt, const std::string &named) {
    if (q.state == DONE)
        return wide("Return to " + giverName(q));
    const std::string enemies = named.empty() ? "monsters" : named;
    const std::string where = " (" + std::to_string((int)glm::length(glm::dvec2(q.site - playerAt))) + " blocks " +
                              direction(playerAt, q.site) + ")";
    const std::string progress = " " + std::to_string(q.count) + "/" + std::to_string(q.required);
    switch (q.kind) {
    case RESCUE: return wide("Free the prisoners" + progress + where);
    case BATTLE: return wide("Win the battle" + progress + where);
    case CHAMPION: return wide("Slay the champion of the " + enemies + progress + where);
    case HUNT:
    default: return wide("Hunt the " + enemies + progress + where);
    }
}

std::string thanksText(const Quest &q) {
    switch (q.kind) {
    case RESCUE: return "You brought them home safe. Thank you, hero! Take this.";
    case BATTLE: return "We won the day thanks to you! Here, you earned it.";
    case CHAMPION: return "Without their champion they will scatter. Well fought! Take this.";
    case HUNT:
    default: return "The roads are safe again. Here is your reward.";
    }
}

std::vector<worldgen::ItemRecord> rewards(const Quest &q, int32_t characterClass, worldgen::Random &random) {
    MissionArea m;
    m.kind = 1;
    m.level = q.level;
    m.tier = (uint8_t)q.tier;
    std::vector<worldgen::ItemRecord> out = missionRewards(m, characterClass, random);
    worldgen::ItemRecord coins;
    coins.type = 12;
    coins.material = 12; // silver
    coins.level = (int16_t)std::min(5 + 5 * q.level * (q.kind == HUNT ? 1 : 2), 30000);
    out.push_back(coins);
    return out;
}

Quest *Book::find(uint32_t id) {
    for (Quest &q : quests)
        if (q.id == id)
            return &q;
    return nullptr;
}

Quest *Book::byGiver(uint64_t giver) {
    for (Quest &q : quests)
        if (q.giver == giver)
            return &q;
    return nullptr;
}

bool Book::died(uint32_t quest, uint8_t role) {
    Quest *q = find(quest);
    if (!q || q->state != ACTIVE || q->kind == RESCUE)
        return false;
    if (role != (q->kind == CHAMPION ? LEADER : ENEMY)) // a champion's followers do not count
        return false;
    q->count = std::min(q->count + 1, q->required);
    if (q->count < q->required)
        return false;
    q->state = DONE;
    return true;
}

bool Book::freed(uint32_t quest) {
    Quest *q = find(quest);
    if (!q || q->state != ACTIVE || q->kind != RESCUE)
        return false;
    q->count = std::min(q->count + 1, q->required);
    if (q->count < q->required)
        return false;
    q->state = DONE;
    return true;
}

// The value: a version byte, then per quest its fields in order (little endian).
namespace {
template <typename T> void put(std::vector<uint8_t> &out, T v) {
    const size_t at = out.size();
    out.resize(at + sizeof v);
    std::memcpy(&out[at], &v, sizeof v);
}
template <typename T> bool get(const std::vector<uint8_t> &in, size_t &at, T &v) {
    if (at + sizeof v > in.size())
        return false;
    std::memcpy(&v, &in[at], sizeof v);
    at += sizeof v;
    return true;
}
} // namespace

std::vector<uint8_t> Book::save() const {
    std::vector<uint8_t> out{2};
    put(out, (uint32_t)quests.size());
    for (const Quest &q : quests) {
        put(out, q.id), put(out, (uint8_t)q.kind), put(out, (uint8_t)q.state), put(out, q.giver), put(out, q.giverType);
        for (int k = 0; k < 3; k++)
            put(out, q.giverAt[k]);
        for (int k = 0; k < 3; k++)
            put(out, q.site[k]);
        put(out, q.day), put(out, q.level), put(out, q.tier), put(out, q.enemyType), put(out, q.allyType);
        put(out, q.enemies), put(out, q.prisoners), put(out, q.allies), put(out, q.required), put(out, q.count);
    }
    put(out, (uint32_t)handedIn.size());
    for (uint32_t id : handedIn)
        put(out, id);
    return out;
}

Book Book::load(const std::vector<uint8_t> &in) {
    Book book;
    size_t at = 0;
    uint8_t version = 0;
    uint32_t n = 0;
    if (!get(in, at, version) || (version != 1 && version != 2) || !get(in, at, n))
        return book;
    for (uint32_t i = 0; i < n; i++) {
        Quest q;
        uint8_t kind = 0, state = 0;
        bool ok = get(in, at, q.id) && get(in, at, kind) && get(in, at, state) && get(in, at, q.giver) &&
                  (version < 2 || get(in, at, q.giverType));
        for (int k = 0; k < 3; k++)
            ok = ok && get(in, at, q.giverAt[k]);
        for (int k = 0; k < 3; k++)
            ok = ok && get(in, at, q.site[k]);
        ok = ok && get(in, at, q.day) && get(in, at, q.level) && get(in, at, q.tier) && get(in, at, q.enemyType) &&
             get(in, at, q.allyType) && get(in, at, q.enemies) && get(in, at, q.prisoners) && get(in, at, q.allies) &&
             get(in, at, q.required) && get(in, at, q.count);
        if (!ok)
            break;
        q.kind = (Kind)kind;
        q.state = (State)state;
        book.quests.push_back(q); // (its creatures are made again when its place loads)
    }
    uint32_t done = 0;
    if (get(in, at, done))
        for (uint32_t i = 0; i < done; i++)
            if (uint32_t id = 0; get(in, at, id))
                book.handedIn.push_back(id);
    return book;
}

} // namespace quests
