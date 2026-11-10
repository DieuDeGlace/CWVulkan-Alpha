#include "creatures/Inventory.h"
#include <cmath>
#include <limits>

namespace creatures {
namespace {
float scale(float level, uint32_t rarity) {
    const float shifted = level - 1.f;
    const float scaled = shifted * 0.05f;
    const float denominator = scaled + 1.f;
    const float reciprocal = 1.f / denominator;
    const float growth = 1.f - reciprocal;
    const float exponent = growth * 3.f;
    const float levelFactor = float(std::pow(2.0, double(exponent)));
    const float rankExponent = float(rarity) * 0.25f;
    const float rankFactor = float(std::pow(2.0, double(rankExponent)));
    return levelFactor * rankFactor;
}

int32_t truncate(float value) {
    // CVTTSS2SI's integer-indefinite result; no saturation or second minimum.
    if (!(value >= -2147483648.f && value < 2147483648.f)) return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
} // namespace

int32_t price(const ItemRecord &item, const std::function<float(float, uint32_t)> &scalar) {
    float multiplier = 0.2f;
    switch (itemType(item)) {
    case 3: case 4: multiplier = 2.f; break;
    case 5: case 6: case 9: multiplier = 1.f; break;
    case 7: case 8: multiplier = 1.5f; break;
    case 23: multiplier = 100.f; break;
    default: break;
    }
    if (twoHanded(item)) multiplier *= 2.f;
    const auto evaluate = [&] {
        const float factor = scalar(float(itemLevel(item)), uint32_t(itemRarity(item)));
        const float tenfold = factor * 10.f;
        return truncate(tenfold * multiplier);
    };
    const int32_t first = evaluate();
    return first < 1 ? 1 : evaluate();
}

int32_t price(const ItemRecord &item) { return price(item, scale); }
} // namespace creatures
