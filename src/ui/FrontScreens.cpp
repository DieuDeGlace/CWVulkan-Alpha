// The screens between the start menu and a world (docs/specs/alpha-character-screens.md):
// the character list, character creation with its style panel, the world list and world
// creation. The game reads and writes the saves; these screens show the lists, edit the
// choices and ask for what the buttons mean (`frontRequest`).
#include "NewCharacter.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace ui {
namespace {

using namespace widgets;

constexpr glm::vec2 PANEL{600, 220}, STYLE{300, 330}, EDIT{125, 20};
constexpr double PI = 3.14159265358979323846;
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

// §3.3: the eased value (1 − f)·value + f·target, single precision.
float eased(float value, float target, float f) { return (1 - f) * value + f * target; }

// §3.2: per millisecond v += (target − v)·0.01, single precision.
template <typename T> T approach(T value, T target, int32_t dt) {
    for (int32_t i = 0; i < dt; i++)
        value += (target - value) * 0.01f;
    return value;
}

// §3.7, §5.3: a saved name widened unit by unit (bytes zero-extended; open point C-2) equals
// the edit's wide text exactly.
bool sameName(const std::string &saved, const std::u16string &typed) {
    return saved.size() == typed.size() && std::equal(saved.begin(), saved.end(), typed.begin(), [](char a, char16_t b) {
               return (char16_t)(unsigned char)a == b;
           });
}

} // namespace

float listEaseShare(float rate, int32_t dt) {
    // f := f + (1 − f)·r, dt times in double with the single-precision rate, then binary32.
    double f = 0;
    for (int32_t i = 0; i < dt; i++)
        f += (1.0 - f) * (double)rate;
    return (float)f;
}

float panelYawStep(float yaw, bool hovered, float dtMs) {
    // Hovered: −0.05° per ms. Otherwise yaw += asin(clamp(−sin yaw)) · f in degrees, the share
    // f built in single precision. (The trigonometry is evaluated in double; its native
    // precision is not stated.)
    if (hovered)
        return yaw - 0.05f * dtMs;
    float share = 0;
    for (int32_t i = 0; i < (int32_t)dtMs; i++)
        share += (1 - share) * 0.01f;
    const double back = std::asin(std::clamp(-std::sin((double)yaw * PI / 180.0), -1.0, 1.0));
    return (float)((double)yaw + back * (double)share * 180.0 / PI);
}

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

int Interface::worldPanel(int worldIndex) const {
    if (worldIndex == (int)worldList.size())
        return (int)listedWorlds.size(); // the "New world" panel stands for M
    const auto it = std::find(listedWorlds.begin(), listedWorlds.end(), (uint32_t)worldIndex);
    return worldIndex >= 0 && it != listedWorlds.end() ? (int)(it - listedWorlds.begin()) : -1;
}

bool Interface::activeLastWorld(const WorldSummary &world) const {
    if (activeCharacter < 0 || activeCharacter >= (int)characterList.size())
        return false;
    const CharacterSummary &c = characterList[activeCharacter];
    return c.lastWorldSeed == world.seed && c.lastWorldName == world.name;
}

void Interface::stepFigure(PanelFigure &figure, bool hovered, float dtMs) const {
    // §3.2: hovered, the figure turns and walks on the spot; otherwise it turns back and stops.
    const int32_t ms = (int32_t)dtMs;
    figure.yaw = panelYawStep(figure.yaw, hovered, dtMs);
    figure.velocity = approach(figure.velocity, hovered ? glm::vec3(0, 8, 0) : glm::vec3(0), ms);
    figure.intent = approach(figure.intent, hovered ? glm::vec3(0, 3, 0) : glm::vec3(0), ms);
}

// §3.3, §3.4 (§4.3 for the world list): one list's column, its pick and its buttons.
void Interface::stepList(bool chars, glm::vec2 W, glm::vec2 mouse, float dtMs) {
    PanelColumn &col = chars ? characterColumn : worldColumn;
    const size_t count = (chars ? characterList.size() : listedWorlds.size()) + 1;
    if (col.rebuild || col.y.size() != count) {
        // §3.1, §4.1: the rebuilt list's panels at (300, 50 + 230 i), new world widgets unturned.
        col.y.resize(count);
        for (size_t i = 0; i < count; i++)
            col.y[i] = 50 + 230.f * (float)i;
        col.x = 300;
        col.rebuild = false;
        if (!chars)
            worldYaw.assign(count - 1, 0.f);
    }
    if (front != (chars ? Front::Characters : Front::Worlds)) {
        col.speed = 0;
        return;
    }
    const int32_t ms = (int32_t)dtMs, height = (int32_t)W.y, halfH = height / 2;
    const float x = (float)((int32_t)W.x / 2 - 300), last = PANEL.y;
    int &selected = chars ? selectedCharacter : selectedWorld;
    ListButtons &buttons = chars ? characterButtons : worldButtons;
    if (!leftHeld) {
        // 1. The bounds; a column shorter than the window is centred.
        float T = 10, B = (float)height - 10 - last;
        const int32_t span = (int32_t)(col.y.back() + last - col.y.front());
        if (span < height - 20) {
            T = (float)(halfH - span / 2);
            B = (float)(halfH + span / 2) - last;
        }
        const float f = listEaseShare(0.01f, ms);
        const auto moveAll = [&](float by) {
            for (float &y : col.y)
                y += by;
            col.x = x;
        };
        // 2, 3. Pulled down to the top bound, then up to the bottom bound.
        if (col.y.front() > T)
            moveAll(eased(col.y.front(), T, f) - col.y.front());
        if (col.y.front() < T && col.y.back() < B)
            moveAll(eased(col.y.back(), B, f) - col.y.back());
        // 4. Coast, and the panel under the mouse is selected.
        for (size_t i = 0; i < count; i++) {
            col.x = x;
            col.y[i] += col.speed * 0.001f * dtMs;
            if (inside(mouse, {col.x, col.y[i]}, PANEL)) {
                // A world panel stands for its world index, the "New world" panel for M.
                const int index = chars                     ? (int)i
                                  : i < listedWorlds.size() ? (int)listedWorlds[i]
                                                            : (int)worldList.size();
                if (index != selected)
                    buttons.remove = false;
                selected = index;
            }
        }
    }
    // 5. Friction, held or not.
    col.speed = eased(col.speed, 0, listEaseShare(0.005f, ms));

    // §3.4: while the left button is held the three buttons hide (and only the small button
    // shows Delete again); otherwise Select and the small button sit on the selected panel.
    const int total = (int)(chars ? characterList.size() : worldList.size()); // N or M
    if (leftHeld) {
        buttons.select = buttons.small = buttons.remove = false;
    } else if (const int panel = chars ? selected : worldPanel(selected); panel >= 0 && panel < (int)count) {
        buttons.at = {col.x, col.y[panel]};
        buttons.select = buttons.small = true;
    } else {
        buttons.select = buttons.small = buttons.remove = false;
    }
    buttons.create = selected >= total;
    if (buttons.create)
        buttons.small = buttons.remove = false;

    // §3.2, §4.2: the figures and models turn only under the mouse.
    if (chars) {
        characterFigures.resize(characterList.size());
        for (size_t i = 0; i < characterFigures.size(); i++)
            stepFigure(characterFigures[i], inside(mouse, {col.x, col.y[i]}, PANEL), dtMs);
        return;
    }
    for (size_t j = 0; j < listedWorlds.size(); j++) {
        worldYaw[j] = panelYawStep(worldYaw[j], inside(mouse, {col.x, col.y[j]}, PANEL), dtMs);
        if (activeLastWorld(worldList[listedWorlds[j]])) {
            // P on its last world's panel: unturned, walking.
            activeFigure.yaw = 0;
            activeFigure.velocity = approach(activeFigure.velocity, glm::vec3(0, 8, 0), ms);
            activeFigure.intent = approach(activeFigure.intent, glm::vec3(0, 3, 0), ms);
        }
    }
}

void Interface::frontUpdate(glm::vec2 W, glm::vec2 mouse, float dtMs) {
    frontWindow = W;
    // §4.1: this mode's worlds, in list order.
    listedWorlds.clear();
    for (size_t i = 0; i < worldList.size(); i++)
        if (onlineWorld(worldList[i].name) == multiplayerWorlds)
            listedWorlds.push_back((uint32_t)i);
    stepList(true, W, mouse, dtMs);
    stepList(false, W, mouse, dtMs);
}

void Interface::frontMouseMove(float mouseY, uint32_t timeMs) {
    // §3.3 dragging: the smoothed cursor y eases by the milliseconds since the last move (the
    // share is believed to come from the same routine as the list's easing).
    const int32_t dt = moveTimeKnown ? (int32_t)(timeMs - moveTime) : 0;
    moveTime = timeMs;
    moveTimeKnown = true;
    const float before = smoothY, step = mouseY - moveY;
    smoothY = eased(smoothY, mouseY, listEaseShare(0.01f, dt));
    moveY = mouseY;
    if (!leftHeld || (front != Front::Characters && front != Front::Worlds))
        return;
    PanelColumn &col = front == Front::Characters ? characterColumn : worldColumn;
    for (float &y : col.y)
        y += smoothY - before;
    col.x = (float)((int32_t)frontWindow.x / 2 - 300);
    col.speed += step * (float)dt * 2;
}

void Interface::pointerPress(float mouseY, int button) {
    smoothY = mouseY;
    characterColumn.speed = 0; // (not the world list's, as in Alpha)
    if (button == 0)
        leftHeld = true;
}

void Interface::drawFront(Painter &painter, glm::vec2 W, glm::vec2 mouse, float dtMs, double seconds, bool terrainWait) {
    if (!terrainWait)
        frontUpdate(W, mouse, dtMs);
    drawButton(painter, backButton().at, backButton().size, u"Back", mouse);
    if (terrainWait && (front == Front::Characters || front == Front::Worlds))
        return;
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
        const PanelColumn &col = chars ? characterColumn : worldColumn;
        for (size_t i = 0; i < col.y.size(); i++) {
            const glm::vec2 at(col.x, col.y[i]);
            if (listPanel) {
                placeWidget(*listPanel, 0, at);
                painter.draw(*listPanel, 0, Transform(1));
            }
            if (i + 1 == col.y.size()) {
                outlined(painter, chars ? u"New character" : u"New world", at + glm::vec2(300, 30), 14, 3, glm::vec4(1),
                         plasma::CENTRE);
            } else if (chars) {
                const CharacterSummary &c = characterList[i];
                // §3.2: the character, 17 blocks away, its origin at (100, 110) in the panel.
                if (characterParts) {
                    const PanelFigure &f = characterFigures[i];
                    for (const auto &[mesh, matrix] : characterParts(i, {f.yaw, f.velocity, f.intent, dtMs}))
                        painter.item(modelDraw(mesh, matrix, at + glm::vec2(100, 110), {-95, 0, 45}, 17, W));
                }
                const int cls = c.characterClass >= 1 && c.characterClass <= 4 ? c.characterClass : 0;
                outlined(painter, utf16(c.name), at + glm::vec2(200, 30), 14, 3, glm::vec4(1), plasma::WRAP, 180);
                outlined(painter,
                         u"LVL " + number(c.level) + u" " + raceWord(c.type) + u" " + CLASS_NAMES[cls], at + glm::vec2(200, 55),
                         12, 3, glm::vec4(1));
                outlined(painter, std::u16string(u"Specialization: ") + SPECIALIZATIONS[cls][c.specialization & 1],
                         at + glm::vec2(200, 75), 10, 3, glm::vec4(1));
            } else {
                const WorldSummary &w = worldList[listedWorlds[i]];
                // §4.2: the preview model in the panel's left part (placement open point C-8).
                if (worldPreview)
                    if (const auto [mesh, size] = worldPreview(listedWorlds[i]); mesh)
                        painter.item(itemDraw(mesh, size, false, at + glm::vec2(100, 110), {-120, 0, worldYaw[i]}, 0.12f, 0, W));
                // The active character on its last world's panel (placement C-8: as on its own).
                if (characterParts && activeLastWorld(w))
                    for (const auto &[mesh, matrix] :
                         characterParts((size_t)activeCharacter, {activeFigure.yaw, activeFigure.velocity, activeFigure.intent, dtMs}))
                        painter.item(modelDraw(mesh, matrix, at + glm::vec2(100, 110), {-95, 0, 45}, 17, W));
                // The discovered counter as a signed integer made a float, in the default format.
                char explored[32];
                std::snprintf(explored, sizeof explored, "%g", (double)(float)(int32_t)w.discovered);
                outlined(painter, utf16(w.name), at + glm::vec2(200, 30), 14, 3, glm::vec4(1), plasma::WRAP, 180);
                outlined(painter, u"Explored: " + utf16(explored) + std::u16string(u" km") + (char16_t)0x00b2, at + glm::vec2(200, 55), 10, 3, glm::vec4(1));
                outlined(painter, u"Seed: " + number((int32_t)w.seed), at + glm::vec2(200, 80), 10, 3, glm::vec4(1));
            }
        }
        // §3.4: Select (or Create) on the chosen panel, the small delete button, Delete.
        const ListButtons &b = chars ? characterButtons : worldButtons;
        if (b.select)
            drawButton(painter, b.at + glm::vec2(380, 170), {200, 30}, b.create ? u"Create" : u"Select", mouse);
        if (b.small)
            drawButton(painter, b.at + glm::vec2(560, 20), {20, 20}, u"x", mouse);
        if (b.remove)
            drawButton(painter, b.at + glm::vec2(620, 0), {300, 30}, chars ? u"Delete Character" : u"Delete World", mouse);
    } else if (front == Front::Creation) {
        if (!terrainWait) {
            drawStylePanel(painter, W, mouse);
            drawEdit(painter, {std::trunc(W.x / 2), W.y - 50}, nameCaptionCleared ? u"" : u"Name", nameText, true, seconds);
        }
        if (characterNameTaken())
            outlined(painter, u"There is already a character with that name.", {std::trunc(W.x / 2), W.y - 200}, 12, 3,
                     glm::vec4(1), plasma::CENTRE);
        else if (!terrainWait && createCharacterShown())
            drawButton(painter, createButton(W).at, createButton(W).size, u"Create Character", mouse);
    } else if (front == Front::WorldCreation) {
        if (!terrainWait) {
            drawEdit(painter, {std::trunc(W.x / 2), W.y - 100}, u"Seed (must be a number)", seedText, editFocus == 1, seconds);
            drawEdit(painter, {std::trunc(W.x / 2), W.y - 50}, u"Name", nameText, editFocus == 0, seconds);
        }
        if (worldNameTaken())
            outlined(painter, u"There is already a world with that name.", {std::trunc(W.x / 2), W.y - 200}, 12, 3,
                     glm::vec4(1), plasma::CENTRE);
        else if (!terrainWait && worldReady())
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
        creationChanged = true; // Apply
    }
}

bool Interface::characterNameTaken() const {
    return std::any_of(characterList.begin(), characterList.end(), [&](const CharacterSummary &c) { return sameName(c.name, nameText); });
}

bool Interface::worldNameTaken() const {
    // §5.3: every entry, online or not, whatever the list shows.
    return std::any_of(worldList.begin(), worldList.end(), [&](const WorldSummary &w) { return sameName(w.name, nameText); });
}

bool Interface::worldReady() const {
    return nameText.size() >= 2 && !seedText.empty() &&
           std::all_of(seedText.begin(), seedText.end(), [](char16_t ch) { return ch >= u'0' && ch <= u'9'; });
}

bool Interface::createCharacterShown() const {
    // §3.7: no saved character of that name, and 2 to 15 UTF-16 units.
    return !characterNameTaken() && nameText.size() >= 2 && nameText.size() <= 15;
}

uint32_t Interface::worldSeed() const {
    // §5.4: a signed 32-bit parse. The validation admits digits only, so every seed up to
    // 2147483647 reads the same as this; above it the native field is left undefined (open
    // point C-6) and the engine keeps its wrap modulo 2^32.
    uint32_t seed = 0;
    for (char16_t ch : seedText)
        seed = seed * 10 + (uint32_t)(ch - u'0');
    return seed;
}

std::vector<Interface::FrontHit> Interface::visibleFrontButtons(glm::vec2 W) const {
    // The window root's buttons first (believed to lie above the screen containers), then the
    // screen's own; the selected-panel buttons as the last update left them (§1.3, §3.4).
    std::vector<FrontHit> out;
    const auto add = [&](FrontButton button, Button b) { out.push_back({button, b.at, b.size}); };
    add(FrontButton::Back, backButton());
    if (front == Front::Worlds) {
        add(FrontButton::Multiplayer, multiplayerButton(W));
        add(FrontButton::ConnectToServer, connectToServerButton(W));
    }
    if (front == Front::Connect)
        add(FrontButton::Connect, connectButton(W));
    if (front == Front::Characters || front == Front::Worlds) {
        const ListButtons &b = front == Front::Characters ? characterButtons : worldButtons;
        if (b.remove)
            add(FrontButton::Delete, {b.at + glm::vec2(620, 0), {300, 30}});
        if (b.small)
            add(FrontButton::Small, {b.at + glm::vec2(560, 20), {20, 20}});
        if (b.select)
            add(FrontButton::Select, {b.at + glm::vec2(380, 170), {200, 30}});
    }
    if (front == Front::Creation && createCharacterShown())
        add(FrontButton::CreateCharacter, createButton(W));
    if (front == Front::WorldCreation && !worldNameTaken() && worldReady())
        add(FrontButton::CreateWorld, createButton(W));
    return out;
}

void Interface::activateFront(FrontButton button) {
    const bool chars = front == Front::Characters;
    switch (button) {
    case FrontButton::Back: // §1.2 Back.
        front = front == Front::WorldCreation || front == Front::Connect ? Front::Worlds
                : front == Front::Worlds                                  ? Front::Characters
                : front == Front::Creation                                ? Front::Characters
                                                                          : Front::None;
        if (front == Front::None)
            startScreen = true;
        break;
    case FrontButton::Multiplayer: // §4.3: the other kind of world, listed again.
        multiplayerWorlds = !multiplayerWorlds;
        multiplayerToggled = true;
        worldColumn.rebuild = true;
        frontRequest = FrontRequest::ListWorlds;
        break;
    case FrontButton::ConnectToServer:
        front = Front::Connect;
        serverText.clear();
        break;
    case FrontButton::Connect:
        if (!serverText.empty())
            frontRequest = FrontRequest::Connect;
        break;
    case FrontButton::CreateCharacter: // §3.8: the new character is selected, appended and active.
        frontRequest = FrontRequest::CreateCharacter;
        selectedCharacter = activeCharacter = (int)characterList.size();
        characterColumn.rebuild = true;
        break;
    case FrontButton::CreateWorld: // §5.4
        frontRequest = FrontRequest::CreateWorld;
        selectedWorld = (int)worldList.size();
        worldColumn.rebuild = true;
        break;
    case FrontButton::Select:
        if (chars && selectedCharacter >= (int)characterList.size()) {
            // §3.6: Create on the "New character" panel; the label setter also blanks "Name".
            front = Front::Creation;
            creation = Creation{};
            nameText.clear();
            nameCaptionCleared = true;
            creationChanged = true;
        } else if (chars) {
            frontRequest = FrontRequest::SelectCharacter;
            activeCharacter = selectedCharacter;
        } else if (selectedWorld >= (int)worldList.size()) {
            front = Front::WorldCreation;
            editFocus = 0;
        } else {
            frontRequest = FrontRequest::SelectWorld;
        }
        break;
    case FrontButton::Small: // §3.5 step 1: the only confirmation.
        (chars ? characterButtons : worldButtons).remove = true;
        break;
    case FrontButton::Delete: // §3.5 step 3, §4.5: the selection and the button stay.
        frontRequest = chars ? FrontRequest::DeleteCharacter : FrontRequest::DeleteWorld;
        (chars ? characterColumn : worldColumn).rebuild = true;
        break;
    }
}

bool Interface::frontPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (front == Front::None)
        return false;
    pointerPress(mouse.y, button);
    if (button != 0)
        return true;
    // Engine reading of open point C-9: the release (event 3) goes to a button that was
    // visible when the press arrived, so a click held across updates still lands. A press and
    // release handled between two updates, the case the contract settles, is the same.
    pressedButtons = std::pair{front, visibleFrontButtons(W)};
    if (front == Front::Creation) {
        // §3.9: the style arrows and the palette answer the press (event 2).
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
                creationChanged = true; // Apply
            }
        if (inside(local, {10, 200}, {280, 150})) {
            creation.colour = creation.hover;
            inventorySounds.push_back(0x55);
            creationChanged = true; // Apply
        }
    } else if (front == Front::WorldCreation) {
        if (inside(mouse, glm::vec2(std::trunc(W.x / 2) - EDIT.x / 2, W.y - 90), EDIT))
            editFocus = 1;
        if (inside(mouse, glm::vec2(std::trunc(W.x / 2) - EDIT.x / 2, W.y - 40), EDIT))
            editFocus = 0;
    }
    return true;
}

void Interface::frontRelease(glm::vec2 mouse) {
    leftHeld = false;
    const auto pressed = std::exchange(pressedButtons, std::nullopt);
    if (front == Front::None || !pressed || pressed->first != front)
        return;
    for (const FrontHit &hit : pressed->second)
        if (inside(mouse, hit.at, hit.size)) {
            activateFront(hit.button);
            return;
        }
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
