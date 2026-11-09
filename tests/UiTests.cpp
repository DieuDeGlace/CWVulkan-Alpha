// Checks the interface file reader against alpha-ui-plasma.md §8: the seal and name scrambling,
// and every shipped .plx file parsing to the counts of §3.8 with its textures inflated.
// Usage: UiTests [game folder]
#include "Config.h"
#include "Install.h"
#include "ui/Plasma.h"
#include "ui/LandNames.h"
#include "ui/Screens.h"
#include <cmath>
#include <cstdio>

using namespace ui;

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

static void checkMenuRequests() {
    // Drive the engine's public screen input API, without touching owner saves.
    // These checks cover requests; Main's save/renderer integration is separate.
    const glm::vec2 window{1280, 720}, listButton{820, 235}, createButton{1100, 680}, backButton{100, 40};
    Interface menu;
    menu.front = Interface::Front::Characters;
    check(menu.frontPress(window, listButton, 0) && menu.front == Interface::Front::Creation,
          "new-character button opens creation");
    menu.frontType(u'A', false);
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::None, "one-letter character name cannot request a save");
    for (char16_t ch : std::u16string(u"lpha")) menu.frontType(ch, false);
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::CreateCharacter && menu.nameText == u"Alpha",
          "typed character name reaches the creation request");
    menu.frontRequest = Interface::FrontRequest::None;
    menu.characterList.push_back({"Alpha"});
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::None, "duplicate character name does not request overwrite");
    menu.frontPress(window, backButton, 0);
    menu.frontPress(window, listButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::SelectCharacter,
          "returning to the list can select the existing character");

    menu.frontRequest = Interface::FrontRequest::None;
    menu.front = Interface::Front::Worlds;
    menu.frontPress(window, listButton, 0);
    check(menu.front == Interface::Front::WorldCreation, "new-world button opens world creation");
    menu.nameText.clear();
    for (char16_t ch : std::u16string(u"Test")) menu.frontType(ch, false);
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::None, "world creation requires a seed");
    menu.frontPress(window, {640, 640}, 0);
    for (char16_t ch : std::u16string(u"41")) menu.frontType(ch, false);
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::CreateWorld && menu.nameText == u"Test" && menu.seedText == u"41",
          "world name and separately focused seed reach the creation request");
    menu.frontRequest = Interface::FrontRequest::None;
    menu.frontType(u'x', false);
    menu.frontPress(window, createButton, 0);
    check(menu.frontRequest == Interface::FrontRequest::None, "nonnumeric world seed cannot request a save");
    menu.frontPress(window, backButton, 0);
    menu.frontPress(window, {1100, 40}, 0);
    check(menu.multiplayerWorlds && menu.frontRequest == Interface::FrontRequest::ListWorlds,
          "multiplayer toggle requests the corresponding world list");
    menu.frontRequest = Interface::FrontRequest::None;
    menu.frontPress(window, createButton, 0);
    check(menu.front == Interface::Front::Connect, "connect button opens the server field");
    menu.frontPress(window, {640, 360}, 0);
    check(menu.frontRequest == Interface::FrontRequest::None, "empty server field does not request a connection");
    for (char16_t ch : std::u16string(u"127.0.0.1")) menu.frontType(ch, false);
    menu.frontPress(window, {640, 360}, 0);
    check(menu.frontRequest == Interface::FrontRequest::Connect && menu.serverText == u"127.0.0.1",
          "typed server address reaches the connection request");

    Interface options;
    GameOptions current;
    current[GameOptions::SOUND_VOLUME] = 50;
    options.setOptions(current);
    options.optionsOpen = true;
    options.click(window, {840, 282});
    check(options.optionsCopy[GameOptions::SOUND_VOLUME] == 60 && current[GameOptions::SOUND_VOLUME] == 50,
          "option arrow edits a draft without changing committed settings");
    check(options.click(window, {480, 550}) == Action::OptionsApply &&
              options.click(window, {630, 550}) == Action::OptionsOk &&
              options.click(window, {780, 550}) == Action::OptionsCancel,
          "Apply, OK and Cancel dispatch distinct option actions");
}

int main(int argc, char **argv) {
    checkMenuRequests();
    // §8.2
    check(plasma::unscramble({0x78, 0x87, 0x81, 0x6D, 0xBD, 0x3D, 0x2F, 0xC7, 0x6E, 0x99, 0xAC, 0x39, 0x28, 0xD1}) ==
              "PlasmaGraphics",
          "seal");
    check(plasma::unscramble({0x80, 0x98, 0x9F, 0xB0, 0x32, 0x3F, 0xD2}) == "Texture", "first name of gui.plx");

    // Screens §5.9.1: names made from numbers.
    check(landNameOf(525240) == "Duraron" && landNameOf(525241) == "Likugar" && landNameOf(526264) == "Galan" &&
              landNameOf(274) == "Aruion" && landNameOf(692) == "Duradara" && landNameOf(25782) == "Vardar" &&
              landNameOf(0) == "Krono" && landNameOf(4294967295u) == "Gela",
          "land names");

    // Screens §5.6: the combat text producers and the worked example.
    {
        const auto hit = hitText(23.6f, 0, false, false, glm::dvec3(0));
        check(hit && hit->text == u"23" && hit->size == 20 && hit->rise == -100 && hit->colour == glm::vec4(1), "hit 23.6");
        const auto small = hitText(0.46f, 0, false, false, glm::dvec3(0));
        check(small && small->text == u"0.4", "hit 0.46 shows 0.4");
        check(hitText(5, 1, false, false, glm::dvec3(0))->text == u"BLOCK", "block");
        check(!hitText(5, 4, false, false, glm::dvec3(0)), "kind 4 shows nothing");
        check(!hitText(0.05f, 0, false, false, glm::dvec3(0)), "tiny hits show nothing");
        const auto taken = hitText(10, 0, true, true, glm::dvec3(0));
        check(taken->size == 60 && taken->rise == 100 && taken->colour == glm::vec4(1, 0.3f, 0.3f, 1), "a critical hit taken");
        check(levelUpText(glm::dvec3(0)).lifeMs == 3000 && experienceText(12, glm::dvec3(0)).text == u"XP +12", "level and XP");
    }
    {
        // Screens §5.2.6: a teleport takes two clicks on the same portal label.
        ui::Interface map;
        map.mapOpen = map.teleportMode = true;
        map.hoveredPortal = {5, 7};
        map.mapRelease();
        check(map.selectedPortal == glm::ivec2(5, 7) && !map.teleportTo && map.mapOpen, "first click selects");
        map.mapRelease();
        check(map.teleportTo == glm::ivec2(5, 7) && !map.mapOpen && map.selectedPortal == glm::ivec2(-1), "second teleports");
        map.mapOpen = true, map.teleportMode = false, map.teleportTo.reset();
        map.hoveredPortal = {5, 7};
        map.mapRelease();
        map.mapRelease();
        check(!map.teleportTo && map.mapOpen, "without teleport mode clicks only select");
        map.hoveredPortal = glm::ivec2(-1);
        map.mapRelease();
        check(map.selectedPortal == glm::ivec2(-1), "a click elsewhere deselects");
    }
    {
        // Screens §8.1.6: the largest side spans 1.2071·H·σ pixels. A 10-voxel cube at
        // σ = 0.1 on a 1280 x 720 window: its right face 43.5 px from the centre point (the
        // image is mirrored, so it lands on the left).
        const ui::ItemDraw d = ui::itemDraw(1, glm::vec3(10), false, {640, 360}, glm::vec3(0), 0.1f, 0, {1280, 720});
        const glm::vec4 p = d.camera * glm::vec4(5, 0, 5, 1);
        const float x = (p.x * d.lens.x + d.lens.z * p.z) / p.z * 640 + 640;
        check(std::abs(p.z - 1) < 1e-5f, "the centre is one unit ahead");
        check(std::abs(x - (640 - 43.4556f)) < 0.01f, "item model size " + std::to_string(x));
        const ui::ItemDraw e = ui::itemDraw(1, glm::vec3(10), false, {785, 315}, glm::vec3(0), 0.03f, 0, {1280, 720});
        const glm::vec4 q = e.camera * glm::vec4(0, 0, 5, 1);
        const glm::vec2 at((q.x * e.lens.x + e.lens.z * q.z) / q.z * 640 + 640,
                           (q.y * e.lens.y + e.lens.w * q.z) / q.z * 360 + 360);
        check(glm::length(at - glm::vec2(785, 315)) < 0.01f, "the centre lands on its point");
    }

    Config config;
    if (!config.load("AlphaVulkan.ini"))
        config.load("config/AlphaVulkan.ini");
    const fs::path game = install::locate(argc > 1 ? argv[1] : config.text("Game", "Path"));
    if (game.empty()) {
        std::printf("no Alpha install found; only the scrambling checks ran\n");
        std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
        return failures ? 1 : 0;
    }

    // §3.8: nodes, mesh shapes, text shapes, transformations, widgets, textures.
    struct FileCase {
        const char *file;
        size_t nodes, meshes, texts, transformations, widgets, textures;
    } files[] = {
        {"gui.plx", 355, 199, 113, 338, 37, 4},     {"start.plx", 66, 65, 0, 66, 0, 0},
        {"help.plx", 168, 39, 72, 168, 0, 0},       {"quest-tag.plx", 25, 17, 7, 25, 0, 0},
        {"cursor.plx", 4, 1, 2, 4, 0, 0},
    };
    for (const auto &c : files) {
        std::string error;
        const auto doc = plasma::load(game / c.file, &error);
        check(doc.has_value(), std::string(c.file) + " parses: " + error);
        if (!doc)
            continue;
        size_t meshes = 0, texts = 0;
        for (const auto &s : doc->shapes)
            (s.kind == plasma::Shape::Kind::Text ? texts : meshes)++;
        const std::string f = c.file;
        check(doc->nodes.size() == c.nodes, f + " nodes " + std::to_string(doc->nodes.size()));
        check(meshes == c.meshes, f + " mesh shapes " + std::to_string(meshes));
        check(texts == c.texts, f + " text shapes " + std::to_string(texts));
        check(doc->transformations.size() == c.transformations, f + " transformations");
        check(doc->displays.size() == c.transformations, f + " displays");
        check(doc->widgets.size() == c.widgets, f + " widgets " + std::to_string(doc->widgets.size()));
        check(doc->textures.size() == c.textures, f + " textures");
        for (const auto &t : doc->textures)
            check(t.rgba.size() == (size_t)t.width * t.height * 4, f + " texture " + t.name + " inflates");
        // Every reference points inside its list.
        for (const auto &n : doc->nodes) {
            check(n.shape < (int32_t)doc->shapes.size(), f + " node " + n.name + " shape");
            check(n.transformation < (int32_t)doc->transformations.size(), f + " node " + n.name + " transformation");
            check(n.display < (int32_t)doc->displays.size(), f + " node " + n.name + " display");
            check(n.widget < (int32_t)doc->widgets.size(), f + " node " + n.name + " widget");
            for (int32_t child : n.children)
                check(child > 0 && child < (int32_t)doc->nodes.size(), f + " node " + n.name + " child");
        }
        if (f == "gui.plx") {
            check(doc->nameCount == 115, "gui.plx names " + std::to_string(doc->nameCount));
            for (const char *name : {"lifebar", "button", "edit", "itembox", "combopoints", "landname"})
                check(doc->find(name) >= 0, std::string("gui.plx has ") + name);
            const auto star = doc->textureById(5);
            check(star && star->name == "star3.png", "texture id 5 is star3");
        }
        if (f == "start.plx")
            check(doc->find("cubeworld") >= 0 && doc->find("picroma") >= 0, "start.plx logos");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
