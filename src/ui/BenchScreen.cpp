// The Weapon Customization bench (docs/specs/alpha-item-services.md §3): the chosen weapon turning
// in a 3D view with its attached cubes, the cubes and spirits the bags hold, and placing, moving
// and removing cubes on the cells the mouse points at.
#include "CubModel.h"
#include "creatures/Spirits.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace ui {
namespace {

using namespace widgets;
using creatures::ItemRecord;

constexpr glm::vec2 PANEL{400, 625};
glm::vec2 origin(glm::vec2 W) { return {std::trunc(W.x / 2) - 610, std::trunc(W.y / 2) - 285}; }
glm::vec2 paletteCorner(int i) { return {20 + 45.f * (float)(i % 8), 40 + 45.f * (float)(i / 8)}; }

} // namespace

ItemRecord *Interface::benchItem() {
    if (!inventory)
        return nullptr;
    ItemRecord *item = nullptr;
    if (benchBox >= 0)
        item = &inventory->equipment[benchBox];
    else if (benchBag >= 0 && benchBag < (int)inventory->bags.size() && benchSlot < inventory->bags[benchBag].size() &&
             !inventory->bags[benchBag][benchSlot].empty())
        item = &inventory->bags[benchBag][benchSlot].item;
    return item && creatures::itemType(*item) == 3 ? item : nullptr;
}

void Interface::benchReset() {
    benchSelected = -1;
    benchHolding = false;
}

void Interface::benchRotate(glm::vec2 delta) {
    benchTurn.z += 0.5f * delta.x;
    benchTurn.x -= 0.5f * delta.y;
}

void Interface::drawBench(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 O = origin(W);
    if (benchPanel) {
        placeWidget(*benchPanel, 0, O);
        painter.draw(*benchPanel, 0, Transform(1));
    }
    outlined(painter, u"Weapon Customization", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    benchHovered = -1;
    benchTargetValid = false;
    ItemRecord *weapon = benchItem();
    if (!weapon || !itemModel || !itemVoxels)
        return;
    // §3.4: the palette.
    benchPalette = creatures::spiritPalette(*inventory, *weapon);
    if (benchSelected >= (int)benchPalette.size())
        benchSelected = (int)benchPalette.size() - 1;
    for (int i = 0; i < (int)benchPalette.size(); i++)
        if (inside(mouse - O, paletteCorner(i), glm::vec2(40)))
            benchHovered = i;

    // §3.5: the cell under the mouse, walked along the ray through the weapon's voxels.
    const ItemModel m = itemModel(*weapon);
    const CubModel *voxels = itemVoxels(*weapon);
    const glm::vec2 centre = O + glm::vec2(PANEL.x / 2, 0.6f * PANEL.y);
    const ItemDraw view = itemDraw(m.mesh, m.size, false, centre, benchTurn, 0.25f, 0, W);
    const glm::vec3 half(m.size.x / 2, m.size.y / 2, 0); // mesh coordinates -> cells
    const auto occupied = [&](glm::ivec3 c, bool skipHeld) {
        if (voxels && c.x >= 0 && c.y >= 0 && c.z >= 0 && c.x < voxels->sx && c.y < voxels->sy && c.z < voxels->sz &&
            voxels->solid(c.x, c.y, c.z))
            return true;
        for (int32_t k = 0; k < creatures::spiritCount(*weapon); k++) {
            const creatures::SpiritCube s = creatures::spiritAt(*weapon, k);
            if (s.cell == c && !(skipHeld && benchHolding && s.cell == benchHeld.cell))
                return true;
        }
        return false;
    };
    {
        const glm::vec2 ndc(2 * mouse.x / W.x - 1, 2 * mouse.y / W.y - 1);
        const glm::vec3 dirCamera((ndc.x - view.lens.z) / view.lens.x, (ndc.y - view.lens.w) / view.lens.y, 1);
        const glm::mat4 toModel = glm::inverse(view.camera);
        const glm::vec3 from = glm::vec3(toModel * glm::vec4(0, 0, 0, 1)) + half;
        const glm::vec3 dir = glm::normalize(glm::mat3(toModel) * dirCamera);
        const float largest = std::max({m.size.x, m.size.y, m.size.z, 1.f});
        glm::ivec3 previous(INT32_MIN);
        for (float t = 0; t < largest * 12; t += 0.1f) {
            const glm::vec3 p = from + dir * t;
            const glm::ivec3 cell(std::floor(p.x), std::floor(p.y), std::floor(p.z));
            if (cell == previous)
                continue;
            if (occupied(cell, true)) {
                benchTargetValid = true;
                const bool placing = benchHolding || (benchSelected >= 0 && benchHovered < 0);
                benchTarget = placing && previous.x != INT32_MIN ? previous : cell;
                benchHit = cell;
                break;
            }
            previous = cell;
        }
    }

    // §3.7: the weapon, its cubes, the ghost and the palette.
    painter.item(view);
    const auto cubeAt = [&](glm::ivec3 cell, glm::vec3 colour, float size) {
        glm::mat4 place = glm::translate(glm::mat4(1), glm::vec3(cell) - half + glm::vec3(0.5f, 0.5f, 0.5f - size / 2));
        place = glm::scale(place, glm::vec3(size));
        ItemDraw d = view;
        d.mesh = cubeMeshId;
        d.camera = view.camera * place;
        painter.item(tinted(d, colour));
    };
    for (int32_t k = 0; k < creatures::spiritCount(*weapon); k++) {
        const creatures::SpiritCube s = creatures::spiritAt(*weapon, k);
        if (benchHolding && s.cell == benchHeld.cell)
            continue;
        const bool hovered = benchTargetValid && !benchHolding && benchSelected < 0 && s.cell == benchTarget;
        cubeAt(s.cell, creatures::spiritColour(s.material), hovered ? 1.1f : 1.f);
    }
    if (benchHovered < 0 && benchTargetValid && (benchHolding || benchSelected >= 0)) {
        const uint8_t material = benchHolding ? benchHeld.material : creatures::itemMaterial(benchPalette[benchSelected].item);
        cubeAt(benchTarget, creatures::spiritColour(material), 1.f);
    }
    for (int i = 0; i < (int)benchPalette.size(); i++) {
        const glm::vec2 box = O + paletteCorner(i);
        const ItemModel pm = itemModel(benchPalette[i].item);
        const bool big = i == benchHovered || i == benchSelected;
        painter.item(itemDraw(pm.mesh, pm.size, false, box + glm::vec2(20), itemTurn, big ? 0.03f : 0.02f, 0, W));
        outlined(painter, number(benchPalette[i].count), box + glm::vec2(40, 40), 10, 2,
                 i == benchSelected ? glm::vec4(0, 1, 1, 1) : glm::vec4(1), plasma::RIGHT);
    }
    outlined(painter,
             u"Upgrades " + number(creatures::spiritCount(*weapon)) + u"/" + number(creatures::spiritCapacity(*weapon)),
             O + glm::vec2(20, PANEL.y - 20), 10, 2, glm::vec4(1));
    if (benchHovered >= 0)
        drawTooltip(painter, W, mouse + glm::vec2(20), benchPalette[benchHovered].item);
}

bool Interface::benchPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (!benchOpen)
        return false;
    if (button == 1)
        benchReset(); // §3.2: a right press drops the hold and the selection
    if (button != 0)
        return inside(mouse, origin(W), PANEL);
    ItemRecord *weapon = benchItem();
    if (!weapon)
        return inside(mouse, origin(W), PANEL);
    if (benchHovered >= 0) { // §3.6: toggle the palette selection
        benchSelected = benchSelected == benchHovered ? -1 : benchHovered;
        return true;
    }
    const bool overPanel = inside(mouse, origin(W), PANEL);
    if (!benchTargetValid) {
        if (benchHolding && !overPanel)
            creatures::removeSpirit(*weapon, benchHeld.cell); // dropped off the bench: lost
        benchReset();
        return overPanel;
    }
    if (benchHolding) { // move
        creatures::removeSpirit(*weapon, benchHeld.cell);
        creatures::placeSpirit(*weapon, benchTarget, benchHeld.material, benchHeld.level);
        benchHolding = false;
    } else if (benchSelected >= 0 && benchSelected < (int)benchPalette.size()) { // place
        if (creatures::spiritCount(*weapon) < 32) {
            const ItemRecord entry = benchPalette[benchSelected].item;
            creatures::placeSpirit(*weapon, benchTarget, creatures::itemMaterial(entry), creatures::itemLevel(entry));
            creatures::takeSpiritItem(*inventory, entry); // (consumed even when a full weapon takes nothing, as read)
            equipmentChanged = true;
        }
    } else { // pick up the cube at the cell
        for (int32_t k = 0; k < creatures::spiritCount(*weapon); k++) {
            const creatures::SpiritCube s = creatures::spiritAt(*weapon, k);
            if (s.cell == benchTarget) {
                benchHeld = s;
                benchHolding = true;
            }
        }
    }
    return true;
}

} // namespace ui
