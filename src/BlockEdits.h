#pragma once
// Blocks changed while the game runs (docs/specs/alpha-block-edits.md): the 20-byte record the
// wire, the zone lists and the save share (§2), how a record changes a zone (§4), the 3-day
// expiry (§5), stamping a blueprint (§6), the natural ground a bomb or a big creature's blow
// takes away (§8, §9), and each zone's list of edits.
#include "CubModel.h"
#include "worldgen/Zone.h"
#include <array>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

// §2: one edit.
struct BlockEdit {
    int32_t x = 0, y = 0, z = 0;
    uint8_t r = 255, g = 255, b = 255;
    uint8_t typeByte = 0; // the whole type byte; 0 removes the block
    int32_t day = 0;      // the day it was made; negative never expires

    bool removes() const { return typeByte == 0; }
    std::array<uint8_t, 20> bytes() const;
    static BlockEdit parse(const uint8_t *bytes);
    bool operator==(const BlockEdit &o) const { return bytes() == o.bytes(); }
};

// §2: what a placement writes: type 6 (rock), protected.
constexpr uint8_t PLACED_BLOCK = worldgen::ROCK | worldgen::DRY_AIR;

// §5: a saved edit is applied again on a day no later than three after its own.
inline bool editKept(int32_t day, int32_t today) { return day < 0 || day >= today - 3; }

using GapFill = std::function<worldgen::Block(int32_t x, int32_t y, int32_t z)>;
using ReadBlock = std::function<worldgen::Block(int32_t x, int32_t y, int32_t z)>;

// §4.1: a record arriving while the zone is held. A removal touches only the column's stored
// blocks (sea water at z ≤ 0, air above); the keep flag holds.
void applyLive(worldgen::Zone &zone, const BlockEdit &edit, const GapFill &gapFill);
// §4.2: a saved record applied when its zone is generated again: the general writer for both
// kinds.
void applySaved(worldgen::Zone &zone, const BlockEdit &edit, const GapFill &gapFill);
// §4.1 step 1: deletes the plants and ground-resting sprites whose block below has gone.
// Returns whether any went.
bool dropFloating(worldgen::Zone &zone);

// §6.3: one block a placement writes, with or without a record.
struct BlueprintBlock {
    int32_t x = 0, y = 0, z = 0;
    worldgen::Block block;
    bool recorded = false; // interior air is written without a record
};
// §6.3 step 1: the block nearest the point position + aim (position in 1/65536 blocks).
std::array<int32_t, 3> placementBase(const int64_t position[3], glm::vec3 aim);
// §6.3: blueprint `model` stood on `base`, turned by `quarterTurns` · 90°, against the world as
// `read` gives it. Blocks the world protects (flag 0x40) are left alone.
std::vector<BlueprintBlock> placeBlueprint(const CubModel &model, std::array<int32_t, 3> base, int32_t quarterTurns,
                                        const ReadBlock &read);
// §6.2: the blueprint models by object number.
const char *blueprintKey(int32_t number); // nullptr for numbers with no blueprint

// Strike-cell-helpers §8: one cell a strike (K3) or a blast breaks: its removal record (dated,
// at the caller's record rounding), the unsnapped raw cell position (the burst's) and the
// tested cell's colour, read before removal.
struct BrokenCell {
    BlockEdit removal;
    std::array<int64_t, 3> at{};
    glm::u8vec3 colour{0};
};
// §5.3: the strike records cells by truncating division, the blast by floor-adjusted division.
enum class CellRounding { Truncate, FloorAdjusted };
// §8: the cells at `centre` (raw) plus integer offsets (x outermost, z innermost) with
// i² + j² + k² < radius², whose looked-up cell (floor-adjusted indices) has flag 0x20.
std::vector<BrokenCell> breakCells(const std::array<int64_t, 3> &centre, int32_t radius, CellRounding records,
                                   int32_t today, const ReadBlock &read);
std::vector<BlockEdit> removals(const std::vector<BrokenCell> &cells);
// A block position produced from raw coordinates, back to raw (exact for such values).
std::array<int64_t, 3> rawBlocks(glm::dvec3 blocks);
// Each held zone's list of edits, in the order they were made (§3), and which lists changed.
class BlockEditLists {
  public:
    using Key = std::pair<int32_t, int32_t>;
    // §3 step 3: appends a record to the list of the zone holding its column.
    void add(const BlockEdit &edit);
    // §4.2: a zone's saved list, filtered by the 3-day rule; the kept records.
    const std::vector<BlockEdit> &restore(int32_t zx, int32_t zy, const std::vector<std::array<uint8_t, 20>> &saved,
                                          int32_t today);
    void forget(int32_t zx, int32_t zy) { lists.erase({zx, zy}); }
    const std::vector<BlockEdit> *zone(int32_t zx, int32_t zy) const;
    std::vector<std::array<uint8_t, 20>> saved(int32_t zx, int32_t zy) const;
    std::set<Key> takeChanged() { return std::exchange(changed, {}); }

  private:
    std::map<Key, std::vector<BlockEdit>> lists;
    std::set<Key> changed;
};
