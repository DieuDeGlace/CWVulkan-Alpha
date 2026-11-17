#include "BlockEdits.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <optional>

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
    for (int k = 0; k < 3; k++) {
        const int64_t offset = int64_t(aim[k] * 65536.f);
        const int64_t raw = std::bit_cast<int64_t>(uint64_t(position[k]) + uint64_t(offset) + 32768u);
        base[k] = std::bit_cast<int32_t>(uint32_t(raw / 65536));
    }
    return base;
}

namespace {

template<class Visit>
void visitBlueprint(const CubModel &model, const std::array<int32_t, 3> &base, int32_t rotation,
                    const ReadBlock &read, const Visit &write) {
    const float angle = (float(rotation) * 90.f) * 0.0174532924f;
    const float cosine = float(std::cos(double(angle))), sine = float(std::sin(double(angle)));
    const float x0 = -.5f * float(model.sx), y0 = -.5f * float(model.sy);
    const float tx = cosine * x0 - sine * y0, ty = sine * x0 + cosine * y0;
    const auto column = [](int32_t anchor, float offset) {
        return std::bit_cast<int32_t>(uint32_t((int64_t(anchor) * 65536 + int64_t(offset * 65536.f)) / 65536));
    };
    for (int i = 0; i < model.sx; i++)
        for (int j = 0; j < model.sy; j++) {
            // The column's lowest and highest filled voxels.
            int zlo = model.sz, zhi = 0;
            for (int k = 0; k < model.sz; k++)
                if (model.solid(i, j, k)) {
                    zlo = std::min(zlo, k);
                    zhi = std::max(zhi, k);
                }
            const float x = float(i) + .5f, y = float(j) + .5f;
            const int32_t X = column(base[0], (cosine * x - sine * y) + tx);
            const int32_t Y = column(base[1], (sine * x + cosine * y) + ty);
            for (int k = 0; k < model.sz; k++) {
                const int32_t Z = column(base[2], float(k) + .5f);
                if (!model.solid(i, j, k)) {
                    if (zlo < k && k < zhi)
                        write(BlueprintBlock{X, Y, Z, Block{0, 0, 0, worldgen::DRY_AIR}, false});
                    continue;
                }
                if (read(X, Y, Z).typeByte & worldgen::DRY_AIR)
                    continue; // structures and earlier placements win
                const glm::u8vec3 c = model.colour(i, j, k);
                write(BlueprintBlock{X, Y, Z, Block{c.r, c.g, c.b, PLACED_BLOCK}, true});
            }
        }
}

} // namespace

std::vector<BlueprintBlock> placeBlueprint(const CubModel &model, std::array<int32_t, 3> base, int32_t quarterTurns,
                                          const ReadBlock &read) {
    std::vector<BlueprintBlock> out;
    visitBlueprint(model, base, quarterTurns, read, [&](const auto &cell) { out.push_back(cell); });
    return out;
}

BlueprintPlacement stageBlueprint(const worldgen::WorldMap &world, const CubModel &model,
                                  const std::array<int32_t, 3> &base, int8_t rotation, int32_t day,
                                  const GapFill &gapFill) {
    using Key = worldgen::WorldMap::ZoneKey;
    constexpr int32_t lastZone = 65535;
    constexpr uint32_t worldWidth = (lastZone + 1u) * Zone::SIZE;
    BlueprintPlacement placement;
    const auto copyZone = [&](Key key, const Zone &source) -> Zone & {
        auto &copy = placement.zones[key];
        if (!copy)
            copy = std::make_shared<Zone>(source);
        return *copy;
    };
    const ReadBlock read = [&](int32_t x, int32_t y, int32_t z) {
        if (uint32_t(x) >= worldWidth || uint32_t(y) >= worldWidth)
            return worldgen::IMPLICIT_SOLID;
        const Key key{x / Zone::SIZE, y / Zone::SIZE};
        if (const auto found = placement.zones.find(key); found != placement.zones.end())
            return found->second->read(x, y, z);
        const auto *zone = world.zone(key.first, key.second);
        return zone ? zone->read(x, y, z) : worldgen::IMPLICIT_SOLID;
    };
    visitBlueprint(model, base, rotation, read, [&](const BlueprintBlock &cell) {
        const auto &b = cell.block;
        const BlockEdit edit{cell.x, cell.y, cell.z, b.r, b.g, b.b, b.typeByte, day};
        if (uint32_t(cell.x) < worldWidth && uint32_t(cell.y) < worldWidth) {
            const Key key{cell.x / Zone::SIZE, cell.y / Zone::SIZE};
            if (const auto *zone = world.zone(key.first, key.second)) {
                applySaved(copyZone(key, *zone), edit, gapFill);
                placement.changedColumns.emplace(cell.x, cell.y);
            }
        }
        if (cell.recorded)
            placement.records.push_back(edit);
    });

    const auto zoneOf = [](int64_t block) { return int32_t(std::max(block, int64_t(0)) / Zone::SIZE); };
    const auto block = [](int64_t raw) { return std::bit_cast<int32_t>(uint32_t(raw / 65536 - (raw < 0 ? 1 : 0))); };
    const auto support = [&](int64_t x, int64_t y, int64_t z) {
        const int64_t below = std::bit_cast<int64_t>(uint64_t(z) - 65536u);
        return std::array{block(x), block(y), block(below)};
    };
    const int32_t lastX = std::min(lastZone, zoneOf(int64_t(base[0]) + model.sx));
    const int32_t lastY = std::min(lastZone, zoneOf(int64_t(base[1]) + model.sy));
    for (int32_t x = zoneOf(base[0]); x <= lastX; x++)
        for (int32_t y = zoneOf(base[1]); y <= lastY; y++) {
            const Key key{x, y};
            const auto *zone = world.zone(x, y);
            if (!zone)
                continue;
            const auto prune = [&](auto entries, const auto &supportCell) {
                const auto &original = zone->*entries;
                const auto discard = [&](const auto &entry) {
                    if (!(entry.flags & 2))
                        return false;
                    const auto [sx, sy, sz] = supportCell(entry);
                    if (read(sx, sy, sz).type() != worldgen::AIR)
                        return false;
                    placement.changedColumns.emplace(sx, sy);
                    return true;
                };
                const auto first = std::find_if(original.begin(), original.end(), discard);
                if (first == original.end())
                    return;
                auto &current = copyZone(key, *zone).*entries;
                const auto start = current.begin() + (first - original.begin());
                auto out = start;
                for (auto entry = start + 1; entry != current.end(); ++entry)
                    if (!discard(*entry))
                        *out++ = std::move(*entry);
                current.erase(out, current.end());
            };
            // The original zone keeps both terrain plants and prefab sprites in this list.
            prune(&Zone::plants, [&](const worldgen::Plant &plant) {
                return support(int64_t(plant.x) * 65536 + 32768, int64_t(plant.y) * 65536 + 32768,
                               int64_t(plant.z) * 65536 - (plant.sink ? 6553 : 0));
            });
            prune(&Zone::sprites, [&](const worldgen::Sprite &sprite) {
                return support(sprite.x, sprite.y, sprite.z);
            });
        }
    return placement;
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
    if (radius <= 0)
        return out;
    const auto recorded = records == CellRounding::Truncate ? truncated : floorAdjusted;
    const float radiusSquared = float(radius * radius);
    constexpr uint32_t worldWidth = 1u << 24;
    for (int32_t i = -radius; i <= radius; i++) {
        const int64_t rawX = centre[0] + int64_t(i) * 65536;
        const int32_t x = floorAdjusted(rawX);
        if (uint32_t(x) >= worldWidth)
            continue;
        for (int32_t j = -radius; j <= radius; j++) {
            const int64_t rawY = centre[1] + int64_t(j) * 65536;
            const int32_t y = floorAdjusted(rawY);
            if (uint32_t(y) >= worldWidth)
                continue;
            const int32_t horizontalSquared = i * i + j * j;
            std::optional<int32_t> waterAbove;
            for (int32_t k = -radius; k <= radius; k++) {
                if (!(radiusSquared > float(horizontalSquared + k * k)))
                    continue;
                const int64_t rawZ = centre[2] + int64_t(k) * 65536;
                const int32_t z = floorAdjusted(rawZ);
                if (waterAbove == z)
                    continue;
                // §3: the test is flag 0x20 of the looked-up cell; read() supplies the
                // lookup's fallbacks (none carries the flag).
                const worldgen::Block cell = read(x, y, z);
                if (!(cell.typeByte & worldgen::SURFACE_COLOURED))
                    continue;
                const int32_t removalZ = recorded(rawZ);
                // A negative strike coordinate can clear the cell above the one it tested.
                // Preserve that water write for the next probe in this column's ordered walk.
                if (removalZ > z && removalZ <= 0)
                    waterAbove = removalZ;
                // §5: the record always carries the air value and the day.
                const BlockEdit removal{recorded(rawX), recorded(rawY), removalZ, 255, 255, 255, 0, today};
                out.push_back({removal, {rawX, rawY, rawZ}, {cell.r, cell.g, cell.b}});
            }
        }
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
    add({floorDiv(e.x, Zone::SIZE), floorDiv(e.y, Zone::SIZE)}, e);
}

void BlockEditLists::add(Key key, const BlockEdit &e) {
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
