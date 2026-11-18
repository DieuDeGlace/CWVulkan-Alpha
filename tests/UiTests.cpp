// Checks the interface file reader against alpha-ui-plasma.md §8: the seal and name scrambling,
// and every shipped .plx file parsing to the counts of §3.8 with its textures inflated.
// Usage: UiTests [game folder]
#include "Config.h"
#include "Install.h"
#include "ui/Plasma.h"
#include "ui/Interaction.h"
#include "ui/LandNames.h"
#include "ui/Screens.h"
#include "creatures/Hits.h"
#include "creatures/Physics.h"
#include "Loot.h"
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

// A click on the character screens as the game delivers it: an update, the press, `held`
// updates with the button down, then the release (event 3).
static void frontClick(Interface &ui, glm::vec2 window, glm::vec2 at, int held = 0) {
    ui.frontUpdate(window, at, 0);
    ui.frontPress(window, at, 0);
    for (int i = 0; i < held; i++)
        ui.frontUpdate(window, at, 0);
    ui.frontRelease(at);
}

// alpha-character-screens.md re-certification (2026-11-16) through the public screen API.
static void checkCharacterScreens() {
    using R = Interface::FrontRequest;
    const glm::vec2 window{1280, 720}, select{820, 235}, small{910, 80}, remove{1100, 65}, nowhere{100, 600};
    const auto click = [&](Interface &ui, glm::vec2 at, int held = 0) { frontClick(ui, window, at, held); };
    const auto characters = [](Interface &ui, int n) {
        ui.front = Interface::Front::Characters;
        for (int i = 0; i < n; i++)
            ui.characterList.push_back({"C" + std::to_string(i)});
    };

    // §3.3 easing share and worked example 8.2 (one character, dt 16: both panels move ~12.6).
    check(std::fabs(listEaseShare(0.01f, 16) - 0.14854f) < 1e-4f && listEaseShare(0.01f, 0) == 0 &&
              listEaseShare(0.01f, -3) == 0,
          "list easing share is 1 - 0.99^dt, none for dt <= 0");
    {
        Interface ui;
        characters(ui, 1);
        ui.selectedCharacter = 1;
        ui.frontUpdate(window, {640, 0}, 16);
        ui.frontUpdate(window, {640, 61}, 0);
        const bool notYet = ui.selectedCharacter == 1;
        ui.frontUpdate(window, {640, 63}, 0);
        check(notYet && ui.selectedCharacter == 0, "worked example 8.2: the short column eases 12.6 px towards its centred bound");
    }
    // §3.3 / §3.4: while the left button is held the pick is skipped and the buttons hide.
    {
        Interface ui;
        characters(ui, 2);
        ui.frontUpdate(window, {640, 100}, 0);
        ui.frontPress(window, {640, 100}, 0);
        ui.frontUpdate(window, {640, 300}, 16);
        const bool kept = ui.selectedCharacter == 0;
        ui.frontRelease({640, 300});
        ui.frontUpdate(window, {640, 300}, 0);
        check(kept && ui.selectedCharacter == 1, "a held button skips the pick; after the release the mouse picks again");
    }
    // §3.4, §3.5: the small button confirms, any held press withdraws it, deletion keeps both.
    {
        Interface ui;
        characters(ui, 2);
        ui.frontUpdate(window, {640, 100}, 0);
        click(ui, remove);
        check(ui.frontRequest == R::None, "Delete Character answers only once the small button showed it");
        click(ui, small, 1);
        click(ui, remove, 2);
        check(ui.frontRequest == R::DeleteCharacter && ui.selectedCharacter == 0,
              "a release reaches the Delete Character shown at the press, and the selection stays");
        ui.frontRequest = R::None;
        click(ui, small);
        click(ui, nowhere, 1);
        click(ui, remove);
        check(ui.frontRequest == R::None, "a held press anywhere withdraws the delete confirmation");
        click(ui, small);
        click(ui, remove);
        ui.frontRequest = R::None;
        click(ui, remove);
        check(ui.frontRequest == R::DeleteCharacter, "Delete Character stays shown after a deletion (native)");
    }
    // §2.1: event 3 goes to the visible widget under the release, not checked against the press.
    {
        Interface ui;
        characters(ui, 1);
        ui.frontUpdate(window, select, 0);
        ui.frontRelease(select);
        check(ui.frontRequest == R::None, "a release without its press does nothing");
        ui.frontPress(window, select, 0);
        ui.frontRelease({100, 40});
        check(ui.front == Interface::Front::None && ui.startScreen && ui.frontRequest == R::None,
              "a press on Select released over Back is Back");
    }
    // §3.5: deleting the last character leaves Select on the New character panel as "Create".
    {
        Interface ui;
        characters(ui, 1);
        ui.frontUpdate(window, {640, 100}, 0);
        click(ui, small);
        click(ui, remove);
        ui.characterList.clear(); // (the game's rebuild)
        click(ui, select);
        check(ui.front == Interface::Front::Creation, "after the last character's deletion Select is Create");
    }
    // §3.7: saved names are widened and compared with the wide edit text.
    {
        Interface ui;
        ui.front = Interface::Front::Creation;
        ui.characterList.push_back({"caf\xe9"});
        ui.characterList.push_back({"ab?"});
        const char16_t eAcute = 0xe9; // (spelt as a unit: the build sets no source character set)
        ui.nameText = std::u16string(u"caf") + eAcute;
        click(ui, {1100, 680});
        check(ui.frontRequest == R::None, "a widened saved name matches the typed name");
        ui.nameText = std::u16string(u"ab") + eAcute;
        click(ui, {1100, 680});
        check(ui.frontRequest == R::CreateCharacter, "a typed unit above 0x7f is not narrowed before the check");
    }
    // §4.1, §4.3, §4.5, §5.3, §5.4: the world list.
    {
        Interface ui;
        ui.front = Interface::Front::Worlds;
        ui.worldList = {{"online_", 1, 0}, {"online", 2, 0}, {"Home", 3, 0}};
        ui.frontUpdate(window, {640, 100}, 0);
        check(ui.selectedWorld == 1, "a six-letter name is singleplayer; the panel picks its world index");
        click(ui, select);
        check(ui.frontRequest == R::SelectWorld && ui.selectedWorld == 1, "world Select asks for the selected world index");
        ui.frontRequest = R::None;
        click(ui, small);
        click(ui, remove);
        check(ui.frontRequest == R::DeleteWorld && ui.selectedWorld == 1, "Delete World never changes the selection");
        ui.frontRequest = R::None;
        click(ui, {1100, 40});
        ui.frontRequest = R::None;
        ui.frontUpdate(window, {640, 100}, 0);
        click(ui, select);
        check(ui.multiplayerWorlds && ui.frontRequest == R::SelectWorld && ui.selectedWorld == 0,
              "a name of exactly online_ is an online world");
        ui.frontRequest = R::None;
        ui.front = Interface::Front::WorldCreation;
        ui.nameText = u"Home";
        ui.seedText = u"2147483647";
        click(ui, {1100, 680});
        check(ui.frontRequest == R::None, "the world name check covers the worlds the list does not show");
        ui.nameText = u"Home2";
        click(ui, {1100, 680});
        check(ui.frontRequest == R::CreateWorld && ui.selectedWorld == 3 && ui.worldSeed() == 2147483647u,
              "Create World selects M and parses the largest signed seed");
    }
    // §3.3 dragging: a press re-anchors the smoothed cursor, so the drag moves by the mouse only.
    {
        Interface ui;
        characters(ui, 4);
        ui.selectedCharacter = 4;
        ui.frontUpdate(window, {0, 0}, 0);
        ui.frontMouseMove(100, 1000);
        ui.frontPress(window, {100, 100}, 0);
        ui.frontMouseMove(100, 2000);
        ui.frontMouseMove(300, 3000);
        ui.frontRelease({100, 300});
        ui.frontUpdate(window, {640, 300}, 0);
        check(ui.selectedCharacter == 0, "a drag from a fresh press moves the panels by the smoothed mouse change");
    }
    // §3.2: figures turn only while hovered, and ease back by the asin rule otherwise.
    {
        float share = 0;
        for (int i = 0; i < 16; i++)
            share += (1 - share) * 0.01f;
        check(std::fabs(panelYawStep(0, true, 20) + 1) < 1e-6f && panelYawStep(30, false, 0) == 30 &&
                  std::fabs(panelYawStep(30, false, 16) - (30 - 30 * share)) < 1e-3f && panelYawStep(350, false, 16) > 350,
              "panel yaw turns 0.05 deg/ms while hovered and eases back to the nearest turn otherwise");
    }
}

static void checkMenuRequests() {
    {
        Interface vendor;
        creatures::InventoryValue source;
        source.groups.resize(3);
        auto item = creatures::freshInventoryItem();
        item[0] = 11; item[1] = 12;
        source.groups[0] = {{0, item}};
        source.groups[2] = {{-7, item}, {5, item}};
        source.auxiliary = 0xabcdef01;
        source.accumulators = {0x80000001, 0xffffffff};
        source.separate[2] = 0x99;
        vendor.vendorInventory.separate[2] = 0x66;
        vendor.vendorInventory.copyFrom(source);
        check(vendor.vendorInventory.groups.size() == 3 && vendor.vendorInventory.groups[1].empty() &&
              vendor.vendorInventory.groups[0][0].count == 0 && vendor.vendorInventory.groups[2][0].count == -7 &&
              vendor.vendorInventory.groups[2][1].count == 5 && vendor.vendorInventory.auxiliary == source.auxiliary &&
              vendor.vendorInventory.accumulators == source.accumulators && vendor.vendorInventory.separate[2] == 0x66 &&
              vendor.vendorInventory.groups[2].data() != source.groups[2].data(),
              "vendor export preserves full groups/counts/scalars and destination gaps in independently owned rows");
    }
    // Drive the engine's public screen input API, without touching owner saves.
    // These checks cover requests; Main's save/renderer integration is separate.
    const glm::vec2 window{1280, 720}, listButton{820, 235}, createButton{1100, 680}, backButton{100, 40};
    // A click as the game delivers it: an update, the press, `held` updates with the button
    // down, the release (event 3).
    const auto click = [&](Interface &ui, glm::vec2 at, int held = 0) { frontClick(ui, window, at, held); };
    Interface menu;
    menu.front = Interface::Front::Characters;
    menu.frontUpdate(window, listButton, 0);
    menu.frontPress(window, listButton, 0);
    check(menu.front == Interface::Front::Characters, "the list buttons do not answer the press");
    menu.frontRelease(listButton);
    check(menu.front == Interface::Front::Creation && menu.creationChanged && menu.creation.hover == std::array<uint8_t, 3>{255, 255, 255},
          "new-character Create (on the release) opens creation, resets the hover colour to white and applies");
    menu.frontType(u'A', false);
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::None, "one-letter character name cannot request a save");
    for (char16_t ch : std::u16string(u"lpha")) menu.frontType(ch, false);
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::CreateCharacter && menu.nameText == u"Alpha" &&
              menu.selectedCharacter == 0 && menu.activeCharacter == 0,
          "typed character name reaches the creation request, which selects the new character N");
    menu.frontRequest = Interface::FrontRequest::None;
    menu.characterList.push_back({"Alpha"});
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::None, "duplicate character name does not request overwrite");
    click(menu, backButton);
    click(menu, listButton);
    check(menu.frontRequest == Interface::FrontRequest::SelectCharacter,
          "returning to the list can select the existing character");

    menu.frontRequest = Interface::FrontRequest::None;
    menu.front = Interface::Front::Worlds;
    click(menu, listButton);
    check(menu.front == Interface::Front::WorldCreation, "new-world button opens world creation");
    menu.nameText.clear();
    for (char16_t ch : std::u16string(u"Test")) menu.frontType(ch, false);
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::None, "world creation requires a seed");
    click(menu, {640, 640});
    for (char16_t ch : std::u16string(u"41")) menu.frontType(ch, false);
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::CreateWorld && menu.nameText == u"Test" && menu.seedText == u"41" &&
              menu.worldSeed() == 41,
          "world name and separately focused seed reach the creation request");
    menu.frontRequest = Interface::FrontRequest::None;
    menu.frontType(u'x', false);
    click(menu, createButton);
    check(menu.frontRequest == Interface::FrontRequest::None, "nonnumeric world seed cannot request a save");
    click(menu, backButton);
    click(menu, {1100, 40});
    check(menu.multiplayerWorlds && menu.frontRequest == Interface::FrontRequest::ListWorlds,
          "multiplayer toggle requests the corresponding world list");
    menu.frontRequest = Interface::FrontRequest::None;
    click(menu, createButton);
    check(menu.front == Interface::Front::Connect, "connect button opens the server field");
    click(menu, {640, 360});
    check(menu.frontRequest == Interface::FrontRequest::None, "empty server field does not request a connection");
    for (char16_t ch : std::u16string(u"127.0.0.1")) menu.frontType(ch, false);
    click(menu, {640, 360});
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

static void checkPlatinumAdaption() {
    const glm::vec2 window{1280, 720}, adaptButton{930, 460};
    for (uint32_t balance : {68u, 69u, UINT32_MAX}) {
        auto wallet = creatures::Inventory::fresh();
        wallet.money = 1234;
        creatures::ItemRecord grant{};
        grant[0] = 13;
        creatures::setItemLevel(grant, 69);
        wallet.add(grant);
        wallet.platinum = balance;
        auto &weapon = wallet.equipment[creatures::MAIN_HAND];
        weapon[0] = 3;
        creatures::setItemLevel(weapon, 10);
        Interface service;
        service.inventory = &wallet;
        service.adaptOpen = true;
        service.adaptBox = creatures::MAIN_HAND;
        service.sheet.level = 20;
        check(service.inventoryPress(window, adaptButton, 0), "the public inventory input reaches adaptation");
        const bool paid = balance == 69;
        check(wallet.platinum == (paid ? 0u : balance) && wallet.money == 1234 &&
              creatures::itemLevel(weapon) == (paid ? 20 : 10) && bool(weapon[0x0e] & 1) == paid &&
              service.equipmentChanged == paid && service.inventorySounds.size() == size_t(paid),
              "required corrected adaptation price spends Platinum Coins at equality and rejects insufficient or signed-negative balances");
        if (paid) {
            service.inventoryPress(window, adaptButton, 0);
            check(wallet.platinum == 0 && service.inventorySounds.size() == 1,
                  "an already adapted selection cannot charge or emit the success sound again");
        }
    }
    auto wallet = creatures::Inventory::fresh();
    auto &weapon = wallet.equipment[creatures::MAIN_HAND];
    weapon[0] = 3;
    creatures::setItemLevel(weapon, 30);
    Interface service;
    service.inventory = &wallet;
    service.adaptOpen = true;
    service.adaptBox = creatures::MAIN_HAND;
    service.sheet.level = 20;
    service.inventoryPress(window, adaptButton, 0);
    check(wallet.platinum == 0 && creatures::itemLevel(weapon) == 20 && (weapon[0x0e] & 1),
          "a zero-price selection still receives the player's low level word and adapted flag");
}

// Skills §3.1, §5.3, §5.4.
static void checkSkillTooltip() {
    check(skillTooltipPunctuation(u":") && skillTooltipPunctuation(u"/") && skillTooltipPunctuation(u")") &&
              skillTooltipPunctuation(u"-") && !skillTooltipPunctuation(u"a:") && !skillTooltipPunctuation(u"10/20") &&
              !skillTooltipPunctuation(u"(1)") && !skillTooltipPunctuation(u"("),
          "skill tooltip punctuation is exact equality, also for : / )");
    const creatures::SkillLevels none{};
    auto zero = skillTooltipVariables(0, 1, 0, none), one = skillTooltipVariables(1, 1, 0, none);
    check(zero["@ridingspeed"] == "0%" && one["@ridingspeed"] == "109.09%", "@ridingspeed is 0 % below level 1");
    check(zero["@sneakcooldown"] == "0" && one["@sneakcooldown"] == "0", "@sneakcooldown has no length entry");
    const auto two = skillTooltipVariables(2, 1, 0, none);
    check(two.at("@smashcooldown") == "17.67" && two.at("@warfrenzycooldown") == "55" &&
              skillTooltipVariables(3, 1, 0, none).at("@smashcooldown") == "16.77",
          "worked-example cooldowns, 60000 - 30000 v for war frenzy");
    check(creatures::skillPoints(10, 5) == 19 && creatures::skillPoints(1, -5) == -1,
          "skill budget divides the cube counter truncating toward zero");
}

// alpha-interaction.md §2, §3, §5.2: squared reach, the shared fall-back, the dead clearing,
// the item fall-back and the prompt order.
static void checkInteractionTargets() {
    const std::function<bool()> aimed = [] { return true; }, missed = [] { return false; };
    {
        // §8 example 4: a stool at 3 blocks (9) beats an NPC at 3.5 (12.25) as fall-back.
        TargetPasses p;
        p.creature(7, 12.25, 40, false);
        p.staticObject({1, 2, 3}, 9, 30, missed);
        const Targets t = p.finish();
        check(t.creature == 0 && t.object == Triple{1, 2, 3}, "nearer static replaces the creature fall-back");
    }
    {
        // Reach is squared: a creature 4.5 blocks away (20.25) is no fall-back; 3.9 (15.21) is.
        TargetPasses far, near;
        far.creature(7, 20.25, 1, false);
        near.creature(7, 15.21, 1, false);
        check(far.finish().creature == 0 && near.finish().creature == 7, "creature fall-back below squared 16");
        TargetPasses table;
        bool asked = false;
        table.staticObject({0, 0, 1}, 20.25, 1, [&] { return asked = true; });
        check(!table.finish().object && !asked, "a static beyond squared 16 is never a target (example 3)");
    }
    {
        // A static no nearer than the creature fall-back leaves it in place.
        TargetPasses p;
        p.creature(9, 9, 60, false);
        p.staticObject({0, 0, 4}, 10, 50, missed);
        const Targets t = p.finish();
        check(t.creature == 9 && !t.object, "a farther static keeps the creature fall-back");
    }
    {
        // §8 example 2: the door's camera distance 30.25 beats the NPC's 36.
        TargetPasses p;
        p.creature(5, 12.25, 36, true);
        p.staticObject({3, 3, 0}, 9, 30.25, aimed);
        const Targets t = p.finish();
        check(t.creature == 0 && t.object == Triple{3, 3, 0}, "nearest box to the camera wins");
    }
    {
        // Dead: cleared after the static pass, but the fall-backs restore them.
        TargetPasses p;
        p.creature(5, 4, 36, true);
        p.dead();
        const Targets t = p.finish();
        check(t.creature == 5, "the fall-backs run whatever the player's health");
        // An item fall-back clears the creature and static fall-backs; uncarriable counts 16 more.
        TargetPasses q;
        q.creature(5, 12.25, 40, false);
        q.staticObject({0, 0, 1}, 9, 30, missed);
        q.item({0, 0, 2}, 2.25, 10, true, missed);
        const Targets u = q.finish();
        check(u.creature == 0 && !u.object && u.item == Triple{0, 0, 2}, "item fall-back clears the other fall-backs");
        TargetPasses r;
        r.item({0, 0, 1}, 1, 1, false, missed);
        r.item({0, 0, 2}, 9, 9, true, missed);
        check(r.finish().item == Triple{0, 0, 2}, "an item it cannot carry counts 16 further");
    }
    check(creaturePrompt(3, true, 2, false, false, 0) == u"[R] Talk" && creaturePrompt(3, false, 2, false, false, 0).empty(),
          "Talk needs look mode");
    check(creaturePrompt(5, true, 25, true, true, 0) == u"[R] Ride" && creaturePrompt(5, true, 25, true, true, 106).empty() &&
              creaturePrompt(5, true, 25, false, true, 0).empty() && creaturePrompt(5, true, 25, true, false, 0).empty(),
          "Ride: own rideable pet, riding skill, not riding");
    const uint64_t none = ~uint64_t(0);
    const PetCandidate pets[] = {{1, 5, 9, 3}, {2, 5, 16, none}, {3, 5, 30, none}, {4, 3, 1, none}};
    check(freePetClaim(pets) == 0, "the nearest pet is owned: nothing claimed");
    const PetCandidate free[] = {{2, 5, 16, none}, {5, 5, 16, none}, {3, 5, 24.9, none}};
    check(freePetClaim(free) == 2, "nearest free pet within squared 25, first on ties");
}

// Synthetic Plasma documents (no owner content): nodes with a transformation, a display and,
// where asked, a mesh shape.
static plasma::Document plasmaDoc(size_t nodes, bool shapes = false) {
    plasma::Document doc;
    for (size_t i = 0; i < nodes; i++) {
        plasma::Node n;
        n.transformation = (int32_t)i;
        n.display = (int32_t)i;
        doc.transformations.emplace_back().deformation.value = glm::mat4(1);
        plasma::Display &d = doc.displays.emplace_back();
        d.visibility.value = 1;
        if (shapes) {
            n.shape = (int32_t)i;
            doc.shapes.emplace_back();
        }
        doc.nodes.push_back(n);
    }
    return doc;
}
template <class T>
static void keyed(plasma::Attribute<T> &a, const std::string &name, std::vector<T> frames,
                  std::vector<plasma::Key> keys) {
    a.frames = std::move(frames);
    a.value = a.frames[0];
    a.sequences.push_back({name, std::move(keys)});
}
static bool near2(glm::vec2 a, glm::vec2 b, float e = 1e-4f) { return glm::length(a - b) < e; }
static glm::vec2 through(const Transform &t, glm::vec2 p) { return glm::vec2(t * glm::vec3(p, 1)); }

// alpha-ui-plasma.md re-certification (2026-11-16) corrections 1-5 and rulings manacubebar §2, §3.
static void checkPlasmaRuntime() {
    using plasma::Key;
    // Correction 1: x' = t + p + R·D·(x − p), D in full before the rotations, z then y then x.
    {
        plasma::Document doc = plasmaDoc(1);
        plasma::Transformation &tr = doc.transformations[0];
        tr.translation.value = {100, 50};
        tr.pivot.value = {7, 3};
        const auto at = [&](glm::vec2 x) { return through(Painter::nodeTransform(doc, doc.nodes[0]), x); };
        check(near2(at({8, 3}), {108, 53}), "identity R and D move a point by t; the pivot is no offset");
        tr.rotation.value = {0, 0, 90};
        check(near2(at({8, 3}), {107, 54}), "a positive z angle turns +x towards +y (clockwise on screen) about the pivot");
        tr.deformation.value[0][0] = 2;
        check(near2(at({8, 3}), {107, 55}), "the deformation scales before the rotation");
        tr.deformation.value[3] = glm::vec4(5, 0, 0, 1);
        check(near2(at({8, 3}), {107, 60}), "the deformation's translation row applies before the rotation");
        tr.deformation.value = glm::mat4(1);
        tr.rotation.value = {180, 0, 0};
        check(near2(at({8, 5}), {108, 51}), "an x rotation mirrors y about the pivot");
        // The full 4x4: x feeding z in D, brought back into x by a y rotation.
        tr.translation.value = tr.pivot.value = glm::vec2(0);
        tr.rotation.value = {0, 90, 0};
        const glm::vec2 plain = at({1, 0});
        tr.deformation.value[0][2] = 1;
        check(near2(plain, {0, 0}) && near2(at({1, 0}), {1, 0}), "D's z terms take part before the y rotation");
        // Correction 2: own matrix first, then the parent's.
        plasma::Document tree = plasmaDoc(2);
        tree.transformations[0].rotation.value = {0, 0, 90};
        tree.transformations[1].translation.value = {10, 0};
        const Transform world = Painter::nodeTransform(tree, tree.nodes[0]) * Painter::nodeTransform(tree, tree.nodes[1]);
        check(near2(through(world, {0, 0}), {0, 10}), "a child's offset turns with its parent");
    }
    // Corrections 3-4: frame 0 is the current value; sorted keys; implicit start from frame 1.
    {
        plasma::Document doc = plasmaDoc(1);
        plasma::Attribute<glm::vec2> &a = doc.transformations[0].translation;
        keyed(a, "show", {{1, 1}, {99, 99}, {0, 0}, {10, 10}}, {{3, 200, 0}, {2, 100, 0}});
        a.value = {4, 4}; // a game write replaces frame 0
        plasma::play(doc, 0, "show");
        const auto &keys = a.playback.keys;
        check(keys.size() == 3 && keys[0].frame == 1 && keys[0].timeMs == 0 && keys[1].timeMs == 100 &&
                  keys[2].timeMs == 200 && a.frames[1] == glm::vec2(4, 4),
              "start sorts the keys and snapshots the current value into frame 1 as a key at 0");
        check(plasma::playing(doc, 0), "a node plays from the play call on");
        plasma::advance(doc, 0, 100);
        check(a.value == glm::vec2(4, 4), "evaluation starts from the snapshot at elapsed 0");
        plasma::advance(doc, 0, 100);
        check(a.value == glm::vec2(0, 0), "the first file key is reached at its time");
        plasma::Attribute<glm::vec2> &b = doc.transformations[0].pivot;
        keyed(b, "loop", {{0, 0}, {0, 0}, {5, 5}, {6, 6}}, {{2, 0, 0}, {3, 400, 0}});
        plasma::play(doc, 0, "loop", 300);
        check(b.playback.keys.size() == 1 && b.playback.periodMs == 300, "a period keeps only earlier keys");
        plasma::play(doc, 0, "nothing");
        keyed(doc.transformations[0].rotation, "empty", {glm::vec3(0), glm::vec3(0)}, {});
        plasma::play(doc, 0, "empty");
        check(doc.transformations[0].rotation.playback.keys.empty(), "a sequence without keys does not start");
    }
    // Correction 5: Bezier segments with clamped (once) or wrapped (looping) neighbours.
    {
        plasma::Document doc = plasmaDoc(1);
        plasma::Attribute<float> &r = doc.displays[0].blurRadius;
        const std::vector<float> values{0, 0, 0, 10, 30};
        keyed(r, "s1", values, {{2, 0, 1}, {3, 100, 1}, {4, 200, 1}});
        r.sequences.push_back({"s0", {{2, 0, 0}, {3, 100, 0}, {4, 200, 0}}});
        plasma::play(doc, 0, "s1");
        plasma::advance(doc, 0, 50);
        plasma::advance(doc, 0, 50);
        check(std::fabs(r.value - 3.125f) < 1e-5f, "smoothness 1 with the first neighbour clamped");
        plasma::play(doc, 0, "s0");
        plasma::advance(doc, 0, 50);
        plasma::advance(doc, 0, 50);
        check(std::fabs(r.value - 5) < 1e-5f, "smoothness 0 is linear");
        plasma::play(doc, 0, "s0", 300);
        plasma::advance(doc, 0, 250);
        plasma::advance(doc, 0, 100);
        const bool wrapped = std::fabs(r.value - 15) < 1e-4f;
        plasma::advance(doc, 0, 100);
        check(wrapped && std::fabs(r.value - 5) < 1e-4f, "looping: the last segment runs to key 0 a period on, neighbours wrap");
        for (int i = 0; i < 20; i++)
            plasma::advance(doc, 0, 100);
        check(plasma::playing(doc, 0), "a looping sequence keeps running");
        // Integers step: key i while u < 1, key i + 1 at u = 1.
        plasma::Attribute<int32_t> &v = doc.displays[0].clipping;
        keyed(v, "blink", {0, 0, 0, 1, 0}, {{2, 0, 0}, {3, 100, 0}, {4, 200, 0}});
        plasma::play(doc, 0, "blink");
        plasma::advance(doc, 0, 50);
        plasma::advance(doc, 0, 50);
        const bool before = v.value == 0;
        plasma::advance(doc, 0, 50);
        check(before && v.value == 1, "integer attributes step to the next key at u = 1");
        // A key on a missing frame ends the sequence without a change.
        plasma::Document bad = plasmaDoc(1);
        keyed(bad.displays[0].fillColor, "x", {glm::vec4(1), glm::vec4(0)}, {{9, 0, 0}, {9, 100, 0}});
        plasma::play(bad, 0, "x");
        plasma::advance(bad, 0, 16);
        check(!plasma::playing(bad, 0) && bad.displays[0].fillColor.value == glm::vec4(1),
              "a missing key frame stops the sequence");
    }
    // Rulings manacubebar §2: running until the second evaluation at or after the last key.
    {
        plasma::Document bar = plasmaDoc(5, true);
        for (int32_t c = 1; c <= 4; c++)
            bar.nodes[0].children.push_back(c);
        const std::vector<glm::vec4> blue{glm::vec4(0, 0, 1, 1)}, white{glm::vec4(1)}, grey{glm::vec4(.5f)};
        keyed(bar.shapes[0].mesh.colors, "manacubes", {blue, blue, white, grey, blue}, {{2, 0, 1}, {3, 600, 1}, {4, 1300, 1}});
        int32_t cubes = 3;
        check(showManaCubeIcons(bar, cubes) == 3, "icons show count mod 4");
        // Pickup frame: the count reaches 4, `manacubes` plays; the HUD then the update run.
        cubes++;
        plasma::play(bar, 0, "manacubes");
        int frames = 0, shown = 0;
        bool allLit = true;
        for (; frames < 200; frames++) {
            const int32_t k = showManaCubeIcons(bar, cubes);
            if (k != 4)
                break;
            for (int32_t c = 1; c <= 4; c++)
                allLit = allLit && bar.displays[c].visibility.value == 1;
            shown++;
            plasma::advance(bar, 0, 16);
        }
        check(shown == 84 && allLit, "four icons for ceil(1300/16) + 2 = 84 frames at 16 ms, got " + std::to_string(shown));
        check(showManaCubeIcons(bar, cubes) == 0 && bar.displays[1].visibility.value == 0 &&
                  bar.shapes[0].mesh.colors.value == blue,
              "then count mod 4, the last key's value held");
        cubes = 8; // a bulk copy plays nothing
        check(showManaCubeIcons(bar, cubes) == 0 && showManaCubeIcons(bar, -5) == -1, "plain C remainder otherwise");
        // One advance per object per update, even when nodes share a shape.
        plasma::Document shared = plasmaDoc(2, true);
        shared.nodes[0].children.push_back(1);
        shared.nodes[1].shape = 0;
        keyed(shared.shapes[0].mesh.colors, "manacubes", {blue, blue, white, grey, blue}, {{2, 0, 1}, {3, 600, 1}, {4, 1300, 1}});
        plasma::play(shared, 0, "manacubes");
        plasma::advance(shared, 0, 16);
        check(shared.shapes[0].mesh.colors.playback.elapsedMs == 16, "a shared shape advances once per update");
        // The walk skips hidden nodes unless UPDATE_HIDDEN reaches them.
        plasma::Document hidden = plasmaDoc(2);
        hidden.nodes[0].children.push_back(1);
        hidden.displays[1].visibility.value = 0;
        keyed(hidden.transformations[1].translation, "show", {glm::vec2(0), glm::vec2(0), glm::vec2(1)}, {{2, 0, 0}});
        plasma::play(hidden, 0, "show");
        plasma::advance(hidden, 0, 16);
        const bool skipped = hidden.transformations[1].translation.value == glm::vec2(0);
        plasma::advance(hidden, 0, 16, true);
        const bool entered = hidden.transformations[1].translation.value == glm::vec2(1);
        check(skipped && entered && plasma::playing(hidden, 0), "hidden nodes advance only under UPDATE_HIDDEN");
    }
}

static void checkObjectiveDialogue() {
    Dictionary dictionary;
    check(dictionary.parse("<dict><speech key=\"objective:invasion\">Now: North-West, captain; yes! Go? Done. % later</speech></dict>"),
          "objective test dictionary parses");
    MissionArea mission;
    mission.kind = 7;
    const worldgen::Location location;
    check(objectiveText(mission, location, dictionary) == u"Now: North-West, captain; yes! Go? Done.",
          "objective text uses the first page, seven attached punctuation tokens and hyphen glue");
    mission.kind = 6;
    check(objectiveText(mission, location, dictionary).empty(), "a live kind with no template produces empty text");

    Interface ui;
    ui.creatureAt = [](uint64_t id) { return std::pair{glm::dvec3(double(id), 0, 0), 2.f}; };
    int prepared = 0;
    const auto pages = [&] {
        ++prepared;
        return nothingSpecial();
    };
    ui.talk(0, pages);
    ui.talk(0, pages); // finish typing
    ui.talk(0, pages); // advance past the only page
    check(prepared == 1, "ordinary talk advances the owned bubble for an exact zero actor key");
    ui.talk(0, pages, true);
    ui.advanceBubbles(0);
    for (uint64_t id : {1ull, 2ull, 3ull})
        ui.talk(id, pages);
    int emptyAttempts = 0;
    ui.talk(0, [&] {
        ++emptyAttempts;
        return std::vector<SpeechPage>{};
    }, true);
    ui.talk(4, pages, true);
    check(prepared == 5 && emptyAttempts == 1,
          "an owned restart resets page and timing, and empty preparation retains the slot without preparing a fifth bubble");
}

int main(int argc, char **argv) {
    checkPlasmaRuntime();
    checkInteractionTargets();
    checkPlatinumAdaption();
    checkMenuRequests();
    checkCharacterScreens();
    checkSkillTooltip();
    checkObjectiveDialogue();
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
    {
        // Re-certified §5.9.1: eleven types are keyed for every variant 0..9 (towns of every
        // style are "N City"); type 5 only 0..4, type 14 only 0..5; exact pairs otherwise.
        Dictionary names;
        check(names.parse("<dict><name key=\"Village\"><singular>@ City</singular></name>"
                          "<name key=\"Ruins\"><singular>Ruins of @</singular></name>"
                          "<name key=\"Island\"><singular>@ Island</singular></name></dict>"),
              "test dictionary parses");
        worldgen::Location L;
        L.serial = 274;
        const auto named = [&](int32_t type, int32_t variant) {
            L.type = type, L.variant = variant;
            return locationName(L, names);
        };
        check(named(1, 0) == "Aruion City" && named(1, 4) == "Aruion City" && named(1, 9) == "Aruion City",
              "every town variant is N City");
        check(named(1, 10).empty() && named(15, 9) == "Aruion Island", "variants 0..9 only");
        check(named(5, 3) == "Ruins of Aruion" && named(14, 1) == "Ruins of Aruion" && named(5, 5).empty(),
              "single-row types keep their exact pairs");
    }

    // Screens §5.6: the combat text producers and the worked example.
    {
        const auto hit = hitText(23.6f, 0, false, false, false, glm::dvec3(0));
        check(hit && hit->text == u"23" && hit->size == 20 && hit->rise == -100 && hit->colour == glm::vec4(1), "hit 23.6");
        const auto small = hitText(0.46f, 0, false, false, false, glm::dvec3(0));
        check(small && small->text == u"0.4", "hit 0.46 shows 0.4");
        check(hitText(5, 1, false, false, false, glm::dvec3(0))->text == u"BLOCK", "block");
        check(!hitText(5, 4, false, false, false, glm::dvec3(0)), "kind 4 shows nothing");
        check(!hitText(0.05f, 0, false, false, false, glm::dvec3(0)), "tiny hits show nothing");
        const auto taken = hitText(10, 0, true, false, true, glm::dvec3(0));
        check(taken->size == 60 && taken->rise == 100 && taken->colour == glm::vec4(1, 0.3f, 0.3f, 1), "a critical hit taken");
        // Re-certified correction 66: −200 only for flagged damage to others; heals sink on the
        // player and rise −100 elsewhere; the +0x44 flag turns damage to others yellow.
        const auto critical = hitText(10, 0, true, true, false, glm::dvec3(0));
        check(critical->rise == -200 && critical->size == 60 && critical->colour == glm::vec4(1, 1, 0.25f, 1),
              "a flagged hit on another creature");
        check(hitText(-10, 0, true, false, false, glm::dvec3(0))->rise == -100 &&
                  hitText(-10, 0, true, false, true, glm::dvec3(0))->rise == 100,
              "flagged heals never rise by 200");
        creatures::HitRecord timed;
        timed.amount = -0.46f;
        timed.position = {65537, -65537, 32769};
        const auto heal = timedHealText(timed, 2.25f);
        check(heal && heal->text == u"0.4" && heal->colour == glm::vec4(0.4f, 1, 0.5f, 1) &&
                  heal->anchor == glm::dvec3(65537, -65537, 32769 + 65536 - 8192) / 65536.0 &&
                  heal->size == 20 && heal->lifeMs == 1000 && heal->rise == 100 && heal->flags == 1,
              "timed heal uses the fixed recipient-size offset and local display fields");
        auto connected = timed;
        connected.amount = 12.9f;
        connected.critical = connected.area = 1;
        const auto local = hitText(connected, true, 2.25f);
        const auto other = hitText(connected, false, 2.25f);
        check(local && other && local->text == u"12" && local->rise == 100 && local->size == 60 &&
                  local->colour == glm::vec4(1, .3f, .3f, 1) &&
                  other->rise == -200 && other->colour == glm::vec4(1, 1, .25f, 1) &&
                  local->anchor == glm::dvec3(65537, -65537, 32769 + 65536 - 8192) / 65536.0 &&
                  other->anchor == glm::dvec3(65537, -65537, 32769 + 131072 - 8192) / 65536.0,
              "connected hit labels retain fixed position bits, live size, and the local-target anchor step");
        timed.amount = -0.09f;
        check(!timedHealText(timed, 2.25f), "timed heal display respects the magnitude threshold");
        check(levelUpText(glm::dvec3(0)).lifeMs == 3000 && experienceText(12, glm::dvec3(0)).text == u"XP +12", "level and XP");
    }
    {
        QuickItemWheel wheel;
        wheel.index = -1;
        check(wheel.selected(4) == 3, "quick-item selection wraps a negative ordinal");
        wheel.earlyCount = 3;
        check(wheel.selected(2) == 1 && wheel.lateSelection(2) == 0,
              "negative late selection wraps against the early stack count before the new count");
        wheel.open = true;
        wheel.index = 1;
        wheel.layout(1, 4);
        check(std::abs(wheel.radius - 2.f) < 0.00001f && std::abs(wheel.angle + 0.9f) < 0.00001f,
              "one millisecond moves the wheel radius and angle one percent toward their targets");
        wheel.layout(10000, 4);
        const glm::vec2 first = wheel.offset(0, 4), selected = wheel.offset(1, 4);
        check(glm::length(first - glm::vec2(-200, 0)) < 0.001f &&
              glm::length(selected - glm::vec2(0, -200)) < 0.001f, "chosen item lands at the top of the wheel");
        wheel.open = false;
        wheel.layout(16, 0);
        check(!wheel.open && wheel.radius == 0 && wheel.index == 1,
              "a closed wheel clears only the radius");

        wheel.open = true;
        check(wheel.steer(16, true, false, false) == 0, "a held direction at opening waits for a neutral frame");
        wheel.steer(16, false, false, false);
        check(wheel.steer(16, true, false, false) == 1 && wheel.index == 0, "a neutral frame arms left steering");
        check(wheel.steer(183, true, false, false) == 0 &&
              wheel.steer(1, true, false, false) == 0 &&
              wheel.steer(1, true, false, false) == 1 && wheel.index == -1,
              "repeat admission tests the timer before subtracting each frame duration");
        wheel.steer(0, false, false, false);
        check(wheel.steer(0, true, true, false) == 2 && wheel.index == -1,
              "simultaneous fresh directions make two selections");
        check(wheel.steer(200, false, false, true) == 0 && !wheel.open, "forward or back closes before selection");
    }
    {
        creatures::Inventory inventory = creatures::Inventory::fresh();
        creatures::ItemRecord bomb{};
        bomb[0] = 1;
        bomb[1] = 7;
        creatures::setItemLevel(bomb, 1);
        inventory.bags[0][0] = {1, bomb};
        int calls = 0;
        const auto apply = [&](const creatures::ItemRecord &item) {
            ++calls;
            check(item == bomb && inventory.bags[0][0].count == 1, "item use observes the stack before decrement");
        };
        inventory.useSlot(0, 0, false, 1, 1, false, apply);
        check(calls == 0 && !inventory.bags[0][0].empty(), "stun declines use and preserves the stack");
        inventory.useSlot(0, 0, false, 1, 1, true, apply);
        check(calls == 1 && inventory.bags[0][0].empty(), "an eligible quick use removes exactly one unit afterward");
    }
    {
        // Screens §5.2.6: a teleport takes two clicks on the same portal label.
        ui::Interface map;
        creatures::Body body;
        body.place({10, 20, 30});
        body.velocity = {-3, 5, 7};
        map.mapOpen = map.teleportMode = true;
        map.hoveredPortal = {5, 7};
        map.mapRelease(&body);
        check(body.blocks() == glm::dvec3(10, 20, 30) && map.selectedPortal == glm::ivec2(5, 7) && map.mapOpen,
              "first release selects the portal and keeps the current position");
        map.mapRelease(&body);
        check(body.position[0] == (1408ll << 16) && body.position[1] == (1920ll << 16) && body.position[2] == 0 &&
              !map.mapOpen && map.selectedPortal == glm::ivec2(-1) && map.hoveredPortal == glm::ivec2(-1),
              "second release writes the fixed zone centre and clears both map indices");
        check(body.velocity == glm::vec3(-3, 5, 7), "portal release preserves motion");
        map.mapOpen = true, map.teleportMode = false;
        map.hoveredPortal = {5, 7};
        body.place({10, 20, 30});
        map.mapRelease(&body);
        map.mapRelease(&body);
        check(body.blocks() == glm::dvec3(10, 20, 30) && map.mapOpen, "without teleport mode clicks only select");
        map.hoveredPortal = glm::ivec2(-1);
        map.mapRelease(&body);
        check(map.selectedPortal == glm::ivec2(-1), "a click elsewhere deselects");
    }
    {
        // Screens §8.1.6: the largest side spans 1.2071·H·σ pixels. A 10-voxel cube at
        // σ = 0.1 on a 1280 x 720 window: its right face 43.5 px from the centre point (86.9 px
        // across, independent of the depth offset; the negated horizontal scale puts +x on the
        // left, whether that mirrors the model is U-F6-13).
        const ui::ItemDraw d = ui::itemDraw(1, glm::vec3(10), false, {640, 360}, glm::vec3(0), 0.1f, 0, {1280, 720});
        const glm::vec4 p = d.camera * glm::vec4(5, 0, 5, 1);
        const float x = (p.x * d.lens.x + d.lens.z * p.z) / p.z * 640 + 640;
        check(std::abs(p.z - 1) < 1e-5f, "the centre is one unit ahead");
        check(std::abs(x - (640 - 43.4556f)) < 0.01f, "item model size " + std::to_string(x));
        const ui::ItemDraw tooltip = ui::itemDraw(1, glm::vec3(10), false, {640, 360}, glm::vec3(0), 0.1f, -0.05f, {1280, 720});
        check(tooltip.camera == d.camera && tooltip.lens == d.lens && tooltip.depth == -0.05f,
              "the tooltip's depth offset is applied after projection and keeps the 86.9 px size");
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
