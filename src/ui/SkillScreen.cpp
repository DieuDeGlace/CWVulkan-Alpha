// The skill window (docs/specs/alpha-skills.md): two specialization buttons and eleven skill
// buttons in four rows, the points line, the cost of a re-skill, Learn and Cancel, and the
// tooltip that explains a skill at its level and the next.
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include "creatures/Abilities.h"
#include "creatures/Combat.h"
#include <cmath>

namespace ui {
namespace {

using namespace widgets;

constexpr glm::vec2 PANEL{300, 460}, BUTTON{42, 42}, SPECIALIZATION{60, 60};
glm::vec2 origin(glm::vec2 W) { return {std::trunc(W.x / 2) - 510, std::trunc(W.y / 2) - 285}; }
// §2.3: the buttons' top-left corners, panel-relative.
glm::vec2 skillCorner(int32_t i) {
    static const glm::vec2 AT[11] = {{50, 210}, {105, 210}, {50, 270}, {105, 270}, {50, 330}, {105, 330},
                                     {50, 150}, {105, 150}, {160, 150}, {215, 150}, {-1000, -1000}};
    return AT[i];
}
glm::vec2 specializationCorner(int s) { return {50.f + 70.f * (float)s, 70}; }
const glm::vec4 CYAN{0, 1, 1, 1}, RED{1, 0.25f, 0.25f, 1};

const char *const CLASSES[5] = {"", "warrior", "ranger", "mage", "rogue"};
const char *const SPECIALIZATIONS[5][2] = {
    {"", ""}, {"berserker", "guardian"}, {"sniper", "scout"}, {"fire", "water"}, {"assassin", "ninja"}};
const char *const SKILL_ICONS[6] = {"pet-taming.png",   "pet-riding.png", "climbing.png",
                                    "hang-gliding.png", "swimming.png",   "ship-driving.png"};
const char *const SKILL_NAMES[11] = {"SkillPetTaming", "SkillPetRiding", "SkillClimbing", "SkillHangGliding",
                                     "SkillSwimming",  "SkillBoatDriving", "", "", "", "SkillAbility4", "SkillAbility5"};

// The icon file and dictionary name of the ability a skill button stands for.
std::pair<const char *, const char *> abilityOf(int32_t id) {
    switch (id) {
    case 54: return {"smash.png", "AbilitySmash"};
    case 86: return {"cyclone.png", "AbilityCyclone"};
    case 102: return {"warfrenzy.png", "AbilityWarFrenzy"};
    case 101: return {"bulwark.png", "AbilityBulwark"};
    case 21: return {"ranger-kick.png", "AbilityRangerKick"};
    case 50: return {"retreat2.png", "AbilityRetreat"};
    case 99: return {"aim.png", "AbilityAim"};
    case 100: return {"swiftness.png", "AbilitySwiftness"};
    case 88: return {"fire-explosion.png", "AbilityFireExplosion"};
    case 34: return {"healing-stream.png", "AbilityHealingStream"};
    case 103: return {"mana-shield.png", "AbilityManaShield"};
    case 49: return {"teleport.png", "AbilityTeleport"};
    case 48: return {"intercept.png", "AbilityIntercept"};
    case 79: return {"sneak.png", "AbilitySneak"};
    case 97: return {"camouflage.png", "AbilityCamouflage"};
    case 96: return {"shuriken.png", "AbilityShuriken"};
    default: return {"", ""};
    }
}

// §5.4: R(x), two decimals, in the stream's default format.
std::string rounded(float x) {
    const float r = std::trunc(std::abs(x) * 100.f + 0.5f) * 0.01f * (x < 0 ? -1.f : 1.f);
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%g", (double)r);
    return buffer;
}

} // namespace

void Interface::loadSkills() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        skillsPanel = clone(gui, n);
        resizeWidget(*skillsPanel, 0, PANEL);
    }
    const int32_t root = gui.find("skillbutton"), child = gui.find("childskillbutton");
    if (root >= 0 && child >= 0)
        for (int32_t i = 0; i < 11; i++)
            skillButtons.push_back(clone(gui, i == 0 || i == 2 || i == 4 || i == 6 ? root : child));
    if (const int32_t n = gui.find("specializationbutton"); n >= 0)
        for (int s = 0; s < 2; s++)
            specializationButtons.push_back(clone(gui, n));
}

void Interface::toggleSkills() {
    skillsOpen = !skillsOpen;
    if (skillsOpen) {
        skillDraft = playerSkills;
        draftSpecialization = playerSpecialization;
        craftingOpen = false;
    }
}

void Interface::drawSkills(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 O = origin(W), local = mouse - O;
    if (skillsPanel) {
        placeWidget(*skillsPanel, 0, O);
        painter.draw(*skillsPanel, 0, Transform(1));
    }
    const int cls = sheet.characterClass;
    const auto setIcon = [](plasma::Document &doc, const std::map<std::string, std::pair<int32_t, int32_t>> &ids,
                            const std::string &file, bool colour) {
        const plasma::Node &root = doc.nodes[0];
        if (root.shape < 0 || doc.shapes[root.shape].kind != plasma::Shape::Kind::Mesh)
            return;
        const auto it = ids.find(file);
        doc.shapes[root.shape].mesh.texture.value = it == ids.end() ? -1 : colour ? it->second.first : it->second.second;
    };
    std::optional<std::pair<int32_t, int32_t>> hovered; // (skill or -1, specialization or -1)
    // §2.4: the specialization buttons, the chosen one in colour.
    for (int s = 0; s < (int)specializationButtons.size(); s++) {
        plasma::Document &doc = specializationButtons[s];
        if (cls >= 1 && cls <= 4)
            setIcon(doc, specializationIcons[s], std::string(CLASSES[cls]) + "-" + SPECIALIZATIONS[cls][s] + ".png",
                    s == draftSpecialization);
        const bool over = inside(local, specializationCorner(s), BUTTON);
        if (over)
            hovered = std::pair{-1, s};
        tint(doc, doc.find("frame"), over ? CYAN : glm::vec4(1));
        anchor(doc, O + specializationCorner(s));
        painter.draw(doc, 0, Transform(1));
    }
    // The skill buttons: the draft level on each, locked children grey.
    for (int32_t i = 0; i < (int32_t)skillButtons.size() && i < 10; i++) {
        plasma::Document &doc = skillButtons[i];
        const std::string icon = i >= 6 && i <= 8 ? abilityOf(creatures::skillAbility(i, cls, draftSpecialization)).first
                                 : i < 6        ? SKILL_ICONS[i]
                                                : "";
        setIcon(doc, skillButtonIcons[i], icon, !creatures::skillLocked(skillDraft, i));
        setLabel(doc, "info", number(skillDraft[i]));
        const bool over = inside(local, skillCorner(i), BUTTON);
        if (over)
            hovered = std::pair{i, -1};
        tint(doc, doc.find("frame"), over ? CYAN : glm::vec4(1));
        anchor(doc, O + skillCorner(i));
        painter.draw(doc, 0, Transform(1));
    }
    // §5.1: the texts.
    outlined(painter, u"Skills", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    outlined(painter,
             u"Points: " + number(creatures::pointsUsed(skillDraft)) + u"/" + number(2 * sheet.level - 2),
             O + glm::vec2(15, 45), 10, 2, glm::vec4(1));
    const bool differs = skillDraft != playerSkills || draftSpecialization != playerSpecialization;
    const int32_t cost = creatures::skillChangeCost(sheet.level, playerSkills, playerSpecialization, skillDraft,
                                                    draftSpecialization);
    const uint32_t money = inventory ? inventory->money : 0;
    const bool trainer = cost == 0 || (trainerNear && trainerNear());
    if (cost > 0) {
        if ((uint32_t)cost > money)
            outlined(painter, u"Not enough money.", O + glm::vec2(15, PANEL.y - 65), 10, 2, RED);
        else if (!trainer)
            outlined(painter, u"Requires class trainer.", O + glm::vec2(15, PANEL.y - 65), 10, 2, RED);
        outlined(painter, u"COST:", O + glm::vec2(15, 415), 10, 2, glm::vec4(1));
        outlined(painter, number(cost / 10000) + u" G ", O + glm::vec2(140, 415), 10, 2, {1, 0.9f, 0, 1}, plasma::RIGHT);
        outlined(painter, number(cost / 100 % 100) + u" S ", O + glm::vec2(180, 415), 10, 2, {0.7f, 0.7f, 0.7f, 1},
                 plasma::RIGHT);
        outlined(painter, number(cost % 100) + u" C ", O + glm::vec2(220, 415), 10, 2, {0.8f, 0.5f, 0, 1}, plasma::RIGHT);
    }
    if (differs) {
        const bool allowed = (uint32_t)cost <= money && trainer;
        const bool overLearn = local.x > 0 && local.x < PANEL.x / 2 && local.y > PANEL.y - 30 && local.y < PANEL.y;
        const bool overCancel = local.x > PANEL.x / 2 && local.x < PANEL.x && local.y > PANEL.y - 30 && local.y < PANEL.y;
        outlined(painter, u"Learn", O + glm::vec2(PANEL.x / 3, PANEL.y - 20), 12, 3,
                 !allowed ? glm::vec4(0.5f, 0.5f, 0.5f, 1) : overLearn ? CYAN : glm::vec4(1), plasma::CENTRE);
        outlined(painter, u"Cancel", O + glm::vec2(2 * PANEL.x / 3, PANEL.y - 20), 12, 3, overCancel ? CYAN : glm::vec4(1),
                 plasma::CENTRE);
    }
    if (hovered)
        drawSkillTooltip(painter, W, mouse + glm::vec2(20), hovered->first, hovered->second);
}

void Interface::drawSkillTooltip(Painter &painter, glm::vec2, glm::vec2 X, int32_t skill, int32_t specialization) {
    if (!dictionary)
        return;
    if (preview) {
        for (int k = 1; k <= 4; k++)
            setVisible(*preview, ("star" + std::to_string(k)).c_str(), false);
        anchor(*preview, X);
        painter.draw(*preview, 0, Transform(1));
    }
    const int cls = sheet.characterClass;
    // §5.4: the variables at level n.
    const auto variables = [&](int32_t n) {
        std::map<std::string, std::string> v;
        const float vn = n < 1 ? 0.f : 1.f - 1.f / (0.1f * (float)n + 1.f);
        const float kn = 1.f - 1.f / (0.1f * (float)(n + 1) + 1.f);
        v["@level"] = std::to_string(n);
        v["@health"] = rounded(50 * vn) + "%";
        v["@ridingspeed"] = rounded(100 * (1 + vn)) + "%";
        v["@climbingpower"] = rounded(100 * kn) + "%";
        v["@flyingspeed"] = (n < 1 ? std::string("0") : rounded(100 * (3 * vn + 1.5f))) + "%";
        v["@swimmingspeed"] = (n < 1 ? std::string("50") : rounded(100 * (0.5f * vn + 0.5f))) + "%";
        v["@boatspeed"] = (n < 1 ? std::string("0") : rounded(100 * (3 * vn + 1))) + "%";
        creatures::Combatant c;
        c.characterClass = (uint8_t)cls;
        c.specialization = (uint8_t)draftSpecialization;
        c.skills.fill(n);
        const std::pair<const char *, int32_t> COOLDOWNS[] = {
            {"smash", 54},        {"cyclone", 86},  {"bulwark", 101},      {"warfrenzy", 102},   {"rangerkick", 21},
            {"retreat", 50},      {"aim", 99},      {"swiftness", 100},    {"fireexplosion", 88}, {"manashield", 103},
            {"teleport", 49},     {"intercept", 48}, {"shuriken", 96},     {"camouflage", 97},   {"sneak", 79}};
        for (const auto &[name, id] : COOLDOWNS)
            v[std::string("@") + name + "cooldown"] = rounded((float)creatures::cooldownLength(c, id) / 1000.f);
        v["@cycloneduration"] = "5";
        v["@bulwarkduration"] = v["@warfrenzyduration"] = v["@swiftnessduration"] = "10";
        v["@manashieldduration"] = "30";
        v["@bulwarkamount"] = rounded(100 * (0.3f * creatures::abilityValue(c, 101) + 0.25f)) + "%";
        v["@warfrenzyamount"] = rounded(100 * (9 * creatures::abilityValue(c, 102) + 1)) + "%";
        for (const char *name : {"@rangerkickknockback", "@retreatdistance", "@aimstealth", "@swiftnessamount",
                                 "@fireexplosionknockback"})
            v[name] = rounded(100 * vn) + "%";
        v["@manashieldpower"] = rounded(100 * creatures::abilityValue(c, 103)) + "%";
        v["@healingstreamcost"] = rounded(100 * (1 - 0.75f * creatures::abilityValue(c, 34)));
        creatures::Combatant committed = c; // (the camouflage duration reads the committed level)
        for (size_t i = 0; i < playerSkills.size(); i++)
            committed.skills[i] = playerSkills[i];
        v["@camouflageduration"] = rounded((12000 * creatures::abilityValue(committed, 97) + 8000) / 1000);
        v["@sneakspeed"] = rounded(100 * 0.4f * creatures::abilityValue(c, 79)) + "%";
        v["@sneakstealth"] = rounded(100 * creatures::abilityValue(c, 79)) + "%";
        return v;
    };
    // §5.3: the entries rendered into pages, each page a paragraph.
    std::vector<SpeechPage> pages;
    const auto render = [&](const std::string &key, const std::map<std::string, std::string> &vars) {
        if (const DictionaryEntry *e = dictionary->find("speech", key)) {
            const auto more = SpeechText(e->text).render({}, vars);
            pages.insert(pages.end(), more.begin(), more.end());
        }
    };
    if (specialization >= 0) {
        if (cls >= 1 && cls <= 4)
            render(std::string("specialization:") + CLASSES[cls] + ":" + SPECIALIZATIONS[cls][specialization], {});
    } else {
        const int32_t n = skillDraft[skill];
        const std::string name =
            skill >= 6 && skill <= 8 ? abilityOf(creatures::skillAbility(skill, cls, draftSpecialization)).second : SKILL_NAMES[skill];
        const auto now = variables(n);
        render("description:" + name, now);
        if (n > 0) {
            render("skill:level", now);
            render("details:" + name, now);
        }
        const auto next = variables(n + 1);
        render("skill:nextlevel", next);
        render("details:" + name, next);
    }
    // The word layout: 12 px for the first page, 10 px after, wrapped at 275.
    glm::vec2 at = X + glm::vec2(14, 25);
    for (size_t p = 0; p < pages.size(); p++) {
        const float size = p == 0 ? 12.f : 10.f;
        TextStyle style;
        style.pixelSize = size;
        style.outline = 3;
        float x = 0;
        bool glue = true;
        for (const SpeechSegment &segment : pages[p]) {
            const std::u16string word = utf16(segment.text);
            const float d = painter.textWidth(word, style);
            const bool punct = word == u"." || word == u"-" || word == u"," || word == u";" || word == u"!" || word == u"?" ||
                               word.find_first_of(u":/)") != std::u16string::npos;
            if (!glue)
                x += punct ? 3 : size;
            glue = word == u"-";
            if (!punct && x + d > 275)
                x = 0, at.y += size + 4;
            style.colour = segment.colour;
            painter.text(word, at + glm::vec2(x, 0), style);
            x += d;
            if (word == u"/" || word == u"(")
                x -= 9;
        }
        at.y += 18;
    }
}

bool Interface::skillsPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (!skillsOpen)
        return false;
    const glm::vec2 local = mouse - origin(W);
    for (int s = 0; s < 2; s++)
        if (button == 0 && inside(local, specializationCorner(s), BUTTON)) {
            draftSpecialization = s;
            inventorySounds.push_back(0x55);
            return true;
        }
    for (int32_t i = 0; i < 10; i++)
        if (inside(local, skillCorner(i), BUTTON)) {
            if (button == 0) {
                if (creatures::raiseSkill(skillDraft, i, creatures::skillPoints(sheet.level, manaCubes)))
                    inventorySounds.push_back(0x55);
            } else if (creatures::lowerSkill(skillDraft, i)) {
                inventorySounds.push_back(0x58);
            }
            return true;
        }
    if (!inside(local, {0, 0}, PANEL))
        return false;
    const bool differs = skillDraft != playerSkills || draftSpecialization != playerSpecialization;
    if (button == 0 && differs && local.y > PANEL.y - 30 && local.y < PANEL.y) {
        if (local.x > 0 && local.x < PANEL.x / 2) { // Learn
            const int32_t cost = creatures::skillChangeCost(sheet.level, playerSkills, playerSpecialization, skillDraft,
                                                            draftSpecialization);
            if (inventory && (uint32_t)cost <= inventory->money && (cost == 0 || (trainerNear && trainerNear()))) {
                inventory->money -= (uint32_t)cost;
                playerSkills = skillDraft;
                playerSpecialization = draftSpecialization;
                skillsLearnt = true;
            }
        } else if (local.x > PANEL.x / 2 && local.x < PANEL.x) { // Cancel
            skillDraft = playerSkills;
            draftSpecialization = playerSpecialization;
        }
    }
    return true;
}

} // namespace ui
