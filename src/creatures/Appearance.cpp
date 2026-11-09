#include "creatures/Appearance.h"
#include <bit>
#include <cstring>

namespace creatures {
namespace {

template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }
glm::vec3 vec(const uint8_t *p, size_t at) { return {get<float>(p, at), get<float>(p, at + 4), get<float>(p, at + 8)}; }
void putVec(uint8_t *p, size_t at, glm::vec3 v) {
    put(p, at, v.x);
    put(p, at + 4, v.y);
    put(p, at + 8, v.z);
}

// The scalars and vectors of Appendix B, in the order the generated table keeps them.
float *scalar(Appearance &a, int i) {
    float *s[] = {&a.headScale, &a.bodyScale, &a.handScale, &a.footScale,
                  &a.armScale,  &a.weaponScale, &a.tailScale, &a.shoulderScale, &a.wingScale, &a.bodyPitch};
    return s[i];
}
float *component(Appearance &a, int v, int k) {
    switch (v) {
    case 0: return &a.box[k];
    case 1: return &a.handRotation[k];
    case 2: return k == 0 ? &a.footAngle : k == 1 ? &a.wingAngle : &a.tailAngle;
    case 3: return &a.bodyOffset[k];
    case 4: return &a.headOffset[k];
    case 5: return &a.handOffset[k];
    case 6: return &a.footOffset[k];
    case 7: return &a.tailOffset[k];
    default: return &a.wingOffset[k];
    }
}
float unitDraw(int d) { return (float)d / 32767.f; }

} // namespace

Appearance Appearance::parse(const uint8_t *p) {
    Appearance a;
    a.unknown[0] = p[0];
    a.unknown[1] = p[1];
    std::memcpy(a.hairColour, p + 2, 3);
    a.flags = get<uint16_t>(p, 0x06);
    a.box = vec(p, 0x08);
    for (int i = 0; i < 8; i++)
        a.models[i] = get<uint16_t>(p, 0x14 + 2 * i);
    float *s[] = {&a.headScale,  &a.bodyScale,   &a.handScale,     &a.footScale, &a.armScale,
                  &a.weaponScale, &a.tailScale, &a.shoulderScale, &a.wingScale, &a.bodyPitch};
    for (int i = 0; i < 10; i++)
        *s[i] = get<float>(p, 0x24 + 4 * i);
    a.handRotation = vec(p, 0x4c);
    a.footAngle = get<float>(p, 0x58);
    a.wingAngle = get<float>(p, 0x5c);
    a.tailAngle = get<float>(p, 0x60);
    a.bodyOffset = vec(p, 0x64);
    a.headOffset = vec(p, 0x70);
    a.handOffset = vec(p, 0x7c);
    a.footOffset = vec(p, 0x88);
    a.tailOffset = vec(p, 0x94);
    a.wingOffset = vec(p, 0xa0);
    return a;
}

void Appearance::serialize(uint8_t *p) const {
    std::memset(p, 0, 172);
    p[0] = unknown[0];
    p[1] = unknown[1];
    std::memcpy(p + 2, hairColour, 3);
    put(p, 0x06, flags);
    putVec(p, 0x08, box);
    for (int i = 0; i < 8; i++)
        put(p, 0x14 + 2 * i, models[i]);
    const float s[] = {headScale, bodyScale, handScale, footScale, armScale, weaponScale, tailScale, shoulderScale,
                       wingScale, bodyPitch};
    for (int i = 0; i < 10; i++)
        put(p, 0x24 + 4 * i, s[i]);
    putVec(p, 0x4c, handRotation);
    put(p, 0x58, footAngle);
    put(p, 0x5c, wingAngle);
    put(p, 0x60, tailAngle);
    putVec(p, 0x64, bodyOffset);
    putVec(p, 0x70, headOffset);
    putVec(p, 0x7c, handOffset);
    putVec(p, 0x88, footOffset);
    putVec(p, 0x94, tailOffset);
    putVec(p, 0xa0, wingOffset);
}

CharacterChoice CharacterChoice::parse(const uint8_t *p) {
    CharacterChoice c;
    c.race = get<int32_t>(p, 0x00);
    c.female = (p[0x04] & 1) != 0;
    c.head = get<int32_t>(p, 0x08);
    c.hair = get<int32_t>(p, 0x0c);
    std::memcpy(c.hairColour, p + 0x10, 3);
    return c;
}

void generate(int32_t &type, Appearance &a, const CharacterChoice *given, const std::function<int()> &draw) {
    // §4.1: the choice record, or one made from five draws for NPCs (race and sex 0, the type
    // unchanged).
    CharacterChoice choice;
    if (given) {
        choice = *given;
        static const int32_t BASE[8] = {2, 0, 9, 11, 4, 7, 15, 13};
        if (choice.race >= 0 && choice.race < 8)
            type = BASE[choice.race] + (choice.female ? 1 : 0);
    } else {
        choice.hairColour[2] = (uint8_t)draw();
        choice.hairColour[1] = (uint8_t)draw();
        choice.hairColour[0] = (uint8_t)draw();
        choice.head = draw() % 100;
        choice.hair = draw() % 100;
    }
    // §4.2
    // (Alpha's width is the float 0x3f75c290, one step above the float nearest 0.96, as its
    // recorded creatures show.)
    a.box = {std::bit_cast<float>(0x3f75c290u), std::bit_cast<float>(0x3f75c290u), 2.16f};
    if (type < 0 || type >= TYPE_COUNT)
        return;
    const bool copiesColour = (type >= 0 && type <= 5) || (type >= 7 && type <= 12) || type == 148 || type == 149 ||
                              type == 15 || type == 16;
    if (copiesColour)
        std::memcpy(a.hairColour, choice.hairColour, 3);
    if (type == 13 || type == 14)
        a.hairColour[0] = a.hairColour[1] = a.hairColour[2] = 255;
    if (type == 6 || type == 46) { // R = draw 3, G = draw 2, B = draw 1
        const int d1 = draw(), d2 = draw(), d3 = draw();
        a.hairColour[0] = (uint8_t)d3;
        a.hairColour[1] = (uint8_t)d2;
        a.hairColour[2] = (uint8_t)d1;
    }

    const TypeRecipe &r = TYPE_RECIPES[type];
    if (type == 119) {
        // The reptile monster: models from draws 1-4 (hand and foot share draw 2, which makes
        // the spec's draw numbers 5-8 for the head scale and the offsets line up).
        a.model(Part::Head) = (uint16_t)(2145 + draw() % 5);
        a.model(Part::Hand) = a.model(Part::Foot) = (uint16_t)(2142 + draw() % 3);
        a.model(Part::Body) = (uint16_t)(2133 + draw() % 9);
        a.model(Part::Tail) = (uint16_t)(2150 + draw() % 2);
    } else {
        for (int i = 0; i < r.ruleCount; i++) {
            const ModelRule &rule = r.rules[i];
            int32_t index = rule.base;
            if (rule.source == 1)
                index += choice.head % rule.mod;
            else if (rule.source == 2)
                index += choice.hair % rule.mod;
            else if (rule.source == 3)
                index += draw() % rule.mod;
            a.model(rule.part) = (uint16_t)index;
        }
    }
    a.flags |= r.flags;
    for (int i = 0; i < 10; i++)
        if (r.scalarMask & (1u << i))
            *scalar(a, i) = r.scalars[i];
    for (int v = 0; v < 9; v++)
        for (int k = 0; k < 3; k++)
            if (r.vectorMask & (1u << (3 * v + k)))
                *component(a, v, k) = r.vectors[3 * v + k];

    // The rules Appendix B states in words.
    switch (type) {
    case 2:
        a.model(Part::Hand) = a.model(Part::Head) > 1248 ? 431 : 430;
        break;
    case 3:
        a.model(Part::Hand) = a.model(Part::Head) > 1269 ? 431 : 430;
        break;
    case 83:
        a.model(Part::Hand) = a.model(Part::Head) != 180 ? 189 : 188;
        break;
    case 84:
        a.model(Part::Hand) = a.model(Part::Head) != 183 ? 189 : 188;
        break;
    case 58:
        if ((draw() & 1) == 0) { // the blue parrot
            a.model(Part::Body) = 270;
            a.model(Part::Head) = 271;
            a.model(Part::Foot) = 272;
            a.model(Part::Wing) = 273;
        }
        break;
    case 116: {
        const float s = 3.f * unitDraw(draw()) + 7.f;
        a.box = {s, s, 1.12f * s};
        break;
    }
    case 119:
        a.headScale = 0.1f * unitDraw(draw()) + 0.4f;
        a.headOffset = {0, 8, 0.5f * unitDraw(draw()) - 1.f};
        a.handOffset = {unitDraw(draw()) + 3.f, 3, -4.1f};
        a.footOffset = {unitDraw(draw()) + 3.f, -3, -4.1f};
        break;
    case 124:
        a.model(Part::Body) = (draw() & 1) ? 2093 : 2094;
        break;
    case 145:
    case 146: {
        const float s = (0.5f * unitDraw(draw()) + 1.2f) * 0.8f;
        a.box = {s, s, s};
        break;
    }
    default:
        break;
    }
}

} // namespace creatures
