// The Weapon Customization bench (docs/specs/alpha-item-services.md §3): the chosen weapon turning
// in a 3D view with its attached cubes, the cubes and spirits the bags hold, and placing, moving
// and removing cubes on the cells the mouse points at.
#include "CubModel.h"
#include "creatures/Spirits.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <bit>
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
    // §3.4: the cursor item is reset at the start of every draw (empty, level 1).
    benchCursor = {};
    creatures::setItemLevel(benchCursor, 1);
    const auto cursorCube = [&](uint8_t subtype, uint8_t material, int32_t level, uint8_t rarity) {
        benchCursor = {};
        benchCursor[0x00] = 11, benchCursor[0x01] = subtype, benchCursor[0x0c] = rarity, benchCursor[0x0d] = material;
        creatures::setItemLevel(benchCursor, std::bit_cast<int16_t>(uint16_t(level)));
    };
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
    // With no palette entry hovered: the held cube (subtype 14 for a spirit material, else 10),
    // else the selected entry.
    if (benchHovered < 0) {
        if (benchHolding)
            cursorCube(benchHeld.material >= 128 ? 14 : 10, benchHeld.material, benchHeld.level, 0);
        else if (benchSelected >= 0)
            benchCursor = benchPalette[benchSelected].item;
    }

    // §3.5: the cell under the mouse, walked along the ray through the weapon's voxels.
    const ItemModel m = itemModel(*weapon);
    const CubModel *voxels = itemVoxels(*weapon);
    const glm::vec2 centre = O + glm::vec2(PANEL.x / 2, 0.6f * PANEL.y);
    const ItemDraw view = itemDraw(m.mesh, m.size, false, centre, benchTurn, 0.25f, 0, W);
    const glm::vec3 half(m.size.x / 2, m.size.y / 2, 0); // mesh coordinates -> cells
    // One cell of the walk: a solid model voxel, or an attached cube (not the held one, whose
    // hit also makes the cursor item that cube as a rarity-2 spirit, whatever its material).
    const auto visit = [&](glm::ivec3 c) {
        bool hit = voxels && c.x >= 0 && c.y >= 0 && c.z >= 0 && c.x < voxels->sx && c.y < voxels->sy &&
                   c.z < voxels->sz && voxels->solid(c.x, c.y, c.z);
        for (int32_t k = 0; k < creatures::spiritCount(*weapon); k++) {
            const creatures::SpiritCube s = creatures::spiritAt(*weapon, k);
            if (s.cell == c && !(benchHolding && s.cell == benchHeld.cell)) {
                hit = true;
                cursorCube(14, s.material, s.level, 2);
            }
        }
        return hit;
    };
    {
        // §3.5: a cell-by-cell walk (3D DDA) while the ray parameter is below 1000 grid units.
        const glm::vec2 ndc(2 * mouse.x / W.x - 1, 2 * mouse.y / W.y - 1);
        const glm::vec3 dirCamera((ndc.x - view.lens.z) / view.lens.x, (ndc.y - view.lens.w) / view.lens.y, 1);
        const glm::mat4 toModel = glm::inverse(view.camera);
        const glm::vec3 from = glm::vec3(toModel * glm::vec4(0, 0, 0, 1)) + half;
        const glm::vec3 dir = glm::normalize(glm::mat3(toModel) * dirCamera);
        glm::ivec3 cell(std::floor(from.x), std::floor(from.y), std::floor(from.z)), step(0), back(0);
        glm::vec3 next(INFINITY), delta(INFINITY);
        for (int a = 0; a < 3; a++)
            if (dir[a] != 0) {
                step[a] = dir[a] > 0 ? 1 : -1;
                delta[a] = std::abs(1.f / dir[a]);
                next[a] = ((dir[a] > 0 ? (float)cell[a] + 1 : (float)cell[a]) - from[a]) / dir[a];
            }
        for (float t = 0; t < 1000;) {
            if (visit(cell)) {
                benchTargetValid = true;
                benchHit = cell;
                // Placing or moving targets the cell in front of the hit face.
                const bool placing = benchHovered < 0 && (benchHolding || benchSelected >= 0);
                benchTarget = placing ? cell + back : cell;
                break;
            }
            const int a = next.x <= next.y && next.x <= next.z ? 0 : next.y <= next.z ? 1 : 2;
            t = next[a];
            cell[a] += step[a];
            next[a] += delta[a];
            back = glm::ivec3(0);
            back[a] = -step[a];
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
        // §3.7 step 4: the cube at the valid target cell is drawn 1.1 x larger. (Its pulse,
        // alpha-only for spirits, is not modelled: the bench tint carries no alpha.)
        const bool targetedCube = benchTargetValid && s.cell == benchTarget;
        cubeAt(s.cell, creatures::spiritColour(s.material), targetedCube ? 1.1f : 1.f);
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
    // §3.5 cursor label: the cursor item's name, then " +" and its power for the types showing
    // one (spirits do, own-material cubes do not). (Placed as the map's cursor label.)
    if (creatures::itemType(benchCursor) != 0 && itemName) {
        std::u16string label = itemName(benchCursor);
        if (!creatures::quantityType(benchCursor) && creatures::itemType(benchCursor) != 19)
            label += u" +" + number(creatures::powerOf(creatures::itemLevel(benchCursor)));
        outlined(painter, label, mouse + glm::vec2(0, -20), 12, 3, glm::vec4(1), plasma::CENTRE);
    }
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
    // §3.6, first matching row: a valid selection places (a hold is left as it is), then a
    // hold moves, then a cube at the cell is picked up.
    if (benchSelected >= 0 && benchSelected < (int)benchPalette.size()) { // place
        if (creatures::spiritCount(*weapon) < 32) { // the row's fixed limit; capacity is the place rule's
            const ItemRecord entry = benchPalette[benchSelected].item;
            creatures::placeSpirit(*weapon, benchTarget, creatures::itemMaterial(entry), creatures::itemLevel(entry));
            creatures::takeSpiritItem(*inventory, entry); // (consumed even when a full weapon takes nothing, as read)
            equipmentChanged = true;
        }
    } else if (benchHolding) { // move
        creatures::removeSpirit(*weapon, benchHeld.cell);
        creatures::placeSpirit(*weapon, benchTarget, benchHeld.material, benchHeld.level);
        benchHolding = false;
        equipmentChanged = true;
    } else { // pick up the first cube at the cell (it stays on the weapon)
        for (int32_t k = 0; k < creatures::spiritCount(*weapon); k++) {
            const creatures::SpiritCube s = creatures::spiritAt(*weapon, k);
            if (s.cell == benchTarget) {
                benchHeld = s;
                benchHolding = true;
                break;
            }
        }
    }
    return true;
}

} // namespace ui
