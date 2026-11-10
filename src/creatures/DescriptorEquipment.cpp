#include "creatures/DescriptorEquipment.h"
#include <bit>
#include <cstring>
#include <utility>

namespace creatures::equipment {
namespace {
constexpr size_t offHand = 6, mainHand = 7;
void header(ItemRecord &item, uint16_t value) { std::memcpy(item.data(), &value, 2); }
void level(ItemRecord &item, uint16_t value) { std::memcpy(item.data() + 0x10, &value, 2); }
void seed(ItemRecord &item, int32_t value) { std::memcpy(item.data() + 4, &value, 4); }

void optionalEquipment(Descriptor &d, const std::function<int32_t()> &draw) {
    if (d.flags & 0x20)
        return;
    const uint8_t subtype = d.items[mainHand][1];
    uint8_t material = 1;
    if (subtype == 10 || subtype == 11 || subtype == 12) material = 25;
    else if (subtype == 6 || subtype == 7 || subtype == 8) material = 26;
    else if (subtype == 3 || subtype == 4 || subtype == 5) material = 27;
    d.items[2][0] = 4;
    level(d.items[2], d.level);
    d.items[2][0x0d] = material;
    for (const auto [slot, type] : {std::pair<size_t, uint8_t>{5, 7}, {4, 5}, {3, 6}}) {
        if (draw() % 2 != 0) {
            level(d.items[slot], d.level);
            d.items[slot][0] = type;
            d.items[slot][0x0d] = material;
        }
    }
    for (const auto [slot, type] : {std::pair<size_t, uint8_t>{1, 8}, {9, 9}, {8, 9}}) {
        if (draw() % 10 == 0) {
            d.items[slot][0] = type;
            level(d.items[slot], d.level);
            d.items[slot][0x0d] = draw() % 2 == 0 ? 12 : 11;
        }
    }
}

void randomEquipment(Descriptor &d, const std::function<int32_t()> &draw) {
    auto &u = d.items[mainHand];
    auto &v = d.items[offHand];
    const int32_t classificationDraw = draw();
    const bool forced = d.kind == 0x61;
    d.classification = uint8_t(classificationDraw % 4 + 1);
    if (forced)
        d.classification = 4;
    if (d.classification >= 1 && d.classification <= 4)
        d.companion = uint8_t(draw() % 2);
    switch (d.classification) {
    case 1:
        switch (draw() % 3) {
        case 0: {
            u[0] = 3;
            const int32_t value = draw();
            u[0x0d] = 1;
            header(v, 0);
            u[1] = uint8_t(value % 3 + 15);
            break;
        }
        case 1: {
            u[0] = 3;
            const int32_t first = draw();
            u[0x0d] = 1;
            v[0] = 3;
            u[1] = uint8_t(first % 3);
            v[1] = uint8_t(draw() % 3);
            v[0x0d] = 1;
            break;
        }
        case 2: {
            u[0] = 3;
            const int32_t value = draw();
            u[0x0d] = 1;
            header(v, 0x0d03);
            u[1] = uint8_t(value % 3);
            v[0x0d] = 1;
            break;
        }
        default: break;
        }
        break;
    case 2: {
        u[0] = 3;
        const int32_t value = draw();
        u[0x0d] = 2;
        u[1] = uint8_t(value % 3 + 6);
        break;
    }
    case 3: {
        const int32_t selection = draw() % 3;
        switch (selection) {
        case 0: case 1:
            u[1] = selection == 0 ? 10 : 11;
            u[0] = 3; u[0x0d] = 2; header(v, 0);
            break;
        case 2:
            u[0] = 3; u[1] = 12; u[0x0d] = 11;
            header(v, 0x0c03); v[0x0d] = 11;
            break;
        default: break;
        }
        break;
    }
    case 4: {
        u[0] = 3;
        const int32_t value = draw();
        u[0x0d] = 1;
        u[1] = uint8_t(value % 3 + 3);
        if (u[1] == 5) {
            header(v, 0);
            v[0x0d] = 0;
        } else {
            v[0] = 3;
            v[1] = u[1];
            v[0x0d] = 1;
        }
        break;
    }
    default: break;
    }
    optionalEquipment(d, draw);
}

void hands(Descriptor &d, const std::function<int32_t()> &draw) {
    auto &u = d.items[mainHand];
    auto &v = d.items[offHand];
    if (d.flags & 0x18) {
        header(v, 0); u[0] = 0; u[1] = 0;
        return;
    }
    if (d.role == 6)
        return;
    switch (d.kind) {
    case 0x76: header(v, 0); u[0] = 0; u[1] = 0; break;
    case 0x75:
        d.actions.push_back(0x5c); d.choice = 0;
        d.candidates.push_back(0x11); d.candidates.push_back(0x60);
        break;
    case 0x6c: case 0x72:
        d.actions.push_back(0x5b); d.actions.push_back(0x5d);
        [[fallthrough]];
    case 0x2e: u[0] = 3; u[1] = 0x11; u[0x0d] = 2; header(v, 0); break;
    case 0x73:
        d.actions.push_back(0x56); d.actions.push_back(0x5d);
        u[1] = 4; u[0] = 3; u[0x0d] = 1; header(v, 0x0403); v[0x0d] = 1;
        break;
    case 0x6d:
        d.actions.push_back(0x56); d.actions.push_back(0x5d); d.actions.push_back(0x5b);
        u[1] = 2; u[0] = 3; u[0x0d] = 7; header(v, 0x0203); v[0x0d] = 7;
        break;
    case 0x51:
        d.actions.push_back(0x57);
        u[0] = 3; u[1] = 0x0c; u[0x0d] = 0x0b; header(v, 0x0c03); v[0x0d] = 0x0b;
        break;
    case 0x52: u[0] = 3; u[0x0d] = 1; header(v, 0x0003); v[0x0d] = 1; u[1] = 0; break;
    case 0x2f: case 0x6f: case 0x71: case 0x70: break;
    case 0x2d: case 0x2b:
        d.actions.push_back(0x5f); d.companion = 2;
        u[0] = 3; u[1] = uint8_t(draw() % 2 + 10); u[0x0d] = 2; header(v, 0);
        break;
    default: randomEquipment(d, draw); break;
    }
}

void numbers(Descriptor &d) {
    auto m = d.multipliers;
    switch (d.kind) {
    case 0x11: case 0x5e: case 0x61:
        m[0] = 400; m[2] = 1.5f; m[3] = 3; m[4] = 3; d.actions.push_back(0x5d); break;
    case 0x15:
        m[0] = 250; m[2] = 2; d.actions.push_back(0x45); m[4] = 3; m[3] = 3; break;
    case 0x19: m[0] = 200; m[2] = .5f; m[4] = 3; m[3] = 3; break;
    case 0x2e: case 0x52:
        m[0] = 250; m[3] = 3; m[4] = 3; d.actions.push_back(0x56); break;
    case 0x2f: case 0x58: {
        const float speed = m[1] * .5f;
        m[0] = 250; m[2] = 3; m[1] = speed; m[4] = 3; m[3] = 3;
        break;
    }
    case 0x51: m[0] = 250; m[3] = 3; m[4] = 3; d.actions.push_back(0x5c); break;
    case 0x55: m[0] = 1000; m[2] = 2; m[4] = 3; m[3] = 3; break;
    case 0x56: d.classification = 3; d.companion = 1; break;
    case 0x6b: case 0x6c: case 0x6d: case 0x6f: case 0x70: case 0x71:
    case 0x72: case 0x73: case 0x74: case 0x76:
        m[0] = 2500; m[2] = 6; m[3] = 5; m[4] = 5;
        m[1] = twoHanded(d.items[mainHand]) ? .75f : .5f;
        break;
    case 0x75: m[0] = 2500; m[2] = 5; m[3] = 4; m[4] = 5; break;
    case 0x77: m[1] = .5f; m[0] = 2500; m[2] = 6; m[3] = 10; m[4] = 10; break;
    default: break;
    }
}
}

void setup(Descriptor d, int32_t option, const std::function<int32_t()> &draw) {
    if (d.flags & 0x40)
        return;
    if (d.classification >= 0x80 && d.classification <= 0x89) {
        header(d.items[2], 4);
        d.items[2][0x0d] = 6;
        if (d.classification == 0x88)
            d.items[2][1] = uint8_t(draw() % 5 + 2);
        d.multipliers[2] = std::bit_cast<float>(uint32_t(0x3dcccccd));
        d.multipliers[1] = std::bit_cast<float>(uint32_t(0x3e99999a));
        d.multipliers[3] = 10;
        d.multipliers[4] = 10;
        return;
    }
    header(d.items[6], 0);
    d.items[7][0] = 0; d.items[7][1] = 0;
    for (size_t slot : {5u, 2u, 3u, 4u, 1u})
        header(d.items[slot], 0);
    hands(d, draw);
    level(d.items[7], d.level);
    level(d.items[6], d.level);
    if (uint8_t(option) == 0) {
        numbers(d);
        if (d.flags & 0x200) {
            auto m = d.multipliers;
            const float health = m[0] * 15.f;
            m[0] = health;
            if (health > 10000.f) m[0] = 10000.f;
            m[2] = m[2] * 3.f;
            m[1] = m[1] * .75f;
            m[3] = m[3] + 2.f;
            m[4] = m[4] + 2.f;
            constexpr std::array<int32_t, 5> actions{0x5c, 0x5d, 0x56, 0x59, 0x57};
            d.actions.push_back(actions[uint32_t(draw()) % actions.size()]);
            d.choice = draw() % 3;
            d.candidates.push_back(0x11); d.candidates.push_back(0x60);
        }
    }
    for (size_t slot : {6u, 7u, 5u, 2u, 4u, 3u})
        seed(d.items[slot], draw());
}

} // namespace creatures::equipment
