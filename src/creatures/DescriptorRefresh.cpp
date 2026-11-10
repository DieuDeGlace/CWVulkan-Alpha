#include "creatures/DescriptorRefresh.h"
#include <algorithm>
#include <array>
#include <bit>
#include <immintrin.h>

namespace creatures::activation {
namespace {
int16_t lowWord(int32_t value) { return std::bit_cast<int16_t>(uint16_t(value)); }

void adjustLevel(int16_t &level, const std::function<int32_t()> &draw) {
    const int32_t original = level;
    if (original < 2)
        return;
    const float offset = float(original) - 1.f;
    const float scaled = offset * .05f;
    const float denominator = scaled + 1.f;
    const float reciprocal = 1.f / denominator;
    const float curve = 1.f - reciprocal;
    const float reduced = curve - .05f;
    if (reduced <= 0) {
        level = 1;
        return;
    }
    const float remaining = 1.f - reduced;
    const float inverse = 1.f / remaining;
    const float shifted = inverse - 1.f;
    const float spread = shifted * 20.f;
    const float lowerValue = spread + 1.f;
    const int32_t lower = std::max(1, _mm_cvttss_si32(_mm_set_ss(lowerValue)));
    level = lowWord(lower + draw() % (original - lower + 1));
}

bool acceptedItem(uint8_t type, uint8_t subtype) {
    switch (type) {
    case 0: case 12: case 13: case 20: case 21: case 23: case 24: case 25:
        return false;
    case 11:
        return subtype == 14;
    default:
        return true;
    }
}
} // namespace

std::optional<std::pair<int32_t, int32_t>> contextCoordinates(int32_t x, int32_t y) {
    const int32_t cy = y / 8;
    const int32_t cx = x / 8;
    if (cx < 0 || cy < 0 || cx >= 8192 || cy >= 8192)
        return std::nullopt;
    return std::pair{cx, cy};
}

bool inWindow(int32_t lower, int32_t upper, const std::function<int32_t()> &readClock) {
    if (lower == upper)
        return true;
    if (readClock() < lower)
        return false;
    return readClock() <= upper;
}

bool inRange(std::span<const int64_t, 3> actor, std::span<const int64_t, 3> descriptor, float radius) {
    std::array<float, 3> difference;
    for (size_t axis = 0; axis < difference.size(); axis++) {
        const int64_t raw = std::bit_cast<int64_t>(uint64_t(actor[axis]) - uint64_t(descriptor[axis]));
        difference[axis] = float(raw) * (1.f / 65536.f);
    }
    const float x2 = difference[0] * difference[0];
    const float y2 = difference[1] * difference[1];
    const float z2 = difference[2] * difference[2];
    const float horizontal = x2 + y2;
    const float distance = horizontal + z2;
    const float limit = radius * radius;
    return limit > distance;
}

bool refresh(Descriptor descriptor, const Context *context, const Services &services) {
    if (!context || context->selector != 5 || !(descriptor.flags & 0x1000))
        return false;
    if (descriptor.role != 6) {
        descriptor.level = context->level;
        descriptor.tier = context->tier;
    }
    services.seed(context->seed + uint32_t(descriptor.identifier));
    services.setup(1);
    if (descriptor.flags & 0x0200) {
        if (descriptor.marker != 0)
            descriptor.flags |= 0x2000;
    } else {
        for (const size_t slot : {7u, 6u, 5u, 2u, 4u, 3u, 1u, 8u, 9u})
            adjustLevel(services.equipmentLevel(slot), services.draw);
    }
    services.inventory([&](InventoryEntry entry) {
        if (entry.count == 0 || !acceptedItem(entry.type, entry.subtype))
            return;
        entry.level = lowWord(context->level);
        if (entry.type < 3 || entry.type > 9)
            return;
        entry.rarity = uint8_t(context->tier + 1u);
        if (services.draw() % 20 == 0)
            entry.rarity = uint8_t(entry.rarity + 1u);
        if (services.draw() % 100 == 0)
            entry.rarity = uint8_t(entry.rarity + 1u);
        if (entry.rarity > 4)
            entry.rarity = 4;
    });
    return true;
}

} // namespace creatures::activation
