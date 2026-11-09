#pragma once
// A creature's looks (docs/specs/alpha-creatures-models.md §3, §4): the 172-byte appearance
// record and the generator that fills it from the creature type and, for player characters,
// the 20-byte character choice record.
#include "creatures/CreatureTables.h"
#include <array>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace creatures {

constexpr uint16_t NO_MODEL = 65535;

// §3.3
enum AppearanceFlags : uint16_t {
    QUADRUPED = 0x0001,
    FLYER = 0x0002,
    NATURAL_WEAPON = 0x0010,
    SWIMMER = 0x0100,
    GLOWING_EYES = 0x0400,
};

// §3.1. Model indices are slots of the model table (§2).
struct Appearance {
    uint8_t unknown[2] = {0, 0};
    uint8_t hairColour[3] = {255, 255, 255};
    uint16_t flags = 0;
    glm::vec3 box{1, 1, 1}; // collision box, blocks
    // By Part. The spec's defaults are 0 (body2.cub, open question M-1); renders show parts a
    // type never sets drawn as body2 pieces, so they start as "no model" here.
    uint16_t models[8] = {NO_MODEL, NO_MODEL, NO_MODEL, NO_MODEL, NO_MODEL, NO_MODEL, NO_MODEL, NO_MODEL};
    float headScale = 1.01f, bodyScale = 1, handScale = 1, footScale = 0.98f, armScale = 1, weaponScale = 0.95f,
          tailScale = 0.8f, shoulderScale = 1, wingScale = 1;
    float bodyPitch = 0;
    glm::vec3 handRotation{0};
    float footAngle = 0, wingAngle = 0, tailAngle = 0;
    glm::vec3 bodyOffset{0, 0, -5}, headOffset{0, 0.5f, 5}, handOffset{6, 0, 0}, footOffset{3, 1, -10.5f},
        tailOffset{0, -8, 2}, wingOffset{0};

    uint16_t &model(Part p) { return models[(int)p]; }
    uint16_t model(Part p) const { return models[(int)p]; }

    // The record as it travels in creature records and saves (little-endian, 172 bytes).
    static Appearance parse(const uint8_t *bytes);
    void serialize(uint8_t *bytes) const;
};

// §4.1: the character choice record (saved character field 13).
struct CharacterChoice {
    int32_t race = 0;
    bool female = false;
    int32_t head = 0, hair = 0;
    uint8_t hairColour[3] = {255, 255, 255};
    static CharacterChoice parse(const uint8_t *bytes);
};

// §4: fills `out` (starting from its current values, normally the defaults) for creature
// `type`, which a choice record may change (race and sex). `draw` is the caller's rand().
void generate(int32_t &type, Appearance &out, const CharacterChoice *choice, const std::function<int()> &draw);

} // namespace creatures
