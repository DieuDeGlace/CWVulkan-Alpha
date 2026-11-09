#pragma once
// The two tables of docs/specs/alpha-creatures-models.md, generated into CreatureTables.cpp by
// tools/gen_creature_tables.py: the model table (Appendix A) and the per-type step of the
// appearance generator (Appendix B).
#include <cstdint>

namespace creatures {

// §2: model slots, by index; nullptr for slots never loaded.
constexpr int MODEL_SLOTS = 2569;
extern const char *const MODEL_KEYS[MODEL_SLOTS];

enum class Part : uint8_t { Head, Hair, Hand, Foot, Body, Tail, Arm, Wing };

// Appendix B: how one part's model is chosen.
struct ModelRule {
    Part part = Part::Head;
    uint8_t source = 0; // 0 fixed (base), 1 head choice, 2 hair choice, 3 a draw: base + value mod `mod`
    int32_t base = 0;
    int32_t mod = 0;
};

// Appendix B: one creature type's step. A set bit in a mask means the value is written.
struct TypeRecipe {
    uint8_t ruleCount = 0;
    ModelRule rules[8] = {};
    uint16_t flags = 0;
    // head, body, hand, foot, arm, weapon, tail, shoulder, wing, body pitch
    uint16_t scalarMask = 0;
    float scalars[10] = {};
    // box, hand rotation, angles (foot, wing, tail), body, head, hand, foot, tail, wing offsets;
    // bit 3 v + k for component k of vector v
    uint32_t vectorMask = 0;
    float vectors[27] = {};
};
constexpr int TYPE_COUNT = 156;
extern const TypeRecipe TYPE_RECIPES[TYPE_COUNT];

} // namespace creatures
