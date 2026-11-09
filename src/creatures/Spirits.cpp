#include "creatures/Spirits.h"
#include <algorithm>
#include <cstring>

namespace creatures {

namespace {

constexpr size_t LIST = 0x14, COUNT = 0x114, ENTRY = 8;

void setCount(ItemRecord &item, int32_t n) { std::memcpy(item.data() + COUNT, &n, 4); }

void write(ItemRecord &item, int32_t k, const SpiritCube &s) {
    uint8_t *e = item.data() + LIST + ENTRY * (size_t)k;
    e[0] = (uint8_t)(int8_t)s.cell.x;
    e[1] = (uint8_t)(int8_t)s.cell.y;
    e[2] = (uint8_t)(int8_t)s.cell.z;
    e[3] = s.material;
    std::memcpy(e + 4, &s.level, 4);
}

} // namespace

int32_t spiritCount(const ItemRecord &item) {
    int32_t n;
    std::memcpy(&n, item.data() + COUNT, 4);
    return std::clamp(n, 0, 32);
}

SpiritCube spiritAt(const ItemRecord &item, int32_t k) {
    const uint8_t *e = item.data() + LIST + ENTRY * (size_t)k;
    SpiritCube s;
    s.cell = {(int8_t)e[0], (int8_t)e[1], (int8_t)e[2]};
    s.material = e[3];
    std::memcpy(&s.level, e + 4, 4);
    return s;
}

int32_t spiritCapacity(const ItemRecord &item) {
    switch (itemSubtype(item)) {
    case 5: case 6: case 7: case 8: case 10: case 11: case 15: case 16: case 17: case 18: return 32;
    default: return 16;
    }
}

void placeSpirit(ItemRecord &item, glm::ivec3 cell, uint8_t material, int32_t level) {
    const int32_t n = spiritCount(item);
    for (int32_t k = 0; k < n; k++)
        if (spiritAt(item, k).cell == cell) {
            write(item, k, {cell, material, level});
            return;
        }
    if (n < spiritCapacity(item)) {
        write(item, n, {cell, material, level});
        setCount(item, n + 1);
    }
}

void removeSpirit(ItemRecord &item, glm::ivec3 cell) {
    const int32_t n = spiritCount(item);
    for (int32_t k = 0; k < n; k++)
        if (spiritAt(item, k).cell == cell) {
            for (int32_t j = k; j + 1 < n; j++)
                write(item, j, spiritAt(item, j + 1));
            setCount(item, n - 1);
            return;
        }
}

int32_t levelOfPower(int32_t p) { return (int32_t)((1.f / (1.f - 0.01f * (float)p) - 1.f) * 20.f + 1.f); }

std::vector<Stack> spiritPalette(const Inventory &inv, const ItemRecord &weapon) {
    std::vector<Stack> out;
    // The weapon's own material's cubes, exactly equal to a plain level-1 cube.
    ItemRecord cube{};
    cube[0x00] = 11, cube[0x01] = 10, cube[0x0d] = itemMaterial(weapon);
    setItemLevel(cube, 1);
    int32_t n = 0;
    for (const auto &bag : inv.bags)
        for (const Stack &s : bag)
            if (!s.empty() && sameItem(s.item, cube))
                n += s.count;
    if (n > 0)
        out.push_back({n, cube});
    // Spirits from 10 below the weapon's power up to it, each power by material.
    const int32_t pw = powerOf(itemLevel(weapon));
    for (int32_t p = std::max(1, pw - 10); p <= pw; p++)
        for (uint8_t m = 128; m <= 131; m++) {
            int32_t count = 0;
            for (const auto &bag : inv.bags)
                for (const Stack &s : bag)
                    if (!s.empty() && itemType(s.item) == 11 && itemSubtype(s.item) == 14 && itemMaterial(s.item) == m &&
                        powerOf(itemLevel(s.item)) == p)
                        count += s.count;
            if (count > 0) {
                ItemRecord spirit{};
                spirit[0x00] = 11, spirit[0x01] = 14, spirit[0x0d] = m;
                setItemLevel(spirit, (int16_t)levelOfPower(p));
                out.push_back({count, spirit});
            }
        }
    return out;
}

void takeSpiritItem(Inventory &inv, const ItemRecord &entry) {
    for (auto &bag : inv.bags)
        for (Stack &s : bag)
            if (!s.empty() && itemType(s.item) == itemType(entry) && itemSubtype(s.item) == itemSubtype(entry) &&
                itemMaterial(s.item) == itemMaterial(entry) &&
                powerOf(itemLevel(s.item)) == powerOf(itemLevel(entry))) {
                if (--s.count < 1)
                    s.count = 0, s.item[0x00] = s.item[0x01] = 0;
                return;
            }
}

bool averageSpiritColour(const Item &item, glm::vec3 &colour) {
    glm::vec3 sum(0);
    int n = 0;
    // Valid descriptors keep the signed count and decoded vector in sync.
    // Bound malformed counts by owned storage rather than reading beyond it.
    const size_t count = item.spirits > 0 ? std::min(size_t(item.spirits), item.cubes.size()) : 0;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t material = item.cubes[i].material;
        if (material >= 128) {
            sum += material <= 131 ? 1.5f * spiritColour(material) : glm::vec3(.5f);
            ++n;
        }
    }
    colour = n ? sum * (1.f / float(n)) : glm::vec3(0);
    return n > 0;
}

glm::vec3 spiritColour(uint8_t material) {
    switch (material) {
    case 128: return {1, 0.5f, 0.1f};
    case 129: return {0.3f, 1, 0.5f};
    case 130: return {0.3f, 0.5f, 1};
    case 131: return {0.8f, 0.8f, 1};
    // Materials §1.1: ordinary materials scale the (white) cube; six have their own colour.
    case 1: return {0.7f, 0.7f, 0.7f};   // iron
    case 2: return {0.4f, 0.3f, 0.2f};   // wood
    case 5: return {0.1f, 0.1f, 0.1f};   // obsidian
    case 7: return {0.9f, 0.9f, 0.9f};   // bone
    case 11: return {1.0f, 0.7f, 0.2f};  // gold
    case 12: return {0.8f, 0.8f, 0.85f}; // silver
    default: return {0.5f, 0.5f, 0.5f};
    }
}

} // namespace creatures
