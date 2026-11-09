#include "ui/Screens.h"
#include "ui/Widgets.h"
#include "stb_image.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ui {
namespace {

// Places a cloned template so its pivot lands on `anchor` (screens spec §2).
void anchor(plasma::Document &doc, glm::vec2 at) {
    if (!doc.nodes.empty() && doc.nodes[0].transformation >= 0) {
        plasma::Transformation &t = doc.transformations[doc.nodes[0].transformation];
        t.translation.value = at - t.pivot.value;
    }
}

const glm::vec4 HOVER{0.2f, 1, 1, 1}; // §3.2

// Writes `text` into every text node named `name` (a label is an outlined node with a plain one
// over it).
void setText(plasma::Document &doc, const char *name, const std::u16string &text) {
    for (const plasma::Node &n : doc.nodes)
        if (n.name == name && n.shape >= 0 && doc.shapes[n.shape].kind == plasma::Shape::Kind::Text)
            doc.shapes[n.shape].text.text.value = text;
}

// §5.3: a bar fills by scaling its rest deformation along x about its pivot.
void fill(plasma::Document &doc, int32_t node, const glm::mat4 &rest, float f) {
    const plasma::Node &n = doc.nodes[node];
    if (n.transformation < 0)
        return;
    glm::mat4 d = rest;
    d[0] *= f;
    doc.transformations[n.transformation].deformation.value = d;
}

std::u16string numbers(float value, float maximum) {
    return utf16(std::to_string((int64_t)value) + "/" + std::to_string((int64_t)maximum));
}

// A PNG from data3.db as an RGBA texture.
bool decodePng(const std::vector<uint8_t> &bytes, plasma::Texture &out) {
    int w = 0, h = 0, channels = 0;
    stbi_uc *pixels = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &w, &h, &channels, 4);
    if (!pixels)
        return false;
    out.width = w;
    out.height = h;
    out.rgba.assign(pixels, pixels + (size_t)w * h * 4);
    stbi_image_free(pixels);
    return true;
}

#include "quest_icons.h"

const char *const MENU_ICONS[6] = {"help.png", "skills.png", "crafting.png", "inventory.png", "worldmap.png", "system.png"};
const char *const MENU_LABELS[6] = {"F1", "x", "C", "B", "M", "O"};
const Action MENU_ACTIONS[6] = {Action::Help, Action::Skills, Action::Crafting, Action::Inventory, Action::WorldMap,
                                Action::SystemPanel};

} // namespace

bool Interface::load(const std::filesystem::path &install, std::string *error) {
    if (!fonts.load(install, error))
        return false;
    auto startDoc = plasma::load(install / "start.plx", error);
    auto guiDoc = plasma::load(install / "gui.plx", error);
    if (!startDoc || !guiDoc)
        return false;
    start = std::move(*startDoc);
    gui = std::move(*guiDoc);
    // §1: the start screen's two logos, kept as clones.
    if (const int32_t n = start.find("cubeworld"); n >= 0)
        logo = clone(start, n);
    if (const int32_t n = start.find("picroma"); n >= 0)
        picroma = clone(start, n);
    // §5: the player's bars, the crosshair and the menu buttons.
    auto bar = [&](Bar &b, const char *name, std::initializer_list<const char *> fills) {
        if (const int32_t n = gui.find(name); n >= 0) {
            b.doc = clone(gui, n);
            for (const char *f : fills)
                if (const int32_t k = b.doc->find(f); k >= 0 && b.doc->nodes[k].transformation >= 0)
                    b.fills.emplace_back(k, b.doc->transformations[b.doc->nodes[k].transformation].deformation.value);
        }
    };
    bar(lifebar, "lifebar", {"bar"});
    bar(experiencebar, "experiencebar", {"bar"});
    bar(mpbar, "mpbar", {"bar", "chargebar"});
    bar(hpbar, "hpbar", {"bar"});
    bar(staminabar, "staminabar", {"bar"});
    bar(petLifebar, "lifebar", {"bar"});
    bar(petExperiencebar, "experiencebar", {"bar"});
    if (const int32_t n = gui.find("ridingbar"); n >= 0)
        ridingbar = clone(gui, n);
    bar(creatureBars[0], "enemylifebar:small", {"bar", "bar2"});
    bar(creatureBars[1], "friendlifebar:small", {"bar", "bar2"});
    bar(creatureBars[2], "staticlifebar:small", {"bar", "bar2"});
    bar(creatureBars[3], "neutrallifebar:small", {"bar", "bar2"});
    // §5.5: the ability bar, labelled M1, M2, 1, 2, 3, 4; the quick-item button keeps its "Q".
    if (const int32_t n = gui.find("abilitybutton"); n >= 0)
        for (int i = 0; i < 6; i++) {
            abilityButtons.push_back(clone(gui, n));
            plasma::Document &doc = abilityButtons.back();
            setText(doc, "info", i == 0 ? u"M1" : i == 1 ? u"M2" : utf16(std::to_string(i - 1)));
            if (const int32_t c = doc.find("cooldown"); c >= 0 && doc.nodes[c].transformation >= 0)
                cooldownRest = doc.transformations[doc.nodes[c].transformation].deformation.value;
        }
    if (const int32_t n = gui.find("quickitembutton"); n >= 0)
        quickItemButton = clone(gui, n);
    if (const int32_t n = gui.find("landname"); n >= 0)
        landName = clone(gui, n);
    // Missions §8.2: the small quest tag, from quest-tag.plx (missing it only loses the tag).
    if (auto questDoc = plasma::load(install / "quest-tag.plx", nullptr)) {
        if (const int32_t n = questDoc->find("questtag"); n >= 0) {
            questBanner = clone(*questDoc, n);
            questBannerLength = plasma::length(*questBanner, 0, "shine");
        }
        if (const int32_t n = questDoc->find("smallquesttag"); n >= 0) {
            smallQuestTag = clone(*questDoc, n);
            if (const int32_t bar = smallQuestTag->find("bar"); bar >= 0 && smallQuestTag->nodes[bar].transformation >= 0) {
                questBarFill = bar;
                questBarRest = smallQuestTag->transformations[smallQuestTag->nodes[bar].transformation].deformation.value;
            }
        }
    }
    if (const int32_t n = gui.find("crosshair"); n >= 0)
        crosshair = clone(gui, n);
    if (const int32_t n = gui.find("menubutton"); n >= 0)
        for (int i = 0; i < 6; i++) {
            menuButtons.push_back(clone(gui, n));
            setText(menuButtons.back(), "count", utf16(MENU_LABELS[i]));
        }
    // §6.1: the options window, a `blackwidget` stretched to 450 x 440, with its arrows and
    // three 110 x 15 buttons.
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        optionsPanel = clone(gui, n);
        resizeWidget(*optionsPanel, 0, {450, 440});
    }
    if (const int32_t n = gui.find("leftbutton"); n >= 0)
        leftArrow = clone(gui, n);
    if (const int32_t n = gui.find("rightbutton"); n >= 0)
        rightArrow = clone(gui, n);
    if (const int32_t n = gui.find("button2"); n >= 0)
        for (const char16_t *label : {u"Apply", u"OK", u"Cancel"}) {
            optionsButtons.push_back(clone(gui, n));
            resizeWidget(optionsButtons.back(), 0, {110, 15});
            setText(optionsButtons.back(), "text", label);
        }
    // §3.3: the system panel, a `blackwidget` stretched to 150 x 120.
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        systemPanel = clone(gui, n);
        resizeWidget(*systemPanel, 0, {150, 120});
    }
    loadInventory();
    loadCrafting();
    loadVendor();
    loadServices();
    loadSkills();
    loadFront();
    if (const int32_t n = gui.find("blackwidget"); n >= 0) { // item services §3.1: 400 x 625
        benchPanel = clone(gui, n);
        resizeWidget(*benchPanel, 0, {400, 625});
    }
    if (const int32_t n = gui.find("speech"); n >= 0)
        speechBubble = clone(gui, n);
    return true;
}

namespace {

// §5.5: ability id -> icon in data3.db.
const std::pair<int32_t, const char *> ABILITY_ICONS[] = {
    {0, "none"}, {1, "attack-right"}, {2, "attack-left"}, {3, "stab-right"}, {4, "stab-left"}, {5, "perforate"},
    {6, "punch-right"}, {7, "punch-left"}, {8, "block"}, {9, "shield-attack"}, {0xa, "shield-slam"}, {0xb, "swirl"},
    {0xc, "slash"}, {0xd, "slash-left"}, {0xe, "slash-right"}, {0x10, "mutilate"}, {0x11, "ambush"},
    {0x12, "pierce-left"}, {0x13, "pierce-right"}, {0x14, "kick"}, {0x15, "ranger-kick"}, {0x16, "shoot"},
    {0x17, "shoot-fast"}, {0x18, "charge-shoot"}, {0x19, "salvo"}, {0x1a, "boomerang"}, {0x1b, "charge-boomerang"},
    {0x1c, "beam"}, {0x1e, "fire-shock"}, {0x1f, "fire-burst"}, {0x20, "water-shock"}, {0x21, "water-burst"},
    {0x22, "healing-stream"}, {0x24, "charge-bullet"}, {0x25, "fireball"}, {0x26, "firebolt"}, {0x27, "firebolt"},
    {0x28, "firebolt"}, {0x29, "waterbolt"}, {0x2a, "waterbolt"}, {0x2c, "waterbolt"}, {0x2b, "waterball"},
    {0x2d, "water-salvo"}, {0x2e, "fire-salvo"}, {0x30, "intercept"}, {0x31, "teleport"}, {0x32, "retreat2"},
    {0x36, "smash"}, {0x37, "salvo"}, {0x39, "slam"}, {0x3a, "slam-back"}, {0x3b, "charge-slam"}, {0x3f, "swirl"},
    {0x40, "charge-mutilate"}, {0x41, "slam-left"}, {0x42, "slam-right"}, {0x43, "jab"}, {0x4f, "sneak"},
    {0x56, "cyclone"}, {0x58, "fire-explosion"}, {0x5f, "beam"}, {0x60, "shuriken"}, {0x61, "camouflage"},
    {0x63, "aim"}, {0x64, "swiftness"}, {0x65, "bulwark"}, {0x66, "warfrenzy"}, {0x67, "mana-shield"},
};

// The icon at saturation 0 (Rec. 601 luma), for abilities that cannot be used now.
plasma::Texture greyed(const plasma::Texture &t) {
    plasma::Texture g = t;
    for (size_t i = 0; i + 3 < g.rgba.size(); i += 4) {
        const uint8_t y = (uint8_t)(0.299f * g.rgba[i] + 0.587f * g.rgba[i + 1] + 0.114f * g.rgba[i + 2]);
        g.rgba[i] = g.rgba[i + 1] = g.rgba[i + 2] = y;
    }
    return g;
}

} // namespace

void Interface::loadIcons(const std::function<std::optional<std::vector<uint8_t>>(const std::string &)> &read) {
    // §5.5: every ability icon, in colour and grey, in each ability button.
    std::map<std::string, std::pair<plasma::Texture, plasma::Texture>> decoded;
    for (const auto &[id, name] : ABILITY_ICONS) {
        auto it = decoded.find(name);
        if (it == decoded.end()) {
            plasma::Texture icon;
            const auto bytes = read(std::string(name) + ".png");
            if (!bytes || !decodePng(*bytes, icon))
                continue;
            icon.name = std::string(name) + ".png";
            it = decoded.emplace(name, std::make_pair(icon, greyed(icon))).first;
        }
        for (plasma::Document &doc : abilityButtons) {
            int32_t next = 1;
            for (const plasma::Texture &t : doc.textures)
                next = std::max(next, t.id + 1);
            plasma::Texture colour = it->second.first, grey = it->second.second;
            colour.id = next, grey.id = next + 1;
            doc.textures.push_back(std::move(colour));
            doc.textures.push_back(std::move(grey));
            abilityIcons[id] = {next, next + 1};
        }
    }

    // §8.1.3: the inventory tabs' icons, set the same way as the menu row's.
    static const char *const TAB_ICONS[4] = {"inventory-equipment.png", "inventory-items.png",
                                             "inventory-ingredients.png", "inventory-pets.png"};
    for (size_t i = 0; i < inventoryTabs.size() && i < 4; i++) {
        plasma::Document &doc = inventoryTabs[i];
        const auto bytes = read(TAB_ICONS[i]);
        const plasma::Node &root = doc.nodes[0];
        plasma::Texture icon;
        if (!bytes || root.shape < 0 || doc.shapes[root.shape].kind != plasma::Shape::Kind::Mesh || !decodePng(*bytes, icon))
            continue;
        icon.id = 1;
        for (const plasma::Texture &t : doc.textures)
            icon.id = std::max(icon.id, t.id + 1);
        icon.name = TAB_ICONS[i];
        doc.textures.push_back(std::move(icon));
        doc.shapes[root.shape].mesh.texture.value = doc.textures.back().id;
    }

    // Skills §2.1, §2.4: every icon a skill or specialization button may show, colour and grey.
    const auto addIcon = [&](plasma::Document &doc, std::map<std::string, std::pair<int32_t, int32_t>> &ids,
                             const std::string &file) {
        plasma::Texture icon;
        const auto bytes = read(file);
        if (!bytes || !decodePng(*bytes, icon))
            return;
        icon.name = file;
        const plasma::Texture grey = greyed(icon);
        const int32_t colourId = widgets::addTexture(doc, icon);
        ids[file] = {colourId, widgets::addTexture(doc, grey)};
    };
    skillButtonIcons.assign(skillButtons.size(), {});
    for (size_t i = 0; i < skillButtons.size(); i++) {
        for (const char *file : {"pet-taming.png", "pet-riding.png", "climbing.png", "hang-gliding.png", "swimming.png",
                                 "ship-driving.png"})
            addIcon(skillButtons[i], skillButtonIcons[i], file);
        for (int32_t id : {54, 86, 102, 101, 21, 50, 99, 100, 88, 34, 103, 49, 48, 79, 97, 96})
            for (const auto &[known, name] : ABILITY_ICONS)
                if (known == id)
                    addIcon(skillButtons[i], skillButtonIcons[i], std::string(name) + ".png");
    }
    specializationIcons.assign(specializationButtons.size(), {});
    for (size_t s = 0; s < specializationButtons.size(); s++)
        for (const char *file : {"warrior-berserker.png", "warrior-guardian.png", "ranger-sniper.png", "ranger-scout.png",
                                 "mage-fire.png", "mage-water.png", "rogue-assassin.png", "rogue-ninja.png"})
            addIcon(specializationButtons[s], specializationIcons[s], file);

    // Talk §3.3: the vendor's tabs, buy and buy back.
    for (size_t k = 0; k < vendorTabs.size() && k < 2; k++) {
        plasma::Document &doc = vendorTabs[k];
        const char *name = k == 0 ? "buy.png" : "buy-back.png";
        plasma::Texture icon;
        const auto bytes = read(name);
        const plasma::Node &root = doc.nodes[0];
        if (!bytes || !decodePng(*bytes, icon) || root.shape < 0 || doc.shapes[root.shape].kind != plasma::Shape::Kind::Mesh)
            continue;
        icon.name = name;
        doc.shapes[root.shape].mesh.texture.value = widgets::addTexture(doc, icon);
    }

    // Crafting §4.1: every icon the crafting tabs may show, in each tab (the class picks).
    for (const char *name : {"craft-melee-weapons.png", "craft-ranged-weapons.png", "craft-magic-weapons.png",
                             "craft-rogue-weapons.png", "craft-heavy-armor.png", "craft-medium-armor.png",
                             "craft-light-armor.png", "craft-rogue-armor.png", "craft-amulets.png", "craft-cooking.png",
                             "craft-alchemy.png", "inventory-ingredients.png"}) {
        plasma::Texture icon;
        const auto bytes = read(name);
        if (!bytes || !decodePng(*bytes, icon))
            continue;
        icon.name = name;
        for (plasma::Document &doc : craftingTabs)
            craftingIcons[name] = widgets::addTexture(doc, icon);
    }

    for (size_t i = 0; i < menuButtons.size() && i < 6; i++) {
        plasma::Document &doc = menuButtons[i];
        const auto bytes = read(MENU_ICONS[i]);
        const plasma::Node &root = doc.nodes[0];
        if (!bytes || root.shape < 0 || doc.shapes[root.shape].kind != plasma::Shape::Kind::Mesh)
            continue;
        plasma::Texture icon;
        if (!decodePng(*bytes, icon))
            continue;
        // The body's texture becomes the icon (shape offset 0x7ac).
        icon.id = 1;
        for (const plasma::Texture &t : doc.textures)
            icon.id = std::max(icon.id, t.id + 1);
        icon.name = MENU_ICONS[i];
        doc.textures.push_back(std::move(icon));
        doc.shapes[root.shape].mesh.texture.value = doc.textures.back().id;
    }
}

// §7: button i of the menu row has its pivot at (W/2 − 150 + 60·i, H − 140); its body and hit
// rectangle are the 48 × 48 square from pivot − 25 to pivot + 23.
glm::vec2 Interface::menuButtonAt(glm::vec2 window, int i) { return {window.x / 2 - 150 + 60.f * i, window.y - 140}; }

std::optional<int> Interface::menuButtonUnder(glm::vec2 window, glm::vec2 mouse) const {
    if (!menuRow)
        return std::nullopt;
    for (int i = 0; i < 6; i++) {
        const glm::vec2 p = menuButtonAt(window, i);
        if (mouse.x >= p.x - 25 && mouse.x < p.x + 23 && mouse.y >= p.y - 25 && mouse.y < p.y + 23)
            return i;
    }
    return std::nullopt;
}

// §5.3, §5.8: the player's bars and the crosshair, laid out afresh every frame.
void Interface::drawHud(Painter &painter, glm::vec2 W, const HudState &s) {
    auto show = [&](Bar &b, glm::vec2 at, std::initializer_list<float> fractions) {
        if (!b.doc)
            return;
        anchor(*b.doc, at);
        size_t i = 0;
        for (float f : fractions)
            if (i < b.fills.size()) {
                fill(*b.doc, b.fills[i].first, b.fills[i].second, f);
                i++;
            }
        painter.draw(*b.doc, 0, Transform(1));
    };
    // §5.4: the bars over creatures, under the player's own.
    for (const HudState::CreatureBar &cb : s.creatureBars) {
        Bar &b = creatureBars[(int)cb.style];
        if (!b.doc)
            continue;
        setText(*b.doc, "info", cb.info);
        setText(*b.doc, "rarename", cb.rareName);
        for (plasma::Node &n : b.doc->nodes)
            if (n.shape >= 0 && b.doc->shapes[n.shape].kind == plasma::Shape::Kind::Text && n.display >= 0) {
                if (n.name == "info")
                    b.doc->displays[n.display].fillColor.value = cb.infoColour;
                else if (n.name == "rarename")
                    b.doc->displays[n.display].fillColor.value = cb.rareColour;
            }
        show(b, cb.at, {cb.fraction, 1 - cb.fraction});
    }

    drawAbilityBar(painter, W, s);

    static const char *const CLASSES[5] = {"", " Warrior", " Ranger", " Mage", " Rogue"};
    const float life = s.maxHealth > 0 ? s.health / s.maxHealth : 0;
    if (lifebar.doc) {
        setText(*lifebar.doc, "text", numbers(s.health, s.maxHealth));
        setText(*lifebar.doc, "info", utf16(s.name));
        const int c = s.characterClass >= 1 && s.characterClass <= 4 ? s.characterClass : 0;
        setText(*lifebar.doc, "class", utf16("LVL " + std::to_string(s.level) + CLASSES[c]));
    }
    show(lifebar, {100, 30}, {life});
    if (experiencebar.doc)
        setText(*experiencebar.doc, "text", numbers((float)s.experience, (float)s.experienceNeeded));
    show(experiencebar, {100, 75},
         {std::min(1.f, s.experienceNeeded > 0 ? (float)s.experience / (float)s.experienceNeeded : 0.f)});
    // §5.3: the pet's life and experience bars and the riding bar, while the player has a pet.
    if (s.hasPet) {
        if (petLifebar.doc) {
            setText(*petLifebar.doc, "text", numbers(s.petHealth, s.petMaxHealth));
            setText(*petLifebar.doc, "info", utf16(s.petName));
            setText(*petLifebar.doc, "class", utf16("LVL " + std::to_string(s.petLevel) + " " + s.petSpecies));
        }
        show(petLifebar, {100, 180}, {s.petMaxHealth > 0 ? s.petHealth / s.petMaxHealth : 0});
        if (petExperiencebar.doc)
            setText(*petExperiencebar.doc, "text", numbers((float)s.petExperience, (float)s.petExperienceNeeded));
        show(petExperiencebar, {100, 225},
             {std::min(1.f, s.petExperienceNeeded > 0 ? (float)s.petExperience / (float)s.petExperienceNeeded : 0.f)});
        if (ridingbar) {
            const int32_t k = std::min(5, (int32_t)(s.ridingEnergy * 5 + 0.9999f));
            int32_t seen = 0;
            for (const plasma::Node &node : ridingbar->nodes)
                if (node.name == "icon" && node.display >= 0)
                    ridingbar->displays[node.display].visibility.value = seen++ < k ? 1 : 0;
            anchor(*ridingbar, {100, 290});
            painter.draw(*ridingbar, 0, Transform(1));
        }
    }
    if (mpbar.doc)
        setText(*mpbar.doc, "text", utf16(std::to_string((int)(s.mana * 100 + 0.5f)) + "/100"));
    show(mpbar, {W.x / 2 + 8, W.y - 90}, {s.mana, s.charge});
    if (hpbar.doc)
        setText(*hpbar.doc, "text", numbers(s.health, s.maxHealth));
    show(hpbar, {W.x / 2 - 163, W.y - 90}, {life});
    if (s.stamina < 1)
        show(staminabar, {W.x / 2, W.y - 115}, {s.stamina});
    if (crosshair && !menuRow) {
        setText(*crosshair, "name", s.health <= 0 ? u"[R] Revive" : s.prompt);
        anchor(*crosshair, W * 0.5f);
        painter.draw(*crosshair, 0, Transform(1));
    }
}

std::optional<FloatingText> hitText(float damage, uint8_t kind, bool critical, bool toPlayer, glm::dvec3 at) {
    if (kind == 4 || kind == 6 || (kind == 0 && std::abs(damage) < 0.1f))
        return std::nullopt;
    FloatingText t;
    t.anchor = at + glm::dvec3(0, 0, 2); // about two blocks up (*inferred*, M-3)
    const float a = std::abs(damage);
    switch (kind) {
    case 1: t.text = u"BLOCK"; break;
    case 3: t.text = u"MISS"; break;
    case 5: t.text = u"ABSORB"; break;
    default:
        if (a < 1) {
            char s[16];
            std::snprintf(s, sizeof s, "%.1f", (float)(int)(a * 10) * 0.1f);
            t.text = utf16(s);
        } else {
            t.text = utf16(std::to_string((int64_t)a));
        }
        break;
    }
    if (damage < 0)
        t.colour = {0.4f, 1, 0.5f, 1};
    else if (toPlayer)
        t.colour = {1, 0.3f, 0.3f, 1};
    t.size = critical ? 60.f : 20.f;
    t.rise = toPlayer ? 100.f : critical ? -200.f : -100.f;
    return t;
}

FloatingText levelUpText(glm::dvec3 player) {
    FloatingText t;
    t.anchor = player;
    t.text = u"LEVEL UP!";
    t.colour = {0.8f, 0.6f, 1, 1};
    t.size = 50;
    t.offset = {0, -250};
    t.lifeMs = 3000;
    t.flags = 2;
    return t;
}

FloatingText experienceText(int32_t gain, glm::dvec3 player) {
    FloatingText t;
    t.anchor = player;
    t.text = utf16("XP +" + std::to_string(gain));
    t.colour = {0.8f, 0.6f, 1, 1};
    t.size = 30;
    t.offset = {0, -200};
    t.lifeMs = 3000;
    t.flags = 2;
    return t;
}

// §5.6: each text drawn outlined at its age, then aged; old ones go.
void Interface::drawFloating(Painter &painter, float dtMs) {
    for (FloatingText &t : floating) {
        const float age = std::min(t.ageMs / t.lifeMs, 1.f);
        if (const auto p = project ? project(t.anchor) : std::nullopt) {
            TextStyle style;
            style.pixelSize = t.size;
            style.flags = t.flags;
            style.colour = t.colour * glm::vec4(1, 1, 1, 1 - age);
            style.outline = t.size * 0.125f;
            style.outlineColour = t.colour * glm::vec4(0.1f, 0.1f, 0.1f, 1 - age);
            painter.text(t.text, *p + t.offset + glm::vec2(0, t.rise * age), style);
        }
        t.ageMs += dtMs;
    }
    floating.erase(std::remove_if(floating.begin(), floating.end(), [](const FloatingText &t) { return t.ageMs > t.lifeMs; }),
                   floating.end());
}

// §5.5: the buttons in a row from (W/2 - 162, H - 50), 48 px apart.
void Interface::drawAbilityBar(Painter &painter, glm::vec2 W, const HudState &s) {
    auto frameColour = [](plasma::Document &doc, bool held) {
        if (const int32_t f = doc.find("frame"); f >= 0 && doc.nodes[f].display >= 0)
            doc.displays[doc.nodes[f].display].fillColor.value = held ? glm::vec4(1, 0, 0.2f, 1) : glm::vec4(1);
    };
    for (size_t i = 0; i < abilityButtons.size() && i < 6; i++) {
        plasma::Document &doc = abilityButtons[i];
        const HudState::AbilitySlot &slot = s.abilities[i];
        // The icon, grey when the ability cannot be used now.
        const auto icon = abilityIcons.find(slot.id);
        const auto fallback = abilityIcons.find(0);
        const auto chosen = icon != abilityIcons.end() ? icon : fallback;
        const plasma::Node &root = doc.nodes[0];
        if (chosen != abilityIcons.end() && root.shape >= 0 && doc.shapes[root.shape].kind == plasma::Shape::Kind::Mesh)
            doc.shapes[root.shape].mesh.texture.value = slot.usable ? chosen->second.first : chosen->second.second;
        // The cooldown overlay, scaled up from its bottom edge.
        if (const int32_t c = doc.find("cooldown"); c >= 0 && doc.nodes[c].transformation >= 0) {
            glm::mat4 d = cooldownRest;
            d[1] *= std::clamp(slot.cooldown, 0.f, 1.f);
            doc.transformations[doc.nodes[c].transformation].deformation.value = d;
        }
        frameColour(doc, slot.held && i != 5);
        anchor(doc, {W.x / 2 - 162 + 48.f * (float)i, W.y - 50});
        painter.draw(doc, 0, Transform(1));
    }
    if (quickItemButton) {
        setText(*quickItemButton, "count", s.quickCount);
        frameColour(*quickItemButton, s.quickHeld);
        anchor(*quickItemButton, {W.x / 2 - 162 + 48.f * 6, W.y - 50});
        painter.draw(*quickItemButton, 0, Transform(1));
    }
}

namespace {

TextStyle chatStyle() {
    TextStyle style;
    style.pixelSize = 10;
    style.outline = 2;
    return style;
}

} // namespace

void Interface::chatAdd(const std::string &text, glm::vec3 colour) {
    Painter measure(fonts, atlas);
    const TextStyle style = chatStyle();
    if (chatLines.empty())
        chatLines.emplace_back();
    auto lineWidth = [&](const std::vector<ChatWord> &line) {
        float w = 0;
        for (const ChatWord &word : line)
            w += word.text == u" " ? 5.f : measure.textWidth(word.text, style);
        return w;
    };
    const glm::vec4 c(colour / 255.f, 1);
    auto addWord = [&](const std::u16string &word) {
        if (word.empty())
            return;
        if (word != u" " && lineWidth(chatLines.back()) + measure.textWidth(word, style) >= 380)
            chatLines.emplace_back();
        chatLines.back().push_back({word, c});
    };
    std::string word;
    for (char ch : text) {
        if (ch == ' ' || ch == '\n' || ch == '\r') {
            addWord(utf16(word));
            word.clear();
            if (ch == ' ')
                addWord(u" ");
            else
                chatLines.emplace_back();
        } else {
            word += ch;
        }
    }
    addWord(utf16(word));
}

// §9.1: the last ten lines from (20, H - 220), and the input line under them.
void Interface::drawChat(Painter &painter, glm::vec2 W, double seconds) {
    while (chatLines.size() > 10)
        chatLines.erase(chatLines.begin());
    const glm::vec2 box(20, W.y - 220);
    TextStyle style = chatStyle();
    for (size_t k = 0; k < chatLines.size(); k++) {
        float x = 3;
        for (const ChatWord &word : chatLines[k]) {
            if (word.text == u" ") {
                x += 5;
                continue;
            }
            style.colour = word.colour;
            painter.text(word.text, box + glm::vec2(x, 19 + 18.f * (float)k), style);
            x += painter.textWidth(word.text, style);
        }
    }
    style.colour = glm::vec4(1);
    painter.text(chatInput, box + glm::vec2(3, 188), style);
    if (chatOpen && ((int64_t)(seconds * 1000) / 500) % 2 == 1)
        painter.text(u"|", box + glm::vec2(painter.textWidth(chatInput, style) + 3, 188), style);
}

void Interface::showLandName(const std::string &name, const std::string &detail) {
    // §5.9.1: a change is taken up only once the running banner is over.
    if ((name == landNameShown && detail == landDetailShown) || landNameMs > 0)
        return;
    landNameShown = name;
    landDetailShown = detail;
    landNameMs = 8000;
    if (landName) {
        setText(*landName, "name", utf16(name));
        setText(*landName, "detail", utf16(detail));
    }
}

void Interface::setObjective(const std::u16string &text) {
    if (smallQuestTag)
        setText(*smallQuestTag, "objective", text);
    questShowMs = 0;
}

void Interface::questComplete(const std::u16string &objective) {
    if (!questBanner)
        return;
    setText(*questBanner, "objective", objective);
    questBannerMs = 0;
}

// The banner shows while its `shine` sequence runs (open point Q-1: its length is the file's;
// 4 s when the file gives none).
void Interface::drawQuestBanner(Painter &painter, glm::vec2 W, float dtMs) {
    if (!questBanner || questBannerMs < 0)
        return;
    questBannerMs += dtMs;
    if (questBannerMs > (questBannerLength > 0 ? questBannerLength : 4000.f)) {
        questBannerMs = -1;
        return;
    }
    plasma::play(*questBanner, 0, "shine", questBannerMs);
    anchor(*questBanner, {W.x / 2, W.y - 300});
    painter.draw(*questBanner, 0, Transform(1));
}

void Interface::drawQuestTag(Painter &painter, glm::vec2 W, float dtMs) {
    if (!smallQuestTag || !questTagShown)
        return;
    plasma::Document &doc = *smallQuestTag;
    questShowMs += dtMs;
    plasma::play(doc, 0, "show", questShowMs);
    widgets::setVisible(doc, "questbar", questBar);
    if (questBar && questBarFill >= 0)
        fill(doc, questBarFill, questBarRest, std::clamp(questFill, 0.f, 1.f));
    anchor(doc, {W.x - 200, 400});
    painter.draw(doc, 0, Transform(1));
}

// §5.9: fades in over four seconds and out over the next four, at (W/2, 230).
void Interface::drawLandName(Painter &painter, glm::vec2 window, float dtMs) {
    landNameMs = std::max(0.f, landNameMs - dtMs);
    if (!landName || landNameMs <= 0)
        return;
    const float u = 2 * (landNameMs / 8000 - 0.5f);
    plasma::Document &doc = *landName;
    if (doc.nodes[0].display >= 0)
        doc.displays[doc.nodes[0].display].fillColor.value.a = 1 - u * u;
    anchor(doc, {window.x / 2, 230});
    painter.draw(doc, 0, Transform(1));
}

void Interface::setOptions(const GameOptions &current) {
    optionsCopy = current;
    // §6.5: the entry of the current resolution, or the first (which then replaces it).
    resolutionIndex = 0;
    for (size_t i = 0; i < resolutions.size(); i++)
        if (resolutions[i] == glm::ivec2(current[GameOptions::RESOLUTION_X], current[GameOptions::RESOLUTION_Y]))
            resolutionIndex = i;
    if (!resolutions.empty()) {
        optionsCopy[GameOptions::RESOLUTION_X] = resolutions[resolutionIndex].x;
        optionsCopy[GameOptions::RESOLUTION_Y] = resolutions[resolutionIndex].y;
    }
}

namespace {

const char *const OPTION_LABELS[11] = {"Mode", "Resolution", "Anti-aliasing", "Render Distance", "Sound FX Volume",
                                       "Music Volume", "Camera Speed", "Camera Smoothness", "Invert Y Axis",
                                       "FPS Limit", "Language"};

glm::vec2 optionsOrigin(glm::vec2 W) { return (W - glm::vec2(450, 440)) * 0.5f; }
glm::vec2 leftArrowAt(int k) { return {240, 12 + 30.f * (float)k}; }
glm::vec2 rightArrowAt(int k) { return {416, 12 + 30.f * (float)k}; }
glm::vec2 optionsButtonAt(int i) { return {i == 0 ? 20.f : i == 1 ? 170.f : 320.f, 405}; }
bool inBox(glm::vec2 p, glm::vec2 at, glm::vec2 size) {
    return p.x >= at.x && p.y >= at.y && p.x < at.x + size.x && p.y < at.y + size.y;
}

} // namespace

// §6.2: a row's value as the window shows it.
static std::string optionText(const GameOptions &o, int k) {
    using F = GameOptions;
    switch (k) {
    case 0: return o[F::FULLSCREEN] ? "Fullscreen" : "Windowed";
    case 1: return std::to_string(o[F::RESOLUTION_X]) + " x " + std::to_string(o[F::RESOLUTION_Y]);
    case 2: return o[F::ANTI_ALIASING] < 1 ? "Disabled" : std::to_string(2 * o[F::ANTI_ALIASING]) + "x";
    case 3: return std::to_string(o[F::RENDER_DISTANCE]) + "%";
    case 4: return std::to_string(o[F::SOUND_VOLUME]) + "%";
    case 5: return std::to_string(o[F::MUSIC_VOLUME]) + "%";
    case 6: return std::to_string(o[F::CAMERA_SPEED]) + "%";
    case 7: return std::to_string(o[F::CAMERA_SMOOTHNESS]) + "%";
    case 8: return o[F::INVERT_Y] ? "Yes" : "No";
    case 9: return o[F::MIN_TIME_STEP] < 1 ? "none" : std::to_string(1000 / o[F::MIN_TIME_STEP]) + " fps";
    default: return std::to_string(o[F::LANGUAGE]);
    }
}

void Interface::drawOptions(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 O = optionsOrigin(W);
    if (optionsPanel) {
        placeWidget(*optionsPanel, 0, O);
        painter.draw(*optionsPanel, 0, Transform(1));
    }
    const glm::vec2 local = mouse - O;
    TextStyle style;
    style.pixelSize = 12;
    style.outline = 3;
    for (int k = 0; k < 11; k++) {
        const float baseline = 27 + 30.f * (float)k;
        style.flags = 0;
        painter.text(utf16(OPTION_LABELS[k]), O + glm::vec2(15, baseline), style);
        style.flags = plasma::CENTRE;
        if (k != 1 || !resolutions.empty())
            painter.text(utf16(optionText(optionsCopy, k)), O + glm::vec2(340, baseline), style);
        // §6.3: the arrows, cyan under the cursor.
        for (auto [doc, at] : {std::pair{&leftArrow, leftArrowAt(k)}, std::pair{&rightArrow, rightArrowAt(k)}}) {
            if (!*doc)
                continue;
            plasma::Document &d = **doc;
            if (d.nodes[0].display >= 0)
                d.displays[d.nodes[0].display].fillColor.value =
                    inBox(local, at, {19, 21}) ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
            placeWidget(d, 0, O + at);
            painter.draw(d, 0, Transform(1));
        }
    }
    for (size_t i = 0; i < optionsButtons.size(); i++) {
        placeWidget(optionsButtons[i], 0, O + optionsButtonAt((int)i));
        painter.draw(optionsButtons[i], 0, Transform(1));
    }
}

// §6.4: a left press in the window; arrows edit the copy, buttons answer.
std::optional<Action> Interface::clickOptions(glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 local = mouse - optionsOrigin(W);
    for (int i = 0; i < 3; i++)
        if (inBox(local, optionsButtonAt(i), {110, 15}))
            return i == 0 ? Action::OptionsApply : i == 1 ? Action::OptionsOk : Action::OptionsCancel;
    using F = GameOptions;
    for (int k = 0; k < 11; k++) {
        const bool left = inBox(local, leftArrowAt(k), {19, 21}), right = inBox(local, rightArrowAt(k), {19, 21});
        if (!left && !right)
            continue;
        const int step = left ? -1 : 1;
        auto stepIn = [&](F::Field f, int by, int lo, int hi) {
            int32_t &v = optionsCopy[f];
            v += by * step;
            if (left && v < lo)
                v = lo;
            if (right && v > hi)
                v = hi;
        };
        switch (k) {
        case 0: optionsCopy[F::FULLSCREEN] = (optionsCopy[F::FULLSCREEN] + 1) % 2; break;
        case 1:
            if (!resolutions.empty()) {
                if (left && resolutionIndex > 0)
                    resolutionIndex--;
                if (right && resolutionIndex + 1 < resolutions.size())
                    resolutionIndex++;
                optionsCopy[F::RESOLUTION_X] = resolutions[resolutionIndex % resolutions.size()].x;
                optionsCopy[F::RESOLUTION_Y] = resolutions[resolutionIndex % resolutions.size()].y;
            }
            break;
        case 2: stepIn(F::ANTI_ALIASING, 1, 0, 2); break;
        case 3: stepIn(F::RENDER_DISTANCE, 5, 25, 100); break;
        case 4: stepIn(F::SOUND_VOLUME, 10, 0, 100); break;
        case 5: stepIn(F::MUSIC_VOLUME, 10, 0, 100); break;
        case 6: stepIn(F::CAMERA_SPEED, 5, 10, 100); break;
        case 7: stepIn(F::CAMERA_SMOOTHNESS, 5, 0, 100); break;
        case 8: optionsCopy[F::INVERT_Y] = optionsCopy[F::INVERT_Y] ? 0 : 1; break;
        case 9: {
            // The left arrow lowers the frame rate: one more millisecond per frame.
            int32_t &v = optionsCopy[F::MIN_TIME_STEP];
            v = left ? std::min(v + 1, 50) : std::max(v - 1, 0);
            break;
        }
        default: optionsCopy[F::LANGUAGE] += step; break;
        }
        return Action::None;
    }
    return Action::None;
}

// §3.2, §3.3: a 200 x 100 container below the centred logo.
Interface::Menu Interface::startMenu(glm::vec2 window) const {
    Menu m;
    m.items = {{"Start Game", 30, Action::StartGame}, {"Options", 80, Action::Options}, {"Exit", 130, Action::Exit}};
    m.pixelSize = 18;
    m.outline = 4;
    m.size = {200, 100};
    m.origin = {(window.x - m.size.x) / 2, window.y / 2 + 50};
    return m;
}

// §7, §3.3: on the centred 150 x 120 system panel.
Interface::Menu Interface::systemMenuLayout(glm::vec2 window) const {
    Menu m;
    m.items = {{"Options", 30, Action::Options}, {"Start Menu", 60, Action::StartMenu}, {"Exit Game", 90, Action::Exit}};
    m.pixelSize = 14;
    m.outline = 3;
    m.size = {150, 120};
    m.origin = (window - m.size) * 0.5f;
    return m;
}

// §3.2: the hover band of an item is the 30 pixels ending 10 below its baseline, inside the
// widget's width.
std::optional<size_t> Interface::hovered(const Menu &menu, glm::vec2 mouse) const {
    const glm::vec2 local = mouse - menu.origin;
    if (local.x < 0 || local.x >= menu.size.x)
        return std::nullopt;
    for (size_t i = 0; i < menu.items.size(); i++)
        if (local.y >= menu.items[i].baseline - 20 && local.y < menu.items[i].baseline + 10)
            return i;
    return std::nullopt;
}

void Interface::drawMenu(Painter &painter, const Menu &menu, glm::vec2 mouse) {
    const auto over = hovered(menu, mouse);
    for (size_t i = 0; i < menu.items.size(); i++) {
        TextStyle style;
        style.pixelSize = menu.pixelSize;
        style.flags = plasma::CENTRE;
        style.outline = menu.outline;
        style.colour = over == i ? HOVER : glm::vec4(1);
        painter.text(utf16(menu.items[i].label), menu.origin + glm::vec2(menu.size.x / 2, menu.items[i].baseline), style);
    }
}

// Quests: one of the three icons, its foot at `foot`. The picture's green background is keyed
// out by hue (green above 1.3 x the larger of red and blue).
void Interface::drawQuestIcon(Painter &painter, QuestIcon icon, glm::vec2 foot, float scale) {
    if (!questPicturesLoaded) {
        questPicturesLoaded = true;
        int w = 0, h = 0, channels = 0;
        stbi_uc *px = stbi_load_from_memory(QUEST_ICONS_PNG, (int)sizeof QUEST_ICONS_PNG, &w, &h, &channels, 4);
        // Offer "!", under-way grey "?", hand-in orange "?".
        static const int CROPS[3][4] = {{188, 16, 24, 48}, {96, 16, 48, 52}, {16, 16, 48, 52}};
        for (int k = 0; px && k < 3; k++) {
            const int x0 = CROPS[k][0], y0 = CROPS[k][1], cw = CROPS[k][2], ch = CROPS[k][3];
            if (x0 + cw > w || y0 + ch > h)
                continue;
            std::vector<uint8_t> rgba((size_t)cw * ch * 4);
            for (int y = 0; y < ch; y++)
                for (int x = 0; x < cw; x++) {
                    const stbi_uc *s = px + ((size_t)(y0 + y) * w + (size_t)(x0 + x)) * 4;
                    uint8_t *d = &rgba[((size_t)y * cw + (size_t)x) * 4];
                    d[0] = s[0], d[1] = s[1], d[2] = s[2];
                    d[3] = (float)std::max(s[0], s[2]) * 1.3f - (float)s[1] > 0 ? 255 : 0;
                }
            if (atlas.add(cw, ch, rgba.data(), questPictures[(size_t)k].rect))
                questPictures[(size_t)k].size = {(float)cw, (float)ch};
        }
        if (px)
            stbi_image_free(px);
    }
    const QuestPicture &p = questPictures[(size_t)icon];
    if (p.size.x <= 0)
        return;
    const glm::vec2 s = p.size * scale;
    painter.image(p.rect, glm::round(foot - glm::vec2(s.x / 2, s.y)), glm::round(foot + glm::vec2(s.x / 2, 0)));
}

Frame Interface::draw(glm::vec2 size, glm::vec2 mouse, double seconds) {
    Painter painter(fonts, atlas);
    const float dtMs = lastSeconds < 0 ? 0.f : (float)((seconds - lastSeconds) * 1000.0);
    lastSeconds = seconds;
    if (startScreen && !optionsOpen) {
        if (logo) {
            anchor(*logo, size * 0.5f);
            painter.draw(*logo, 0, Transform(1));
        }
        if (picroma) {
            anchor(*picroma, size - glm::vec2(20));
            painter.draw(*picroma, 0, Transform(1));
        }
        drawMenu(painter, startMenu(size), mouse);
    } else if (front != Front::None) {
        drawFront(painter, size, mouse, dtMs, seconds);
    } else if (!startScreen && mapOpen) {
        // §5.2.3 step 7: the compass at (W - 60, 40), turned with the map. (Routine 0x476660
        // is read as not scaling the model to its size: 0.003 units per voxel.)
        if (compass.mesh) {
            const float largest = std::max({compass.size.x, compass.size.y, compass.size.z});
            painter.item(itemDraw(compass.mesh, compass.size, false, {size.x - 60, 40}, {-120, 0, -compassYaw},
                                  0.003f * largest, 0, size));
        }
        for (const auto &[at, icon] : mapQuestMarks) // quests: givers and places
            drawQuestIcon(painter, icon, at, 0.6f);
        // §5.2.5: the map's labels, fill over a 3-pixel black rim, centred.
        for (const MapLabel &l : mapLabels) {
            TextStyle style;
            style.pixelSize = l.size;
            style.flags = plasma::CENTRE;
            style.outline = 3;
            style.colour = l.colour;
            painter.text(l.text, l.at, style);
        }
        // §5.2.6: the cursor label. (Its exact offset from the cursor comes from cursor.plx and
        // is not traced; it is set just above the pointer here.)
        if (!cursorLabel.empty()) {
            TextStyle style;
            style.pixelSize = 12;
            style.flags = plasma::CENTRE;
            style.outline = 3;
            style.colour = glm::vec4(1);
            painter.text(cursorLabel, mouse + glm::vec2(0, -20), style);
        }
    } else if (!startScreen) {
        // Quests: the icons over givers' heads, bobbing a little, and on the corner map.
        const float bob = 3 * (float)std::sin(seconds * 3);
        if (hud)
            for (const auto &[at, icon] : mapQuestMarks)
                drawQuestIcon(painter, icon, at, 0.35f);
        for (const QuestMark &m : questMarks)
            if (const auto p = project ? project(m.at) : std::nullopt)
                drawQuestIcon(painter, m.icon, *p + glm::vec2(0, bob), 0.8f);
        drawFloating(painter, dtMs);
        if (hud)
            drawChat(painter, size, seconds);
        drawLandName(painter, size, dtMs);
        if (hud) {
            drawHud(painter, size, *hud);
            drawQuestTag(painter, size, dtMs);
            drawQuestBanner(painter, size, dtMs);
        }
        drawBubbles(painter, size);
        // §7: the menu row, the hovered icon brightened.
        if (menuRow) {
            const auto over = menuButtonUnder(size, mouse);
            for (int i = 0; i < (int)menuButtons.size(); i++) {
                plasma::Document &doc = menuButtons[i];
                anchor(doc, menuButtonAt(size, i));
                if (doc.nodes[0].display >= 0)
                    doc.displays[doc.nodes[0].display].fillColor.value = glm::vec4(glm::vec3(over == i ? 1.5f : 1.f), 1);
                painter.draw(doc, 0, Transform(1));
            }
        }
    }
    if (craftingOpen)
        skillsOpen = false; // §1.3: they share the spot left of the inventory
    benchOpen = benchOpen && inventoryOpen; // the bench closes with the inventory
    if (benchOpen)
        craftingOpen = skillsOpen = false;
    if (!startScreen && !mapOpen && benchOpen)
        drawBench(painter, size, mouse);
    if (!startScreen && !mapOpen && craftingOpen)
        drawCrafting(painter, size, mouse, dtMs);
    if (!startScreen && !mapOpen && skillsOpen)
        drawSkills(painter, size, mouse);
    vendorOpen = vendorOpen && inventoryOpen; // talk §3.6: these close with the inventory
    identifyOpen = identifyOpen && inventoryOpen;
    adaptOpen = adaptOpen && inventoryOpen;
    if (!startScreen && !mapOpen && (identifyOpen || adaptOpen))
        drawServices(painter, size, mouse);
    if (!startScreen && !mapOpen && vendorOpen) {
        drawVendor(painter, size, mouse);
        if (vendorHovered)
            drawTooltip(painter, size, mouse + glm::vec2(20), *vendorHovered);
    }
    if (!startScreen && !mapOpen && inventoryOpen)
        drawInventory(painter, size, mouse);
    else if (!startScreen && !mapOpen && hud && targetItem && !craftingOpen)
        drawTooltip(painter, size, size - glm::vec2(320), *targetItem, true);
    if (!startScreen && systemMenu) {
        const Menu menu = systemMenuLayout(size);
        if (systemPanel) {
            placeWidget(*systemPanel, 0, menu.origin);
            painter.draw(*systemPanel, 0, Transform(1));
        }
        drawMenu(painter, menu, mouse);
    }
    if (optionsOpen)
        drawOptions(painter, size, mouse);
    return painter.take();
}

void Interface::mapRelease() {
    if (teleportMode && selectedPortal != glm::ivec2(-1) && selectedPortal == hoveredPortal) {
        teleportTo = selectedPortal;
        hoveredPortal = selectedPortal = glm::ivec2(-1);
        mapOpen = false;
    } else {
        selectedPortal = hoveredPortal; // a click off every portal label deselects
    }
}

Action Interface::click(glm::vec2 size, glm::vec2 mouse) {
    if (optionsOpen)
        return *clickOptions(size, mouse);
    if (!startScreen)
        if (const auto i = menuButtonUnder(size, mouse))
            return MENU_ACTIONS[*i];
    const Menu menu = startScreen ? startMenu(size) : systemMenuLayout(size);
    if (!startScreen && !systemMenu)
        return Action::None;
    if (const auto i = hovered(menu, mouse))
        return menu.items[*i].action;
    return Action::None;
}

} // namespace ui
