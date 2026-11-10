#pragma once
// The screens of Alpha's interface (docs/specs/alpha-ui-screens.md): the start screen with its
// logos and menu, and the in-game system menu. Each frame the game asks for the triangles of
// what is showing and passes clicks back; the answers are actions for the game to carry out.
#include "GameOptions.h"
#include "Speech.h"
#include "worldgen/Random.h"
#include "creatures/Crafting.h"
#include "creatures/Inventory.h"
#include "creatures/Skills.h"
#include "creatures/Spirits.h"
#include "ui/Painter.h"
#include <functional>
#include <map>
#include <optional>

struct CubModel;

namespace ui {

enum class Action {
    None, StartGame, Options, Exit, StartMenu,
    // The Esc menu row (§7).
    Help, Skills, Crafting, Inventory, WorldMap, SystemPanel,
    // The options window's buttons (§6.4); the arrows act inside the window.
    OptionsApply, OptionsOk, OptionsCancel,
};

// What the in-game HUD shows (§5.3, §5.8), filled by the game each frame.
struct HudState {
    std::string name;
    float health = 0, maxHealth = 1;
    int32_t level = 1, characterClass = 1;
    int32_t experience = 0, experienceNeeded = 1;
    float mana = 0, charge = 0, stamina = 1;
    std::u16string prompt; // interaction §3: "[R] Talk", "[R] Open", ... under the crosshair
    // §5.3: the player's pet, when there is one: its bars and the riding bar's icons.
    bool hasPet = false;
    std::string petName, petSpecies;
    float petHealth = 0, petMaxHealth = 1, ridingEnergy = 1;
    int32_t petLevel = 1, petExperience = 0, petExperienceNeeded = 1;

    // §5.4: a life bar over a creature's head, its pivot at `at` (window pixels).
    enum class BarStyle { Enemy, Friend, Static, Neutral };
    struct CreatureBar {
        glm::vec2 at{0};
        BarStyle style = BarStyle::Friend;
        float fraction = 1;     // health / max health
        std::u16string info;    // species name and level bonus
        std::u16string rareName; // the creature's own name or title; empty hides it
        glm::vec4 infoColour{1}, rareColour{0, 0.731f, 1, 1};
    };
    std::vector<CreatureBar> creatureBars;

    // §5.5: the ability bar. Slots 0..5 are M1, M2, 1, 2, 3, 4.
    struct AbilitySlot {
        int32_t id = 0;        // ability id (icon table); 0 shows none.png
        bool usable = true;    // false greys the icon
        float cooldown = 0;    // remaining / length, 0 when none
        bool held = false;     // its control is held (tints the frame)
    };
    AbilitySlot abilities[6];
    std::u16string quickCount = u" "; // the quick item's stack size
    bool quickHeld = false;
};

// §5.6: a text that rises (or sinks) and fades over a point in the world.
struct FloatingText {
    glm::dvec3 anchor{0}; // world blocks
    glm::vec2 offset{0};  // pixels
    glm::vec4 colour{1};
    std::u16string text;
    float size = 20, rise = -100, lifeMs = 1000, ageMs = 0;
    int32_t flags = 1; // text alignment (§3.4)
};

// §5.6's producers. A hit's text, or none for the kinds and amounts that show nothing; `kind`
// is the hit record's kind (1 block, 3 miss, 5 absorb, 4 and 6 not shown).
std::optional<FloatingText> hitText(float damage, uint8_t kind, bool critical, bool toPlayer, glm::dvec3 at);
FloatingText levelUpText(glm::dvec3 player);
FloatingText experienceText(int32_t gain, glm::dvec3 player);

class Interface {
  public:
    // Loads the fonts and the .plx files from the install folder.
    bool load(const std::filesystem::path &install, std::string *error = nullptr);
    // The menu row's icons (§7), PNGs read through `read` (from data3.db).
    void loadIcons(const std::function<std::optional<std::vector<uint8_t>>(const std::string &)> &read);

    // What is showing.
    bool startScreen = false;
    bool systemMenu = false; // the system panel (O)
    bool menuRow = false;    // the Esc menu row (§7)
    bool allLifeBars = false; // V: bars over every creature in range, not only the focused (§5.4)
    std::optional<HudState> hud; // in the game
    std::vector<FloatingText> floating; // §5.6, aged and dropped as they are drawn
    // §9: the chat box. `chatAdd` appends text in a colour (bytes), wrapping at 380 px and
    // breaking lines at line feeds; Enter opens the input and submits it into `chatSubmitted`.
    void chatAdd(const std::string &text, glm::vec3 colour);
    bool chatOpen = false;
    std::u16string chatInput;
    std::vector<std::string> chatSubmitted; // taken by the game
    bool typing() const { return chatOpen; }
    // Engine controller extension: back cancels a draft/panel, never applies it
    // or submits chat. Returns false when no open screen can be left.
    bool controllerBack();

    // §5.9, §5.9.1: called every frame with the current texts; shows the banner when they
    // differ from the last shown and no banner is running.
    void showLandName(const std::string &name, const std::string &detail);
    // Missions §8.2: the small quest tag at (W - 200, 400), its objective text, and its bar
    // filled to `questFill` when `questBar`.
    bool questTagShown = false, questBar = false;
    float questFill = 0;
    // Sets the objective text and plays the tag's `show` sequence from the start.
    void setObjective(const std::u16string &text);
    // Alpha-open-points-1 §2: the big "QUEST COMPLETE!" banner at (W/2, H - 300) plays its
    // `shine` sequence from the start, with `objective` as its text.
    void questComplete(const std::u16string &objective);
    // Quests (docs/design/quests.md): the Steam-style icons over quest givers' heads and on the
    // world map: an orange "!" (a quest to offer), a grey "?" (under way) and an orange "?"
    // (done: hand it in). Map marks are window pixels, the icon's foot.
    enum class QuestIcon : uint8_t { Offer, Underway, HandIn };
    struct QuestMark {
        glm::dvec3 at{0};
        QuestIcon icon = QuestIcon::Offer;
    };
    std::vector<QuestMark> questMarks;
    std::vector<std::pair<glm::vec2, QuestIcon>> mapQuestMarks;
    // World blocks to window pixels with the game camera; none behind the camera.
    std::function<std::optional<glm::vec2>(glm::dvec3)> project;
    // Whether the mouse belongs to the interface (a menu is open) rather than the camera.
    bool wantsMouse() const {
        return startScreen || systemMenu || menuRow || optionsOpen || mapOpen || inventoryOpen || craftingOpen ||
               skillsOpen || front != Front::None;
    }

    // §8.1: the inventory screen (B or I): the bag window, the character panel and the twelve
    // equipment boxes. The game points `inventory` at the player's belongings and fills
    // `sheet` each frame; `itemModel` gives an item's mesh and size, `itemName` its name.
    bool inventoryOpen = false;
    creatures::Inventory *inventory = nullptr;
    struct CharacterSheet {
        std::string name;
        int32_t level = 1, characterClass = 1, specialization = 0;
        float health = 0, armour = 0, resistance = 0, critical = 0, tempo = 1, regeneration = 0;
        float weaponRating = 0, armorRating = 0;
        bool canAct = true; // no hit stun: consumables may be used (§8.1.10)
    };
    CharacterSheet sheet;
    struct ItemModel {
        uint32_t mesh = 0;
        glm::vec3 size{0};
    };
    std::function<ItemModel(const creatures::ItemRecord &)> itemModel;
    std::function<std::u16string(const creatures::ItemRecord &)> itemName;
    glm::vec3 itemTurn{225, 0, 0}; // §8.1.6: every interface item's turn, advanced by the game
    // §5.2.3 step 7: the world map's compass (model 2559) and Alpha's camera yaw ψ in degrees.
    ItemModel compass;
    // §8.1.7 world source: the item the player would pick up, shown at the bottom right.
    std::optional<creatures::ItemRecord> targetItem;
    float compassYaw = 0;
    // A mouse press (0 left, 1 right, 2 middle) with the inventory open. Returns false when it
    // was over none of its windows (the game then drops or closes, §8.1.8).
    bool inventoryPress(glm::vec2 size, glm::vec2 mouse, int button);
    // Talk §3: the vendor window beside the inventory: the stock of the vendor talked to
    // (tab 0) and the buy-back list (tab 1, newest first, shared by all vendors, at most 10).
    bool vendorOpen = false;
    int vendorTab = 0;
    std::vector<creatures::ItemRecord> vendorStock;
    std::vector<creatures::Stack> buyBack; // oldest first
    // Character and world screens (alpha-character-screens.md), after "Start Game". The game
    // fills the lists from the saves, answers `frontRequest` and rebuilds the player's looks
    // when `creationChanged` is set.
    enum class Front { None, Characters, Creation, Worlds, WorldCreation, Connect };
    Front front = Front::None;
    struct CharacterSummary {
        std::string name;
        int32_t level = 1, type = 2, characterClass = 1, specialization = 0;
    };
    struct WorldSummary {
        std::string name;
        uint32_t seed = 0, discovered = 0;
    };
    std::vector<CharacterSummary> characterList;
    // §4.1: a listed character's posed parts (mesh, matrix) turned to `yaw`, for its panel.
    std::function<std::vector<std::pair<uint32_t, glm::mat4>>(size_t index, float yaw)> characterParts;
    float panelYaw = 0; // the panels' figures turn 0.05° per ms
    // §4.2: a listed world's preview mesh and its size in voxels (mesh 0: none).
    std::function<std::pair<uint32_t, glm::vec3>(size_t index)> worldPreview;
    std::vector<WorldSummary> worldList;
    struct Creation {
        int race = 0, gender = 0, characterClass = 0, face = 0, haircut = 0; // class 0 warrior … 3 rogue
        std::array<uint8_t, 3> colour{255, 200, 50}, hover{255, 200, 50};
    };
    Creation creation;
    bool creationChanged = false;
    std::u16string nameText, seedText, serverText;
    bool multiplayerWorlds = false; // §4.3: the world list shows the online_ worlds
    int editFocus = 0; // world creation: 0 the name, 1 the seed
    enum class FrontRequest {
        None, SelectCharacter, CreateCharacter, DeleteCharacter, SelectWorld, CreateWorld, DeleteWorld, Connect,
        ListWorlds, // the multiplayer toggle: the game lists the worlds again
    };
    FrontRequest frontRequest = FrontRequest::None;
    int selectedCharacter = 0, selectedWorld = 0;
    bool leftHeld = false; // the left button, for dragging the lists (set by the game)
    void frontType(char16_t ch, bool backspace); // typed into the focused edit
    bool frontPress(glm::vec2 window, glm::vec2 mouse, int button);
    // Skills (alpha-skills.md): the skill window (X). The game fills the committed levels,
    // specialization and mana cubes each frame and says whether a class trainer is near; Learn
    // pays from the inventory's money and sets `skillsLearnt` for the game to copy the draft.
    bool skillsOpen = false;
    const Dictionary *dictionary = nullptr; // the skill tooltips' texts
    creatures::SkillLevels playerSkills{}, skillDraft{};
    int32_t playerSpecialization = 0, draftSpecialization = 0, manaCubes = 0;
    std::function<bool()> trainerNear;
    bool skillsLearnt = false;
    void toggleSkills(); // §1.3: X, copying the committed values into the draft
    // Item services §3: the Weapon Customization bench (static 77), left of the inventory. A
    // right or middle press on a weapon in the bags or the equipment boxes chooses it; the
    // middle button turns it (`benchRotate`). `itemVoxels` gives an item model's voxels and
    // `cubeMeshId` the one-voxel cube the attached cubes are drawn with.
    bool benchOpen = false;
    int benchBag = -1, benchBox = -1;
    size_t benchSlot = 0;
    glm::vec3 benchTurn{-90, 0, 90};
    std::function<const CubModel *(const creatures::ItemRecord &)> itemVoxels;
    uint32_t cubeMeshId = 0;
    void benchRotate(glm::vec2 delta);
    void benchReset();
    creatures::ItemRecord *benchItem();
    // Item services §1, §2: the identifier's and the adapter's windows (NPC classes 131 and
    // 137), beside the inventory. A right or middle press on a bag slot (or, for adaption, an
    // equipment box) chooses the item. Adaptation always uses the corrected cost (Loot.h).
    bool identifyOpen = false, adaptOpen = false;
    int identifyBag = -1, adaptBag = -1, adaptBox = -1;
    size_t identifySlot = 0, adaptSlot = 0;
    // Talk §1: speech bubbles, four at most, each following its creature. `creatureAt(id)`
    // gives a creature's drawn centre and height (blocks), none once it has gone.
    std::function<std::optional<std::pair<glm::dvec3, float>>(uint64_t)> creatureAt;
    // §1.3: opens a bubble for `creature`, or advances (finishes typing, next page) its own.
    void talk(uint64_t creature, const std::function<std::vector<SpeechPage>()> &text);
    // §1.4: time, the 3 s auto-advance, closing and the glide after the creature.
    void stepBubbles(float dtMs);
    // Crafting (alpha-crafting.md §4, §5): the recipe window (C or a station) and its preview.
    // `stationNear` says whether the player is near a station; the game makes what
    // `finishedCraft` holds and empties it.
    bool craftingOpen = false;
    int craftingTab = 0;
    std::function<bool(int32_t station)> stationNear;
    std::optional<creatures::Recipe> finishedCraft;
    void selectRecipe(int tab, size_t row) { craftingTab = tab, craftSelection = std::pair{tab, row}; }
    // §5.1, §5.2: the craft timer, run by the game every frame with the left button's state.
    void stepCraft(float dtMs, bool leftHeld);
    // What presses asked for, taken by the game: sounds, items to use, equipment changes.
    std::vector<int32_t> inventorySounds;
    std::vector<creatures::ItemRecord> itemsToUse;
    bool equipmentChanged = false, gliderEquipped = false;

    // §5.2: the world map. While it is open the game draws the map instead of the world and the
    // interface shows only these labels (positions in window pixels).
    bool mapOpen = false;
    float mapZoom = 1, mapZoomTarget = 1; // s, eased towards the target (§5.2.2)
    glm::dvec3 mapPan{0};                 // blocks
    struct MapLabel {
        glm::vec2 at;
        std::u16string text;
        glm::vec4 colour;
        float size;
    };
    std::vector<MapLabel> mapLabels;
    // §5.2.1, §5.2.6: the map opened from a portal. A first click on a portal label selects it,
    // a second while hovering it asks for a teleport to that zone's centre (`teleportTo`).
    bool teleportMode = false;
    glm::ivec2 hoveredPortal{-1}, selectedPortal{-1};
    std::optional<glm::ivec2> teleportTo;
    std::u16string cursorLabel; // shown at the cursor ("Teleport")
    void mapRelease(); // the left button released over the map

    // §6: the options window. It edits `optionsCopy`; Apply and OK hand it to the game.
    // `resolutions` is the display modes' distinct sizes (§6.5).
    bool optionsOpen = false;
    bool optionsToApply = false; // Apply or OK was pressed; the game takes the copy
    GameOptions optionsCopy;
    std::vector<glm::ivec2> resolutions;
    // Fills the copy from the settings in force (at start-up, §6.1 step 4).
    void setOptions(const GameOptions &current);

    // The triangles of everything showing, for a window of `size` pixels with the mouse at
    // `mouse` (window pixels).
    Frame draw(glm::vec2 size, glm::vec2 mouse, double seconds);
    // A left click at `mouse`; returns what it asks for.
    Action click(glm::vec2 size, glm::vec2 mouse);

    Atlas atlas;
    // The quest icons in the atlas (loaded on first use) and their sizes in pixels.
    struct QuestPicture {
        AtlasRect rect;
        glm::vec2 size{0};
    };
    std::array<QuestPicture, 3> questPictures{};
    bool questPicturesLoaded = false;
    void drawQuestIcon(Painter &painter, QuestIcon icon, glm::vec2 foot, float scale);

  private:
    struct MenuItem {
        const char *label;
        float baseline;
        Action action;
    };
    struct Menu {
        std::vector<MenuItem> items;
        float pixelSize, outline;
        glm::vec2 origin, size; // the widget's rectangle
    };
    Menu startMenu(glm::vec2 window) const;
    Menu systemMenuLayout(glm::vec2 window) const;
    void drawMenu(Painter &painter, const Menu &menu, glm::vec2 mouse);
    std::optional<size_t> hovered(const Menu &menu, glm::vec2 mouse) const;
    void drawHud(Painter &painter, glm::vec2 window, const HudState &state);
    // §7: menu button i's pivot, and the button under the mouse.
    static glm::vec2 menuButtonAt(glm::vec2 window, int i);
    std::optional<int> menuButtonUnder(glm::vec2 window, glm::vec2 mouse) const;

    // A HUD template (looked up in gui.plx and kept as a clone), with its fill bars' rest
    // deformation.
    struct Bar {
        std::optional<plasma::Document> doc;
        std::vector<std::pair<int32_t, glm::mat4>> fills; // node, deformation at rest
    };
    Bar lifebar, experiencebar, mpbar, hpbar, staminabar;
    Bar petLifebar, petExperiencebar; // §5.3: clones for the pet
    std::optional<plasma::Document> ridingbar;
    Bar creatureBars[4]; // §5.4: enemylifebar, friendlifebar, staticlifebar, neutrallifebar (:small)
    std::optional<plasma::Document> crosshair;
    std::vector<plasma::Document> menuButtons;
    // §5.5: six ability buttons and the quick-item button, with the cooldown overlay's rest
    // deformation, and per ability id the texture ids of its icon in colour and in grey (the
    // same in every button).
    std::vector<plasma::Document> abilityButtons;
    std::optional<plasma::Document> quickItemButton;
    glm::mat4 cooldownRest{1};
    std::map<int32_t, std::pair<int32_t, int32_t>> abilityIcons;
    void drawAbilityBar(Painter &painter, glm::vec2 window, const HudState &state);

    void drawFloating(Painter &painter, float dtMs);
    // §8.1, in InventoryScreen.cpp.
    void loadInventory();
    void drawInventory(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    void drawCharacter(Painter &painter);
    // With `world`, the item is the world item under the target (its compare box sits above).
    void drawTooltip(Painter &painter, glm::vec2 window, glm::vec2 at, const creatures::ItemRecord &item, bool world = false);
    void drawItemText(Painter &painter, glm::vec2 at, const creatures::ItemRecord &item, float wrap, bool classLine);
    void drawItem(Painter &painter, glm::vec2 window, const creatures::ItemRecord &item, glm::vec2 at, float scale,
                  float depth = 0);
    void scrollRange();
    // The character and world screens, in FrontScreens.cpp.
    void loadFront();
    void drawFront(Painter &painter, glm::vec2 window, glm::vec2 mouse, float dtMs, double seconds);
    void drawStylePanel(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    void drawButton(Painter &painter, glm::vec2 at, glm::vec2 size, const std::u16string &caption, glm::vec2 mouse);
    void drawEdit(Painter &painter, glm::vec2 labelAt, const std::u16string &label, const std::u16string &text, bool focused,
                  double seconds);
    void scrollList(float &top, float &speed, size_t count, glm::vec2 window, glm::vec2 mouse, float dtMs);
    float characterListSpeed = 0, worldListSpeed = 0, lastMouseY = -1;
    bool characterNameTaken() const;
    bool worldNameTaken() const;
    bool worldReady() const;
    std::optional<plasma::Document> listPanel, stylePanel, bigButton, editBox;
    float characterListTop = 50, worldListTop = 50;
    bool multiplayerToggled = false; // (the start-up caption has a capital W)
    bool deleteShown = false;
    // Skills, in SkillScreen.cpp.
    void loadSkills();
    void drawSkills(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    bool skillsPress(glm::vec2 window, glm::vec2 mouse, int button);
    void drawSkillTooltip(Painter &painter, glm::vec2 window, glm::vec2 at, int32_t skill, int32_t specialization);
    std::optional<plasma::Document> skillsPanel;
    std::vector<plasma::Document> skillButtons, specializationButtons;
    // Per button document: icon file -> (colour, grey) texture ids.
    std::vector<std::map<std::string, std::pair<int32_t, int32_t>>> skillButtonIcons, specializationIcons;
    // The bench, in BenchScreen.cpp.
    void drawBench(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    bool benchPress(glm::vec2 window, glm::vec2 mouse, int button);
    std::optional<plasma::Document> benchPanel;
    std::vector<creatures::Stack> benchPalette;
    int benchSelected = -1, benchHovered = -1;
    bool benchHolding = false, benchTargetValid = false;
    creatures::SpiritCube benchHeld;
    glm::ivec3 benchTarget{0}, benchHit{0};
    // Item services, in ServicesScreen.cpp.
    void loadServices();
    void drawServices(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    bool servicesPress(glm::vec2 window, glm::vec2 mouse, int button);
    creatures::ItemRecord *identifySource();
    creatures::ItemRecord *adaptSource();
    int32_t adaptPrice();
    std::optional<plasma::Document> identifyPanel, adaptPanel, itemFrame, adaptArrow;
    worldgen::Random serviceRandom{12345};
    // Talk §3, in VendorScreen.cpp.
    void loadVendor();
    void drawVendor(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    bool vendorPress(glm::vec2 window, glm::vec2 mouse, int button);
    std::optional<plasma::Document> vendorPanel;
    std::vector<plasma::Document> vendorTabs;
    int32_t vendorRow = 0;
    std::optional<creatures::ItemRecord> vendorHovered;
    // Talk §3.4: one unit sold from a stack (the money, the sound, the buy-back list).
    void sell(creatures::Stack &stack);
    // Talk §1, in SpeechBubbles.cpp.
    struct Bubble {
        uint64_t creature = 0; // 0: free
        glm::dvec3 anchor{0};  // blocks
        std::vector<SpeechPage> pages;
        float elapsed = 0;
        size_t page = 0;
        int32_t typedBefore = 0; // characters typed last frame (for the voice blips)
    };
    std::array<Bubble, 4> bubbles;
    std::optional<plasma::Document> speechBubble;
    void drawBubbles(Painter &painter, glm::vec2 window);
    // Crafting, in CraftingScreen.cpp.
    void loadCrafting();
    void drawCrafting(Painter &painter, glm::vec2 window, glm::vec2 mouse, float dtMs);
    bool craftingPress(glm::vec2 window, glm::vec2 mouse, int button);
    std::optional<plasma::Document> craftingPanel, previewPanel, wideItemBox, itemSelector, craftButton, craftBar;
    std::vector<plasma::Document> craftingTabs;
    std::map<std::string, int32_t> craftingIcons; // texture ids in each tab document
    std::array<int32_t, 6> craftingRow{};          // scroll rows
    std::optional<std::pair<int, size_t>> craftSelection;
    creatures::Recipe crafting;  // the recipe being made (copied at the start, §5.1)
    float craftTimer = -1;
    bool craftReady = false;     // ready and under the mouse last frame (+0x3c4)
    glm::vec3 previewTurn{-120, 0, 0};
    std::optional<plasma::Document> inventoryPanel, characterPanel, itemBox, upButton, downButton, scrollThumb, preview;
    std::optional<plasma::Document> equippedPreview; // the compare box (open points 2 §3)
    std::vector<plasma::Document> inventoryTabs, equipmentBoxes;
    int inventoryTab = 0;
    std::array<int32_t, 4> scrollRow{}, scrollPositions{};
    bool draggingThumb = false;
    void drawOptions(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    std::optional<Action> clickOptions(glm::vec2 window, glm::vec2 mouse);
    std::optional<plasma::Document> optionsPanel, leftArrow, rightArrow;
    std::vector<plasma::Document> optionsButtons; // Apply, OK, Cancel
    size_t resolutionIndex = 0;
    void drawChat(Painter &painter, glm::vec2 window, double seconds);
    struct ChatWord {
        std::u16string text; // " " for a space
        glm::vec4 colour;
    };
    std::vector<std::vector<ChatWord>> chatLines;
    void drawLandName(Painter &painter, glm::vec2 window, float dtMs);
    std::optional<plasma::Document> landName;
    std::optional<plasma::Document> smallQuestTag; // quest-tag.plx
    std::optional<plasma::Document> questBanner;   // its `questtag`: QUEST COMPLETE!
    float questBannerMs = -1, questBannerLength = 0;
    void drawQuestBanner(Painter &painter, glm::vec2 window, float dtMs);
    int32_t questBarFill = -1;
    glm::mat4 questBarRest{1};
    float questShowMs = 1e9f; // time since the tag's `show` sequence started
    void drawQuestTag(Painter &painter, glm::vec2 window, float dtMs);
    std::string landNameShown, landDetailShown;
    float landNameMs = 0;
    double lastSeconds = -1;

    Fonts fonts;
    plasma::Document start, gui;
    std::optional<plasma::Document> logo, picroma, systemPanel;
};

} // namespace ui
