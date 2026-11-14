#include "BlockEdits.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using worldgen::Block;
using worldgen::Zone;

namespace {

int32_t floorDiv(int32_t a, int32_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

void store(Zone &zone, const BlockEdit &e, const GapFill &gapFill) {
    zone.place(e.x, e.y, e.z, Block{e.r, e.g, e.b, e.typeByte},
               [&](int32_t gz) { return gapFill ? gapFill(e.x, e.y, gz) : worldgen::IMPLICIT_SOLID; });
}

} // namespace

std::array<uint8_t, 20> BlockEdit::bytes() const {
    std::array<uint8_t, 20> out{};
    std::memcpy(out.data() + 0x00, &x, 4);
    std::memcpy(out.data() + 0x04, &y, 4);
    std::memcpy(out.data() + 0x08, &z, 4);
    out[0x0c] = r, out[0x0d] = g, out[0x0e] = b, out[0x0f] = typeByte;
    std::memcpy(out.data() + 0x10, &day, 4);
    return out;
}

BlockEdit BlockEdit::parse(const uint8_t *p) {
    BlockEdit e;
    std::memcpy(&e.x, p + 0x00, 4);
    std::memcpy(&e.y, p + 0x04, 4);
    std::memcpy(&e.z, p + 0x08, 4);
    e.r = p[0x0c], e.g = p[0x0d], e.b = p[0x0e], e.typeByte = p[0x0f];
    std::memcpy(&e.day, p + 0x10, 4);
    return e;
}

void applyLive(Zone &zone, const BlockEdit &e, const GapFill &gapFill) {
    if (!zone.contains(e.x, e.y))
        return;
    if (!e.removes()) {
        store(zone, e, gapFill);
        return;
    }
    const worldgen::Column &c = zone.column(e.x, e.y);
    if (e.z < c.bottom || e.z >= c.top())
        return; // only stored blocks can be removed
    // Zone::place turns air at z ≤ 0 into sea water and leaves a "keep" block alone.
    store(zone, BlockEdit{e.x, e.y, e.z, 255, 255, 255, worldgen::AIR, e.day}, gapFill);
}

void applySaved(Zone &zone, const BlockEdit &e, const GapFill &gapFill) {
    if (zone.contains(e.x, e.y))
        store(zone, e, gapFill);
}

bool dropFloating(Zone &zone) {
    const auto unsupported = [&](int32_t x, int32_t y, int32_t z) {
        return zone.contains(x, y) && zone.read(x, y, z - 1).type() == worldgen::AIR;
    };
    const size_t plants = zone.plants.size(), sprites = zone.sprites.size();
    std::erase_if(zone.plants, [&](const worldgen::Plant &p) { return unsupported(p.x, p.y, p.z); });
    std::erase_if(zone.sprites, [&](const worldgen::Sprite &s) {
        if (!(s.flags & 2))
            return false; // only those marked as resting on the ground
        const auto cell = [](int64_t v) { return (int32_t)std::floor((double)v / 65536.0); };
        return unsupported(cell(s.x), cell(s.y), cell(s.z));
    });
    return zone.plants.size() != plants || zone.sprites.size() != sprites;
}

std::array<int32_t, 3> placementBase(const int64_t position[3], glm::vec3 aim) {
    std::array<int32_t, 3> base{};
    for (int k = 0; k < 3; k++)
        base[k] = (int32_t)((position[k] + (int64_t)(aim[k] * 65536.0f) + 32768) / 65536);
    return base;
}

std::vector<BlueprintBlock> placeBlueprint(const CubModel &model, std::array<int32_t, 3> base, int32_t quarterTurns,
                                        const ReadBlock &read) {
    std::vector<BlueprintBlock> out;
    const int32_t q = ((quarterTurns % 4) + 4) % 4;
    for (int i = 0; i < model.sx; i++)
        for (int j = 0; j < model.sy; j++) {
            // The column's lowest and highest filled voxels.
            int zlo = model.sz, zhi = 0;
            for (int k = 0; k < model.sz; k++)
                if (model.solid(i, j, k)) {
                    zlo = std::min(zlo, k);
                    zhi = std::max(zhi, k);
                }
            // The voxel's centre about the model's middle, turned by quarter turns (exact for
            // the even sizes every blueprint has).
            const float u = (float)i + 0.5f - (float)model.sx / 2, v = (float)j + 0.5f - (float)model.sy / 2;
            const float tx[4] = {u, -v, -u, v}, ty[4] = {v, u, -v, -u};
            const int32_t X = base[0] + (int32_t)std::floor(tx[q]), Y = base[1] + (int32_t)std::floor(ty[q]);
            for (int k = 0; k < model.sz; k++) {
                const int32_t Z = base[2] + k;
                if (!model.solid(i, j, k)) {
                    if (zlo < k && k < zhi)
                        out.push_back({X, Y, Z, Block{0, 0, 0, worldgen::DRY_AIR}, false});
                    continue;
                }
                if (read(X, Y, Z).typeByte & worldgen::DRY_AIR)
                    continue; // structures and earlier placements win
                const glm::u8vec3 c = model.colour(i, j, k);
                out.push_back({X, Y, Z, Block{c.r, c.g, c.b, PLACED_BLOCK}, true});
            }
        }
    return out;
}

const char *blueprintKey(int32_t n) {
    static const char *const keys[] = {
        "framework-floor-wood.cub", "framework-wall-window.cub", "framework-wall-door.cub", "framework-roof1.cub",
        "framework-entrance.cub",   "framework-wall.cub",        "framework-roof2.cub",     "framework-roof3.cub",
        "framework-wall-balcony.cub", "framework-roof-wall2.cub", "building-stone-well.cub", "building-stone-stairs2.cub",
    };
    return n >= 0 && n < 12 ? keys[n] : nullptr;
}

namespace {

// Strike-cell-helpers §2.1: one is subtracted from negative raw values before the truncating
// division, so an exact negative multiple of 65536 lands one block below its floor.
int32_t floorAdjusted(int64_t raw) { return int32_t((raw < 0 ? raw - 65536 : raw) / 65536); }
int32_t truncated(int64_t raw) { return int32_t(raw / 65536); }

} // namespace

std::array<int64_t, 3> rawBlocks(glm::dvec3 blocks) {
    return {int64_t(blocks.x * 65536.0), int64_t(blocks.y * 65536.0), int64_t(blocks.z * 65536.0)};
}

std::vector<BrokenCell> breakCells(const std::array<int64_t, 3> &centre, int32_t radius, CellRounding records,
                                   int32_t today, const ReadBlock &read) {
    std::vector<BrokenCell> out;
    const auto recorded = records == CellRounding::Truncate ? truncated : floorAdjusted;
    for (int32_t i = -radius; i <= radius; i++)
        for (int32_t j = -radius; j <= radius; j++)
            for (int32_t k = -radius; k <= radius; k++) {
                if (!(float(radius * radius) > float(i * i + j * j + k * k)))
                    continue;
                const std::array<int64_t, 3> at{centre[0] + int64_t(i) * 65536, centre[1] + int64_t(j) * 65536,
                                                centre[2] + int64_t(k) * 65536};
                // §3: the test is flag 0x20 of the looked-up cell; read() supplies the
                // lookup's fallbacks (none carries the flag).
                const worldgen::Block cell = read(floorAdjusted(at[0]), floorAdjusted(at[1]), floorAdjusted(at[2]));
                if (!(cell.typeByte & worldgen::SURFACE_COLOURED))
                    continue;
                // §5: the record always carries the air value and the day.
                const BlockEdit removal{recorded(at[0]), recorded(at[1]), recorded(at[2]), 255, 255, 255, 0, today};
                out.push_back({removal, at, {cell.r, cell.g, cell.b}});
            }
    return out;
}

std::vector<BlockEdit> removals(const std::vector<BrokenCell> &cells) {
    std::vector<BlockEdit> out;
    out.reserve(cells.size());
    for (const BrokenCell &cell : cells)
        out.push_back(cell.removal);
    return out;
}

void BlockEditLists::add(const BlockEdit &e) {
    const Key key{floorDiv(e.x, Zone::SIZE), floorDiv(e.y, Zone::SIZE)};
    lists[key].push_back(e);
    changed.insert(key);
}

const std::vector<BlockEdit> &BlockEditLists::restore(int32_t zx, int32_t zy,
                                                      const std::vector<std::array<uint8_t, 20>> &saved,
                                                      int32_t today) {
    std::vector<BlockEdit> &list = lists[{zx, zy}];
    list.clear();
    for (const auto &bytes : saved) {
        const BlockEdit e = BlockEdit::parse(bytes.data());
        if (editKept(e.day, today))
            list.push_back(e);
    }
    if (list.size() != saved.size())
        changed.insert({zx, zy}); // dropped edits are gone from the next save
    return list;
}

const std::vector<BlockEdit> *BlockEditLists::zone(int32_t zx, int32_t zy) const {
    const auto it = lists.find({zx, zy});
    return it == lists.end() ? nullptr : &it->second;
}

std::vector<std::array<uint8_t, 20>> BlockEditLists::saved(int32_t zx, int32_t zy) const {
    std::vector<std::array<uint8_t, 20>> out;
    if (const auto *list = zone(zx, zy))
        for (const BlockEdit &e : *list)
            out.push_back(e.bytes());
    return out;
}
