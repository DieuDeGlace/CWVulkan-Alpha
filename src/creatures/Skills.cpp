#include "creatures/Skills.h"
#include "creatures/Combat.h"
#include <algorithm>

namespace creatures {

int32_t skillParent(int32_t i) {
    switch (i) {
    case 1: return 0;
    case 3: return 2;
    case 5: return 4;
    case 7: return 6;
    case 8: return 7;
    case 9: return 8;
    default: return -1;
    }
}

int32_t skillAbility(int32_t i, int32_t cls, int32_t spec) {
    static const int32_t TABLE[4][3][2] = {
        {{54, 54}, {86, 86}, {102, 101}},  // warrior: smash, cyclone, war frenzy / bulwark
        {{21, 21}, {50, 50}, {99, 100}},   // ranger: kick, retreat, aim / swiftness
        {{88, 34}, {103, 103}, {49, 49}},  // mage: fire explosion / healing stream, mana shield, teleport
        {{48, 48}, {79, 79}, {97, 96}},    // rogue: intercept, sneak, camouflage / shuriken
    };
    if (cls < 1 || cls > 4 || i < 6 || i > 8)
        return 0;
    return TABLE[cls - 1][i - 6][spec ? 1 : 0];
}

bool skillLocked(const SkillLevels &draft, int32_t i) {
    const int32_t parent = skillParent(i);
    return parent >= 0 && draft[parent] < 5;
}

int32_t skillPoints(int32_t level, int32_t cubes) { return 2 * level - 2 + std::max(cubes, 0) / 4; }

int32_t pointsUsed(const SkillLevels &draft) {
    int32_t used = 0;
    for (int32_t v : draft)
        used += v;
    return used;
}

bool raiseSkill(SkillLevels &draft, int32_t i, int32_t available) {
    if (pointsUsed(draft) >= available || skillLocked(draft, i))
        return false;
    draft[i]++;
    return true;
}

bool lowerSkill(SkillLevels &draft, int32_t i) {
    // A parent at 5 or less may not drop while its child has points.
    for (int32_t child = 0; child < 11; child++)
        if (skillParent(child) == i && draft[i] <= 5 && draft[child] > 0)
            return false;
    draft[i] = std::max(draft[i] - 1, 0);
    return true;
}

int32_t skillChangeCost(int32_t level, const SkillLevels &committed, int32_t committedSpec, const SkillLevels &draft,
                        int32_t draftSpec) {
    const float F = itemLevelFactor((float)level, 0);
    float cost = 0;
    for (size_t i = 0; i < draft.size(); i++)
        if (draft[i] < committed[i])
            cost += F * (float)(committed[i] - draft[i]) * 10.f;
    if (draftSpec != committedSpec && level > 1)
        cost += F * 100.f;
    return (int32_t)cost;
}

} // namespace creatures
