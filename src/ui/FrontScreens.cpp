// The screens between the start menu and a world (docs/specs/alpha-character-screens.md):
// the character list, character creation with its style panel, the world list and world
// creation. The game reads and writes the saves; these screens show the lists, edit the
// choices and ask for what the buttons mean (`frontRequest`).
#include "NewCharacter.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace {

using namespace widgets;

constexpr glm::vec2 PANEL{600, 220}, STYLE{300, 330}, EDIT{125, 20};
const glm::vec4 CYAN{0, 1, 1, 1};

const char16_t *const RACES[8] = {u"Human", u"Elf", u"Dwarf", u"Orc", u"Goblin", u"Lizard", u"Undead", u"Frogman"};
const char16_t *const CLASS_NAMES[5] = {u"", u"Warrior", u"Ranger", u"Mage", u"Rogue"};
const char16_t *const SPECIALIZATIONS[5][2] = {{u"", u""},
                                              {u"Berserker", u"Guardian"},
                                              {u"Sniper", u"Scout"},
                                              {u"Fire Mage", u"Water Mage"},
                                              {u"Assassin", u"Ninja"}};

// §3.2: the race word of a creature type (frogmen have none).
const char16_t *raceWord(int32_t type) {
    switch (type) {
    case 0: case 1: return u"Elf";
    case 2: case 3: return u"Human";
    case 4: case 5: return u"Goblin";
    case 7: case 8: return u"Lizardman";
    case 9: case 10: return u"Dwarf";
    case 11: case 12: return u"Orc";
    case 15: case 16: return u"Undead";
    default: return u"";
    }
}

// §3.9: the hair palette, 16 columns of 7 shades.
glm::vec3 swatch(int c, int r) {
    static const glm::vec3 BASE[16] = {{1, 0, 0},     {1, 0.25f, 0}, {1, 0.5f, 0},  {1, 1, 0},     {0.5f, 1, 0},
                                       {0, 1, 0},     {0, 1, 0.5f},  {0, 1, 1},     {0, 0.5f, 1},  {0, 0.25f, 1},
                                       {0, 0, 1},     {0.25f, 0, 1}, {0.5f, 0, 1},  {0.75f, 0, 1}, {1, 0, 1},
                                       {0.5f, 0.5f, 0.5f}};
    const glm::vec3 b = BASE[c];
    if (r < 3)
        return b * (0.2f + 0.25f * (float)r);
    if (r == 3)
        return b;
    return b + (glm::vec3(1) - b) * (0.25f * (float)(r - 3));
}

// §2.2: the fixed buttons.
struct Button {
    glm::vec2 at, size;
};
Button backButton() { return {{20, 20}, {200, 40}}; }
Button createButton(glm::vec2 W) { return {{W.x - 320, W.y - 60}, {300, 40}}; }
Button multiplayerButton(glm::vec2 W) { return {{W.x - 320, 20}, {300, 40}}; }
Button connectToServerButton(glm::vec2 W) { return {{W.x - 320, W.y - 60}, {300, 40}}; }
Button connectButton(glm::vec2 W) { return {{std::trunc((W.x - 200) / 2), std::trunc((W.y - 40) / 2)}, {200, 40}}; }

} // namespace

void Interface::loadFront() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        listPanel = clone(gui, n);
        resizeWidget(*listPanel, 0, PANEL);
        stylePanel = clone(gui, n);
        resizeWidget(*stylePanel, 0, STYLE);
    }
    if (const int32_t n = gui.find("button"); n >= 0)
        bigButton = clone(gui, n);
    if (const int32_t n = gui.find("edit"); n >= 0)
        editBox = clone(gui, n);
    // The templates' own placeholder words go; captions and typed text are drawn in code.
    for (auto *doc : {&bigButton, &editBox})
        if (*doc)
            for (plasma::Shape &shape : (*doc)->shapes)
                if (shape.kind == plasma::Shape::Kind::Text)
                    shape.text.text.value.clear();
}

void Interface::drawButton(Painter &painter, glm::vec2 at, glm::vec2 size, const std::u16string &caption, glm::vec2 mouse) {
    if (bigButton) {
        resizeWidget(*bigButton, 0, size);
        placeWidget(*bigButton, 0, at);
        painter.draw(*bigButton, 0, Transform(1));
    }
    outlined(painter, caption, at + glm::vec2(size.x / 2, size.y / 2 + 5), 12, 3,
             inside(mouse, at, size) ? CYAN : glm::vec4(1), plasma::CENTRE);
}

void Interface::drawEdit(Painter &painter, glm::vec2 labelAt, const std::u16string &label, const std::u16string &text,
                         bool focused, double seconds) {
    outlined(painter, label, labelAt, 12, 3, glm::vec4(1), plasma::CENTRE);
    const glm::vec2 box = labelAt + glm::vec2(-EDIT.x / 2, 10);
    if (editBox) {
        placeWidget(*editBox, 0, box);
        painter.draw(*editBox, 0, Transform(1));
    }
    const bool caret = focused && std::fmod(seconds, 1.0) < 0.5;
    outlined(painter, text + (caret ? u"_" : u""), box + glm::vec2(4, 15), 12, 3, glm::vec4(1));
}

// The panels' column (§3.3): dragged by the mouse, coasting after a fling and slowing down,
// eased back towards its bounds.
void Interface::scrollList(float &top, float &speed, size_t count, glm::vec2 W, glm::vec2 mouse, float dtMs) {
    if (leftHeld && lastMouseY >= 0) {
        const float dy = mouse.y - lastMouseY;
        top += dy;
        if (dtMs > 0)
            speed = dy / dtMs * 1000.f; // pixels per second, kept for the coast
    } else {
        top += speed * 0.001f * dtMs;
        speed *= std::pow(0.995f, dtMs);
    }
    lastMouseY = mouse.y;
    const float span = 230.f * (float)count - 10.f;
    float T = 10, B = W.y - 10 - span;
    if (span < W.y - 20)
        T = B = std::trunc(W.y / 2) - std::trunc(span / 2);
    const float f = 1 - std::pow(0.99f, dtMs);
    if (top > T)
        top += (T - top) * f;
    else if (top < B)
        top += (B - top) * f;
}

void Interface::drawFront(Painter &painter, glm::vec2 W, glm::vec2 mouse, float dtMs, double seconds) {
    drawButton(painter, backButton().at, backButton().size, u"Back", mouse);
    if (front == Front::Worlds) {
        // §4.3: the multiplayer toggle (its caption as Alpha writes it) and Connect to server.
        drawButton(painter, multiplayerButton(W).at, multiplayerButton(W).size,
                   !multiplayerToggled ? u"Multiplayer Worlds..." : multiplayerWorlds ? u"Singleplayer worlds..." : u"Multiplayer worlds...",
                   mouse);
        drawButton(painter, connectToServerButton(W).at, connectToServerButton(W).size, u"Connect to server", mouse);
    }
    if (front == Front::Connect) {
        // §6: the address and Connect.
        drawEdit(painter, {std::trunc(W.x / 2), std::trunc(W.y / 2) - 100}, u"Server address", serverText, true, seconds);
        drawButton(painter, connectButton(W).at, connectButton(W).size, u"Connect", mouse);
        return;
    }
    if (front == Front::Characters || front == Front::Worlds) {
        const bool chars = front == Front::Characters;
        const size_t count = (chars ? characterList.size() : worldList.size()) + 1;
        float &top = chars ? characterListTop : worldListTop;
        panelYaw -= 0.05f * dtMs;
        scrollList(top, chars ? characterListSpeed : worldListSpeed, count, W, mouse, dtMs);
        int &selected = chars ? selectedCharacter : selectedWorld;
        for (size_t i = 0; i < count; i++) {
            const glm::vec2 at(std::trunc(W.x / 2) - 300, top + 230.f * (float)i);
            if (inside(mouse, at, PANEL) && selected != (int)i)
                selected = (int)i, deleteShown = false; // §3.3 step 4: the mouse picks
            if (listPanel) {
                placeWidget(*listPanel, 0, at);
                painter.draw(*listPanel, 0, Transform(1));
            }
            if (i + 1 == count) {
                outlined(painter, chars ? u"New character" : u"New world", at + glm::vec2(300, 30), 14, 3, glm::vec4(1),
                         plasma::CENTRE);
            } else if (chars) {
                const CharacterSummary &c = characterList[i];
                // §4.1: the character, 17 blocks away, its origin at (100, 110) in the panel.
                if (characterParts)
                    for (const auto &[mesh, matrix] : characterParts(i, panelYaw))
                        painter.item(modelDraw(mesh, matrix, at + glm::vec2(100, 110), {-95, 0, 45}, 17, W));
                const int cls = c.characterClass >= 1 && c.characterClass <= 4 ? c.characterClass : 0;
                outlined(painter, utf16(c.name), at + glm::vec2(200, 30), 14, 3, glm::vec4(1), plasma::WRAP, 180);
                outlined(painter,
                         u"LVL " + number(c.level) + u" " + raceWord(c.type) + u" " + CLASS_NAMES[cls], at + glm::vec2(200, 55),
                         12, 3, glm::vec4(1));
                outlined(painter, std::u16string(u"Specialization: ") + SPECIALIZATIONS[cls][c.specialization & 1],
                         at + glm::vec2(200, 75), 10, 3, glm::vec4(1));
            } else {
                const WorldSummary &w = worldList[i];
                // §4.2: the preview model in the panel's left part, slowly turning (believed).
                if (worldPreview)
                    if (const auto [mesh, size] = worldPreview(i); mesh)
                        painter.item(itemDraw(mesh, size, false, at + glm::vec2(100, 110), {-120, 0, panelYaw * 0.2f}, 0.12f, 0, W));
                char explored[32];
                std::snprintf(explored, sizeof explored, "%g", (double)w.discovered);
                outlined(painter, utf16(w.name), at + glm::vec2(200, 30), 14, 3, glm::vec4(1), plasma::WRAP, 180);
                outlined(painter, u"Explored: " + utf16(explored) + std::u16string(u" km") + (char16_t)0x00b2, at + glm::vec2(200, 55), 10, 3, glm::vec4(1));
                outlined(painter, u"Seed: " + number(w.seed), at + glm::vec2(200, 80), 10, 3, glm::vec4(1));
            }
        }
        // §3.4: Select (or Create) on the chosen panel, the small delete button, Delete.
        if (selected >= 0 && selected < (int)count) {
            const glm::vec2 at(std::trunc(W.x / 2) - 300, top + 230.f * (float)selected);
            const bool isNew = selected + 1 == (int)count;
            drawButton(painter, at + glm::vec2(380, 170), {200, 30}, isNew ? u"Create" : u"Select", mouse);
            if (!isNew) {
                drawButton(painter, at + glm::vec2(560, 20), {20, 20}, u"x", mouse);
                if (deleteShown)
                    drawButton(painter, at + glm::vec2(620, 0), {300, 30}, chars ? u"Delete Character" : u"Delete World", mouse);
            }
        }
    } else if (front == Front::Creation) {
        drawStylePanel(painter, W, mouse);
        drawEdit(painter, {std::trunc(W.x / 2), W.y - 50}, u"Name", nameText, true, seconds);
        if (characterNameTaken())
            outlined(painter, u"There is already a character with that name.", {std::trunc(W.x / 2), W.y - 200}, 12, 3,
                     glm::vec4(1), plasma::CENTRE);
        else if (nameText.size() >= 2 && nameText.size() <= 15)
            drawButton(painter, createButton(W).at, createButton(W).size, u"Create Character", mouse);
    } else if (front == Front::WorldCreation) {
        drawEdit(painter, {std::trunc(W.x / 2), W.y - 100}, u"Seed (must be a number)", seedText, editFocus == 1, seconds);
        drawEdit(painter, {std::trunc(W.x / 2), W.y - 50}, u"Name", nameText, editFocus == 0, seconds);
        if (worldNameTaken())
            outlined(painter, u"There is already a world with that name.", {std::trunc(W.x / 2), W.y - 200}, 12, 3,
                     glm::vec4(1), plasma::CENTRE);
        else if (worldReady())
            drawButton(painter, createButton(W).at, createButton(W).size, u"Create World", mouse);
    }
}

void Interface::drawStylePanel(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 P(20, W.y - 20 - STYLE.y), local = mouse - P;
    if (stylePanel) {
        placeWidget(*stylePanel, 0, P);
        painter.draw(*stylePanel, 0, Transform(1));
    }
    static const char16_t *const LABELS[5] = {u"Race", u"Gender", u"Class", u"Face", u"Haircut"};
    const std::u16string values[5] = {RACES[creation.race], creation.gender ? u"Female" : u"Male",
                                      CLASS_NAMES[creation.characterClass + 1], u"Face " + number(creation.face + 1),
                                      u"Haircut " + number(creation.haircut + 1)};
    for (int k = 0; k < 5; k++) {
        const float baseline = 27 + 30.f * (float)k;
        outlined(painter, LABELS[k], P + glm::vec2(15, baseline), 12, 3, glm::vec4(1));
        outlined(painter, values[k], P + glm::vec2(195, baseline), 12, 3, glm::vec4(1), plasma::CENTRE);
        for (auto [doc, at] : {std::pair{&leftArrow, glm::vec2(100, 12 + 30.f * k)}, std::pair{&rightArrow, glm::vec2(266, 12 + 30.f * k)}}) {
            if (!*doc)
                continue;
            tint(**doc, 0, inside(local, at, {19, 21}) ? CYAN : glm::vec4(1));
            placeWidget(**doc, 0, P + at);
            painter.draw(**doc, 0, Transform(1));
        }
    }
    outlined(painter, u"Hair color", P + glm::vec2(150, 182), 12, 3, glm::vec4(1), plasma::CENTRE);
    // The palette; the swatch under the mouse previews its colour (§3.9).
    std::array<uint8_t, 3> hover = creation.colour;
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 16; c++) {
            const glm::vec2 at = P + glm::vec2(13 + 17.f * c, 197 + 17.f * r);
            painter.rectangle(at, at + glm::vec2(17), {0, 0, 0, 1});
            const glm::vec3 colour = swatch(c, r);
            painter.rectangle(at + glm::vec2(1), at + glm::vec2(16), glm::vec4(colour, 1));
            if (inside(mouse, at, glm::vec2(17)))
                hover = {(uint8_t)(colour.r * 255), (uint8_t)(colour.g * 255), (uint8_t)(colour.b * 255)};
        }
    if (hover != creation.hover) {
        creation.hover = hover;
        creationChanged = true;
    }
}

bool Interface::characterNameTaken() const {
    std::string name;
    for (char16_t ch : nameText)
        name += ch < 0x80 ? (char)ch : '?';
    return std::any_of(characterList.begin(), characterList.end(), [&](const CharacterSummary &c) { return c.name == name; });
}

bool Interface::worldNameTaken() const {
    std::string name;
    for (char16_t ch : nameText)
        name += ch < 0x80 ? (char)ch : '?';
    return std::any_of(worldList.begin(), worldList.end(), [&](const WorldSummary &w) { return w.name == name; });
}

bool Interface::worldReady() const {
    return nameText.size() >= 2 && !seedText.empty() &&
           std::all_of(seedText.begin(), seedText.end(), [](char16_t ch) { return ch >= u'0' && ch <= u'9'; });
}

bool Interface::frontPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (front == Front::None)
        return false;
    if (button != 0)
        return true;
    characterListSpeed = 0; // a press stops a flung character list (not the world list, as in Alpha)
    if (inside(mouse, backButton().at, backButton().size)) {
        // §1.2 Back.
        front = front == Front::WorldCreation || front == Front::Connect ? Front::Worlds
                : front == Front::Worlds      ? Front::Characters
                : front == Front::Creation    ? Front::Characters
                                              : Front::None;
        if (front == Front::None)
            startScreen = true;
        return true;
    }
    if (front == Front::Worlds && inside(mouse, multiplayerButton(W).at, multiplayerButton(W).size)) {
        multiplayerWorlds = !multiplayerWorlds;
        multiplayerToggled = true;
        frontRequest = FrontRequest::ListWorlds;
        return true;
    }
    if (front == Front::Worlds && inside(mouse, connectToServerButton(W).at, connectToServerButton(W).size)) {
        front = Front::Connect;
        serverText.clear();
        return true;
    }
    if (front == Front::Connect) {
        if (!serverText.empty() && inside(mouse, connectButton(W).at, connectButton(W).size))
            frontRequest = FrontRequest::Connect;
        return true;
    }
    if (front == Front::Characters || front == Front::Worlds) {
        const bool chars = front == Front::Characters;
        const size_t count = (chars ? characterList.size() : worldList.size()) + 1;
        const int selected = chars ? selectedCharacter : selectedWorld;
        const float top = chars ? characterListTop : worldListTop;
        if (selected < 0 || selected >= (int)count)
            return true;
        const glm::vec2 at(std::trunc(W.x / 2) - 300, top + 230.f * (float)selected);
        const bool isNew = selected + 1 == (int)count;
        if (inside(mouse, at + glm::vec2(380, 170), {200, 30})) {
            if (chars && isNew) {
                front = Front::Creation;
                creation = Creation{};
                nameText.clear();
                creationChanged = true;
            } else if (chars) {
                frontRequest = FrontRequest::SelectCharacter;
            } else if (isNew) {
                front = Front::WorldCreation;
                editFocus = 0;
            } else {
                frontRequest = FrontRequest::SelectWorld;
            }
        } else if (!isNew && inside(mouse, at + glm::vec2(560, 20), {20, 20})) {
            deleteShown = true;
        } else if (!isNew && deleteShown && inside(mouse, at + glm::vec2(620, 0), {300, 30})) {
            frontRequest = chars ? FrontRequest::DeleteCharacter : FrontRequest::DeleteWorld;
            deleteShown = false; // (Alpha leaves it showing; one confirmation per delete here)
        }
        return true;
    }
    if (front == Front::Creation) {
        const glm::vec2 P(20, W.y - 20 - STYLE.y), local = mouse - P;
        for (int k = 0; k < 5; k++)
            for (int side = 0; side < 2; side++) {
                if (!inside(local, glm::vec2(side ? 266 : 100, 12 + 30.f * k), {19, 21}))
                    continue;
                const int step = side ? 1 : -1;
                switch (k) {
                case 0: creation.race = (creation.race + step + 8) % 8; break;
                case 1: creation.gender ^= 1; break;
                case 2: creation.characterClass = (creation.characterClass + step + 4) % 4; break;
                case 3: creation.face += step; break;
                case 4: creation.haircut += step; break;
                default: break;
                }
                // Face and haircut wrap by the new type's counts: below 0 to the last, past it to 0.
                const int32_t type = creatureTypeOf(creation.race, creation.gender == 1);
                const auto wrap = [](int &v, int32_t n) { v = v < 0 ? n - 1 : v >= n ? 0 : v; };
                wrap(creation.face, faceCount(type));
                wrap(creation.haircut, haircutCount(type));
                inventorySounds.push_back(0x55);
                creationChanged = true;
            }
        if (inside(local, {10, 200}, {280, 150})) {
            creation.colour = creation.hover;
            inventorySounds.push_back(0x55);
            creationChanged = true;
        }
        if (!characterNameTaken() && nameText.size() >= 2 && nameText.size() <= 15 &&
            inside(mouse, createButton(W).at, createButton(W).size))
            frontRequest = FrontRequest::CreateCharacter;
        return true;
    }
    if (front == Front::WorldCreation) {
        if (inside(mouse, glm::vec2(std::trunc(W.x / 2) - EDIT.x / 2, W.y - 90), EDIT))
            editFocus = 1;
        if (inside(mouse, glm::vec2(std::trunc(W.x / 2) - EDIT.x / 2, W.y - 40), EDIT))
            editFocus = 0;
        if (!worldNameTaken() && worldReady() && inside(mouse, createButton(W).at, createButton(W).size))
            frontRequest = FrontRequest::CreateWorld;
        return true;
    }
    return true;
}

void Interface::frontType(char16_t ch, bool backspace) {
    std::u16string *text = front == Front::Connect    ? &serverText
                           : front == Front::Creation ? &nameText
                           : front == Front::WorldCreation ? (editFocus == 1 ? &seedText : &nameText)
                                                           : nullptr;
    if (!text)
        return;
    if (backspace) {
        if (!text->empty())
            text->pop_back();
    } else if (ch >= 32) {
        text->push_back(ch);
    }
}

} // namespace ui
