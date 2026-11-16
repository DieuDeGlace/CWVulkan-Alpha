#include "ui/Interaction.h"

namespace ui {

std::u16string staticPrompt(int32_t kind, uint8_t state, bool occupiedByPlayer) {
    switch (kind) {
    case DOOR:
    case BIG_DOOR:
    case WINDOW:
    case CHEST:
        return state != 0 ? u"[R] Open" : u"[R] Close";
    default:
        break;
    }
    const char16_t *text = nullptr;
    switch (kind) {
    case STOOL: case BENCH: text = u"[R] Sit Down"; break;
    case BED: case BEACH_TOWEL: case WOOD_MAT: text = u"[R] Sleep"; break;
    case RUNESTONE: text = u"[R] Teleport"; break;
    case FURNACE: text = u"[R] Use Furnace"; break;
    case ANVIL: text = u"[R] Use Anvil"; break;
    case SPINNING_WHEEL: text = u"[R] Use Spinning Wheel"; break;
    case LOOM: text = u"[R] Use Loom"; break;
    case SAWBENCH: text = u"[R] Use Saw"; break;
    case WORKBENCH: text = u"[R] Use Workbench"; break;
    case CUSTOMIZATION_BENCH: text = u"[R] Use Customization Bench"; break;
    default: return u"[R] Examine";
    }
    return occupiedByPlayer ? u"" : text;
}

bool rideable(int32_t type) {
    switch (type) {
    case 19: case 20: case 22: case 23: case 63: case 64: case 65: case 66: case 67: case 74: case 75:
    case 98: case 99: case 100: case 102: case 103: case 104: case 105: case 151:
        return true;
    default:
        return type >= 25 && type <= 40;
    }
}

std::u16string creaturePrompt(uint8_t controller, bool lookMode, int32_t type, bool ownedByPlayer, bool ridingSkill,
                              int32_t playerAction) {
    if (controller == 3 && lookMode)
        return u"[R] Talk";
    if (controller == 5 && rideable(type) && ridingSkill && ownedByPlayer && playerAction != 106)
        return u"[R] Ride";
    return u"";
}

void TargetPasses::creature(uint64_t id, double dp, double dc, bool underCrosshair) {
    if (dp < 16 && (Fd < 0 || dp < Fd))
        Fd = dp, fallbackCreature = id; // (the creature fall-back leaves the object one alone)
    if (underCrosshair && (B < 0 || dc < B))
        B = dc, found.creature = id;
}

void TargetPasses::staticObject(Triple at, double dp, double dc, const std::function<bool()> &aimed) {
    if (dp > 16)
        return;
    if (Fd < 0 || dp < Fd)
        Fd = dp, fallbackObject = at, fallbackCreature = 0;
    if ((B < 0 || dc < B) && aimed())
        B = dc, found.object = at, found.creature = 0;
}

void TargetPasses::item(Triple at, double q, double qc, bool carriable, const std::function<bool()> &aimed) {
    if (q > 16)
        return;
    if (!carriable)
        q += 16, qc += 16;
    // An item fall-back clears the creature and static fall-backs found so far.
    if (Fi < 0 || q < Fi)
        Fi = q, fallbackItem = at, fallbackObject.reset(), fallbackCreature = 0;
    if ((B < 0 || qc < B) && aimed())
        B = qc, found.item = at, found.object.reset(), found.creature = 0;
}

Targets TargetPasses::finish() const {
    Targets out = found;
    if (out.creature == 0 && !out.object) {
        if (fallbackCreature != 0)
            out.creature = fallbackCreature;
        if (fallbackObject)
            out.object = fallbackObject;
    }
    if (!out.item && fallbackItem)
        out.item = fallbackItem;
    return out;
}

uint64_t freePetClaim(std::span<const PetCandidate> candidates) {
    const PetCandidate *nearest = nullptr;
    for (const PetCandidate &c : candidates)
        if (c.controller == 5 && c.distance2 < 25 && (!nearest || c.distance2 < nearest->distance2))
            nearest = &c;
    return nearest && nearest->owner == ~uint64_t(0) ? nearest->id : 0;
}

} // namespace ui
