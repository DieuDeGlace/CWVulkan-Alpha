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

} // namespace ui
