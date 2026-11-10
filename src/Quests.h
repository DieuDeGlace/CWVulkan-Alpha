#pragma once
// Quests (docs/design/quests.md): AlphaVulkan's own quest board on top of Alpha's missions.
// Town residents offer rescues, battles beside townspeople, hunts and champions to slay; the player takes them by
// talking, does the job out in the world and hands them in to the giver for a reward.
//
// Everything here is plain state and rules, used alike by an offline game and by the server:
// which residents give quests, what each offers on a day, the creatures of a quest, progress,
// rewards and the saved value.
#include "worldgen/Random.h"
#include "worldgen/Zone.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace quests {

enum Kind : uint8_t { RESCUE = 1, BATTLE = 2, HUNT = 3, CHAMPION = 4 };
enum State : uint8_t { ACTIVE = 1, DONE = 2 }; // a handed-in quest leaves the book
// What a quest creature is to its quest (worldgen::Spawn::questRole, Npc::questRole).
enum Role : uint8_t { NO_ROLE = 0, ENEMY = 1, PRISONER = 2, ALLY = 3, LEADER = 4 };

constexpr int MAX_HELD = 3;       // quests a player holds at once
constexpr double FREE_CLEAR = 15; // no living guard this close to a prisoner (blocks)
constexpr int32_t CHAMPION_LEVELS = 3; // a champion is this many levels above its land

struct Quest {
    uint32_t id = 0; // from the giver and the day; never 0
    Kind kind = HUNT;
    State state = ACTIVE;
    uint64_t giver = 0;      // the resident's creature id
    int32_t giverType = 2;   // and its creature type (its name comes from both)
    glm::dvec3 giverAt{0};   // where it lives (blocks)
    glm::dvec3 site{0};      // the place (blocks; z set when its creatures are made)
    int32_t day = 0;         // the day it was offered
    int32_t level = 1, tier = 0;
    int32_t enemyType = 0, allyType = 0;
    int32_t enemies = 0, prisoners = 0, allies = 0; // creatures made at the place
    int32_t required = 0, count = 0;                // progress: freed prisoners, kills, or the champion
    bool placed = false;     // its creatures exist

    bool operator==(const Quest &o) const = default;
};

// Whether a town creature gives quests: residents (class 0x88), one in three by spawn id.
bool isGiver(uint64_t creatureId, uint8_t characterClass);

// The quest a giver offers on `day` (the same for every player of the world). `level` and
// `tier` are those of the giver's land.
Quest offer(uint32_t worldSeed, uint64_t giver, int32_t giverType, glm::dvec3 giverAt, int32_t day, int32_t level,
            int32_t tier);

// The quest's creatures round its place; `ground(x, y)` gives the height to stand on (the
// first open block over the ground). Their spawn records carry the quest and their role.
std::vector<worldgen::Spawn> creatures(const Quest &q, const std::function<int32_t(int32_t x, int32_t y)> &ground);

// "north-east" and so on, from `from` toward `to`.
std::string direction(glm::dvec3 from, glm::dvec3 to);
// The giver's name (the game's generated title of its id and type).
std::string giverName(const Quest &q);
// What the giver says when offering, the quest tag's objective, and the hand-in words.
// `enemies` is the plural name of the quest's creatures ("monsters" when empty).
std::string offerText(const Quest &q, glm::dvec3 giverAt, const std::string &enemies = {});
std::u16string objectiveText(const Quest &q, glm::dvec3 playerAt, const std::string &enemies = {});
std::string thanksText(const Quest &q);

// The reward of a handed-in quest for a player of `characterClass`: Alpha's mission reward at
// the quest's level (Platinum Coins first, then a piece of gear), then the coins.
std::vector<worldgen::ItemRecord> rewards(const Quest &q, int32_t characterClass, worldgen::Random &random);

// One player's quests.
class Book {
  public:
    std::vector<Quest> quests;
    std::vector<uint32_t> handedIn; // quests done and handed in (a giver does not offer them again)

    Quest *find(uint32_t id);
    Quest *byGiver(uint64_t giver);
    bool full() const { return (int)quests.size() >= MAX_HELD; }
    // Progress: a quest creature died, or a prisoner was freed. True when the quest just
    // became done.
    bool died(uint32_t quest, uint8_t role);
    bool freed(uint32_t quest);
    // The saved value (`av_quests`), and reading it back.
    std::vector<uint8_t> save() const;
    static Book load(const std::vector<uint8_t> &bytes);
};

} // namespace quests
