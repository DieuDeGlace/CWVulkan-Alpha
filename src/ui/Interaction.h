#pragma once
// What the crosshair offers to do (docs/specs/alpha-interaction.md): the prompts of §3 for
// the creature and the static the player is aiming at, and the bookkeeping of the target
// passes of §2.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace ui {

// §3, the static kinds that matter here (alpha-worldgen-structures.md §0 table B).
enum StaticKind : int32_t {
    DOOR = 1, BIG_DOOR = 2, WINDOW = 3, LEVER = 9, CHEST = 10, STOOL = 16, BENCH = 18, BED = 19,
    RUNESTONE = 45, CAMPFIRE = 65, BEACH_TOWEL = 68, WOOD_MAT = 69, FURNACE = 71, ANVIL = 72,
    SPINNING_WHEEL = 73, LOOM = 74, SAWBENCH = 75, WORKBENCH = 76, CUSTOMIZATION_BENCH = 77,
};

// §3 step 3: the prompt for a static of `kind` in `state` (+0x30: 1 closed, 0 open, 2 a
// closed chest with loot); `occupiedByPlayer` hides the seat, bed and station prompts (the
// empty string: no assignment, an earlier prompt stays).
std::u16string staticPrompt(int32_t kind, uint8_t state, bool occupiedByPlayer);

// §3 step 2: 0x444760, the creature types a player can ride.
bool rideable(int32_t type);

// §3 step 2: "[R] Talk" for controller 3 in look mode; else "[R] Ride" for a controller-5
// creature of a rideable type whose owner (+0x198) is the player, with the player's riding
// field (+0x113C) set and its action not 106; else empty, and the object branch follows.
std::u16string creaturePrompt(uint8_t controller, bool lookMode, int32_t type, bool ownedByPlayer, bool ridingSkill,
                              int32_t playerAction);

// §1: a world item or static as (zone x, zone y, index); "none" is an empty optional.
using Triple = std::array<int32_t, 3>;
struct Targets {
    uint64_t creature = 0;       // G+0x800A70
    std::optional<Triple> item;  // G+0x800A78
    std::optional<Triple> object; // G+0x800A84
};

// §2.1 … §2.5: one frame's target passes. Every distance given is squared (blocks²), the
// player distance dp/q from the player's position, the camera distance dc/qc from the eye.
// Creatures and statics share the fall-back Fd; all passes share the camera distance B.
class TargetPasses {
  public:
    // §2.2 steps 4-5, for a listed candidate past the skip rule and its line of sight. The
    // fall-back (dp < 16) is taken before the box result.
    void creature(uint64_t id, double dp, double dc, bool underCrosshair);
    // §2.1 step 3: an open dialogue bubble names its creature (later passes may replace it).
    void bubbleCreature(uint64_t id) { found.creature = id; }
    // §2.3 steps 1-4: skipped beyond dp 16; `aimed` (the line of sight and the box test) is
    // asked only when dc could win.
    void staticObject(Triple at, double dp, double dc, const std::function<bool()> &aimed);
    // §2.1 step 6: the player is dead.
    void dead() { found.creature = 0, found.object.reset(); }
    // §2.4 steps 2-6, for an item the player may pick up (step 1 is the caller's): skipped
    // beyond q 16; one it cannot carry counts 16 further on both distances.
    void item(Triple at, double q, double qc, bool carriable, const std::function<bool()> &aimed);
    // §2.5: the targets after the fall-backs (applied whatever the player's health).
    Targets finish() const;

  private:
    Targets found;
    double B = -1, Fd = -1, Fi = -1;
    uint64_t fallbackCreature = 0;
    std::optional<Triple> fallbackObject, fallbackItem;
};

// §5.2 a): with no creature target, the controller-5 creature nearest the player within
// squared distance < 25 (only a strictly nearer one replaces, in visiting order) is claimed
// when its owner field (+0x198) is all ones. Returns the claimed id, or 0.
struct PetCandidate {
    uint64_t id = 0;
    uint8_t controller = 0;
    double distance2 = 0;
    uint64_t owner = 0;
};
uint64_t freePetClaim(std::span<const PetCandidate> candidates);

} // namespace ui
