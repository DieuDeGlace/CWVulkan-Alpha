#pragma once
// Skill points (docs/specs/alpha-skills.md §3, §4): the budget, the tree's prerequisites, raising
// and lowering a draft, and what committing it costs. The window edits a draft; nothing changes
// on the player until Learn.
#include <array>
#include <cstdint>

namespace creatures {

using SkillLevels = std::array<int32_t, 11>;

// §2.1: the skill a child needs at level 5 first, or -1 for a root.
int32_t skillParent(int32_t i);
// §2.1: the ability ids of skills 6, 7 and 8 for a class and specialization (0: none).
int32_t skillAbility(int32_t i, int32_t characterClass, int32_t specialization);
// §2.4: a child whose parent is below 5 is locked.
bool skillLocked(const SkillLevels &draft, int32_t i);
// §3.1: the points a character may spend, 2·L − 2 and one per four mana cubes.
int32_t skillPoints(int32_t level, int32_t manaCubes);
int32_t pointsUsed(const SkillLevels &draft);
// §4.2, §4.3: a left press raises (false when refused), a right press lowers.
bool raiseSkill(SkillLevels &draft, int32_t i, int32_t available);
bool lowerSkill(SkillLevels &draft, int32_t i);
// §4.4: the copper a change costs: 10·F(L) per point below the committed level and 100·F(L)
// for another specialization (free at level 1).
int32_t skillChangeCost(int32_t level, const SkillLevels &committed, int32_t committedSpecialization,
                        const SkillLevels &draft, int32_t draftSpecialization);

} // namespace creatures
