#include "worldgen/Zone.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>

namespace worldgen {

namespace {
constexpr Block WATER_BLOCK{255, 255, 255, 0x82};
constexpr Block AIR_BLOCK{255, 255, 255, AIR};
} // namespace

void Zone::finishGeneratedItems() {
    using ItemKey = std::array<uint8_t, 3>;
    std::map<ItemKey, uint32_t> counts;
    const auto ground = [&](GroundItem &item) {
        ++counts[{item.category, item.subtype, item.material()}];
        if (item.originTag() & 1)
            return;
        while (clear(read(item.x, item.y, item.z - 1).type()))
            --item.z;
    };
    visitGeneratedItems(*this, ground, [&](const ItemDrop &drop) {
        ++counts[{drop.item.type, drop.item.subtype, drop.item.material}];
    });
    const int64_t centreX = (int64_t(zx) * SIZE + SIZE / 2) * 65536;
    const int64_t centreY = (int64_t(zy) * SIZE + SIZE / 2) * 65536;
    for (const auto &[key, count] : counts) {
        if ((key[0] == 1 || key[0] == 11) && std::bit_cast<int32_t>(count) >= 8) {
            ZoneMarker marker{9, centreX, centreY, 0};
            marker.itemKey = key;
            markers.push_back(marker);
        }
    }
}

AirshipTemplate airshipTemplate(int32_t zx, int32_t zy, int32_t index, int32_t rotation, int32_t top, int32_t centreX,
                                int32_t centreY) {
    constexpr double PI = 3.141592653589793; // binary64 0x400921FB54442D18
    const auto fixed = [](float blocks) { return int64_t(blocks * 65536.f); };
    AirshipTemplate t;
    // §2.1 identifier: K(a, b, c) = ((b << 16) + a) << 8 + c, wrapping in 64 bits.
    const uint64_t k = ((uint64_t(int64_t(zy)) << 16) + uint64_t(int64_t(zx))) << 8;
    t.id = std::bit_cast<int64_t>(k + uint64_t(int64_t(index)));
    // Steps 1-3: the heading, then the home 19 blocks out along a = (r + 2) pi / 2, 66 above.
    t.heading = float(int32_t(uint32_t(rotation) * 90u));
    const float a = float((double(rotation + 2) * PI) * 0.5);
    const float sine = float(std::sin(double(a))), cosine = float(std::cos(double(a)));
    const int32_t Y = int32_t(sine * 19.f + float(centreY));
    const int32_t X = int32_t(cosine * 19.f + float(centreX));
    const int32_t Z = top + 66;
    t.home = {int64_t(X) << 16, int64_t(Y) << 16, int64_t(Z) << 16};
    // Step 4: the target, 100 back along the heading and 100 up (cosine before sine).
    const float h = float((double(-t.heading) * PI) / 180.0);
    const float ch = float(std::cos(double(h))), sh = float(std::sin(double(h)));
    t.target = {t.home[0] + fixed(sh * -100.f), t.home[1] + fixed(ch * -100.f), t.home[2] + fixed(100.f)};
    // Steps 5-6.
    t.targetYaw = t.heading;
    t.destination = {t.home[0] + fixed(50.f), t.home[1] + fixed(50.f), t.home[2] + fixed(20.f)};
    return t;
}

float Zone::decorationLight(int64_t fixedX, int64_t fixedY, int64_t fixedZ) const {
    const int64_t x = fixedX / 65536, y = fixedY / 65536, z = fixedZ / 65536;
    if (uint64_t(x) >= 0x1000000 || uint64_t(y) >= 0x1000000 || !contains(int32_t(x), int32_t(y)))
        return 0;
    const auto &col = column(int32_t(x), int32_t(y));
    if (z < col.bottom)
        return 0;
    if (z - col.bottom >= int64_t(col.blocks.size()))
        return 255;
    const auto &block = col.blocks[size_t(z - col.bottom)];
    if (block.type() == GLOWING || (block.type() == AIR && z <= 0 && !(block.typeByte & DRY_AIR)))
        return 255;
    return clear(block.type()) ? float(std::max<uint8_t>(5, block.r)) : 0.f;
}

void Zone::initializeDecorationLight() {
    for (auto &p : plants)
        p.generationLight = decorationLight(int64_t(p.x) * 65536 + 32768, int64_t(p.y) * 65536 + 32768,
                                           int64_t(p.z) * 65536 - (p.sink ? 6553 : 0));
    for (auto &s : sprites)
        s.generationLight = decorationLight(s.x, s.y, s.z);
    for (auto &leaf : leafSources)
        leaf.generationLight = decorationLight(leaf.x, leaf.y, leaf.z);
    songGenerationLight.clear();
    songGenerationLight.reserve(songPosts.size());
    for (const auto &pos : songPosts)
        songGenerationLight.push_back({pos, decorationLight(pos[0], pos[1], pos[2])});
}

Block Zone::read(int32_t x, int32_t y, int32_t z) const {
    const Column &c = column(x, y);
    if (z < c.bottom)
        return IMPLICIT_SOLID;
    if (z >= c.top())
        return z > 0 ? AIR_BLOCK : WATER_BLOCK;
    const Block b = c.blocks[z - c.bottom];
    if (b.implicitWaterAt(z))
        return WATER_BLOCK;
    return b;
}

void Zone::place(int32_t x, int32_t y, int32_t z, Block b, const std::function<Block(int32_t)> &gapFill) {
    Column &c = column(x, y);
    if (b.typeByte == AIR && z >= c.top())
        return; // air above the top is already air
    if (z <= 0 && (b.typeByte & 0x5f) == 0)
        b = WATER_BLOCK;
    if (c.blocks.empty()) {
        c.bottom = z;
        c.blocks.push_back(b);
        return;
    }
    if (z < c.bottom) {
        // Below the old bottom: the new block, then the gap up to the old bottom filled.
        std::vector<Block> below;
        below.push_back(b);
        for (int32_t gz = z + 1; gz < c.bottom; gz++)
            below.push_back(gapFill ? gapFill(gz) : IMPLICIT_SOLID);
        c.blocks.insert(c.blocks.begin(), below.begin(), below.end());
        c.bottom = z;
        return;
    }
    if (z >= c.top()) // open points 9 §4: the upward extension zero-fills its new cells
        c.blocks.resize(z - c.bottom + 1, Block{});
    Block &slot = c.blocks[z - c.bottom];
    if (slot.typeByte & KEEP) { // terrain §1.3: a "keep" block only takes non-air, and stays kept
        if (b.type() != AIR) {
            slot = b;
            slot.typeByte |= KEEP;
        }
        return;
    }
    slot = b;
}

uint8_t Zone::light(int32_t x, int32_t y, int32_t z) const {
    const Column &c = column(x, y);
    if (z >= c.bottom && z < c.top()) {
        const Block &block = c.blocks[z - c.bottom];
        if (clear(block.type()))
            return block.r;
    }
    return 255;
}

} // namespace worldgen
