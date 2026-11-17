#pragma once
// The screens of Alpha's interface (docs/specs/alpha-ui-screens.md): the start screen with its
// logos and menu, and the in-game system menu. Each frame the game asks for the triangles of
// what is showing and passes clicks back; the answers are actions for the game to carry out.
#include "GameOptions.h"
#include "Speech.h"
#include "worldgen/Random.h"
#include "creatures/Crafting.h"
#include "creatures/Inventory.h"
#include "creatures/InventoryValue.h"
#include "creatures/Skills.h"
#include "creatures/Spirits.h"
#include "ui/Painter.h"
#include "ui/ControllerKeyboard.h"
#include "ui/Vendor.h"
#include <functional>
#include <map>
#include <optional>

struct CubModel;
namespace creatures {
struct Body;
struct HitRecord;
}

namespace ui {

enum class Action {
    None, StartGame, Options, Exit, StartMenu,
    // The Esc menu row (§7).
    Help, Skills, Crafting, Inventory, WorldMap, SystemPanel,
    // The options window's buttons (§6.4); the arrows act inside the window.
    OptionsApply, OptionsOk, OptionsCancel,
};

// The quick-item selector retains its ordinal across wheel openings.
struct QuickItemWheel {
    using Slot = std::pair<int, size_t>;
    bool open = false;
    bool selectorVisible = false;
    int32_t index = 0;
    float radius = 0;
    float angle = 0;
    size_t earlyCount = 0;
    std::optional<Slot> hudSlot;
    int32_t hudCount = 0;

    size_t selected(size_t count) const;
    size_t lateSelection(size_t count) const;
    int32_t steer(int32_t elapsed, bool left, bool right, bool close);
    void layout(int32_t elapsed, size_t count);
    glm::vec2 offset(size_t item, size_t count) const;

  private:
    int32_t leftMs = 0, rightMs = 0;
    bool ready = false;
};

// What the in-game HUD shows (§5.3, §5.8), filled by the game each frame.
struct HudState {
    std::string name;
    float health = 0, maxHealth = 1;
    int32_t level = 1, characterClass = 1;
    int32_t experience = 0, experienceNeeded = 1;
    float mana = 0, charge = 0, stamina = 1;
    std::u16string prompt; // interaction §3: "[R] Talk", "[R] Open", ... under the crosshair
    // Rulings camera-lens-pan §2.1: the local player's mode bit 0x400 swaps `crosshair` for
    // `zoomcrosshair`.
    bool lens = false;
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
// is the hit record's kind (1 block, 3 miss, 5 absorb, 4 and 6 not shown); `critical` and `area`
// are its +0x14 and +0x44 flags.
std::optional<FloatingText> hitText(float damage, uint8_t kind, bool critical, bool area, bool toPlayer, glm::dvec3 at);
std::optional<FloatingText> timedHealText(const creatures::HitRecord &hit, float recipientHeight);
FloatingText levelUpText(glm::dvec3 player);
FloatingText experienceText(int32_t gain, glm::dvec3 player);

// Rulings manacubebar §3 on a `manacubebar` document (its node 0): the bar is shown and its
// children, in file order, are shown while k > 0, k falling by one per child, where
// k = `cubes` mod 4 (C's signed remainder), or 4 while a sequence plays in the bar. Returns k.
int32_t showManaCubeIcons(plasma::Document &bar, int32_t cubes);

// Skills §5.3: a word is punctuation-like only when it is exactly one of . - , ; ! ? : / ).
bool skillTooltipPunctuation(std::u16string_view word);
// Skills §5.4: the skill tooltip's variables V(i, n) at level `n` for the player's class, the
// draft specialization and the committed levels (read by @camouflageduration).
std::map<std::string, std::string> skillTooltipVariables(int32_t n, int32_t cls, int32_t specialization,
                                                         const creatures::SkillLevels &committedSkills);

// Talk §1.6 step 1: a bubble's follow share for an integer step dt (iterated in double, then
// binary32). §1.7: the pitch of a voice blip when the typed count crosses a multiple of three
// (one draw), none otherwise.
float speechFollowShare(int32_t dt);
std::optional<float> voiceBlip(int32_t typedBefore, int32_t consumed, const std::function<int()> &draw);

// Character screens §3.3: the lists' easing share 1 − (1 − rate)^dt (dt steps in double from 0,
// rounded to binary32; 0 for dt ≤ 0).
float listEaseShare(float rate, int32_t dt);
// §3.2, §4.2: a panel figure's or world model's yaw (degrees) after a frame of dtMs, turning
// while hovered and easing back towards the nearest multiple of 360° otherwise.
float panelYawStep(float yaw, bool hovered, float dtMs);

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
    void submitChat(); // shared keyboard/controller queue; no transport here
    bool canOpenControllerKeyboard() const;
    bool openControllerKeyboard();
    bool controllerKeyboardOpen() const;
    bool finishControllerKeyboard(bool accept);
    void controllerKeyboardType(char16_t character, bool backspace = false);
    void controllerKeyboardMove(int x, int y);
    glm::vec2 controllerKeyboardFocus(glm::vec2 window) const;
    bool controllerKeyboardPress(glm::vec2 window, glm::vec2 mouse, int button);
    const ControllerKeyboard &controllerKeyboardState() const { return keyboard; }

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
    // Rulings manacubebar §1 step 4.6: a mana-cube pickup made `manaCubes` a multiple of 4, so
    // `manacubes` plays once (period 0) on `manacubebar`; all four icons show while it runs (§3).
    void playManaCubes();
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
        return controllerKeyboardOpen() || startScreen || systemMenu || menuRow || optionsOpen || mapOpen || inventoryOpen || craftingOpen ||
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
    vendor::Grid vendorGrid;
    creatures::InventoryValue vendorInventory;
    vendor::Buyback buyBack;
    // Engine lifetime adapter: Main resolves the selected actor at each explicit
    // refresh. This does not certify the original cached-pointer lifetime.
    std::function<void(creatures::InventoryValue &)> vendorExport;
    void refreshVendor();
    // Character and world screens (alpha-character-screens.md), after "Start Game". The game
    // fills the lists from the saves, answers `frontRequest` and rebuilds the player's looks
    // when `creationChanged` is set.
    enum class Front { None, Characters, Creation, Worlds, WorldCreation, Connect };
    Front front = Front::None;
    struct CharacterSummary {
        std::string name;
        int32_t level = 1, type = 2, characterClass = 1, specialization = 0;
        uint32_t lastWorldSeed = 0; // §4.2: the world the character was last in
        std::string lastWorldName;
    };
    // Every worlds.db entry in list order; a world's index here is its world index (§4.1). The
    // screen itself lists the singleplayer or the online_ entries.
    struct WorldSummary {
        std::string name;
        uint32_t seed = 0, discovered = 0;
    };
    std::vector<CharacterSummary> characterList;
    // §3.2: a panel figure's turn (degrees), its two eased motion fields (+0x40 drawn as the
    // velocity, +0x34 as the intent; believed walk fields) and the frame time.
    struct FigureMotion {
        float yawDegrees = 0;
        glm::vec3 velocity{0}, intent{0};
        float dtMs = 0;
    };
    // §3.2: a listed character's posed parts (mesh, matrix) in that motion, for its panel.
    std::function<std::vector<std::pair<uint32_t, glm::mat4>>(size_t index, const FigureMotion &motion)> characterParts;
    // §4.2: a world's preview mesh (by world index) and its size in voxels (mesh 0: none).
    std::function<std::pair<uint32_t, glm::vec3>(size_t index)> worldPreview;
    std::vector<WorldSummary> worldList;
    // §4.1: an online world: at least seven characters, the first seven `online_`.
    static bool onlineWorld(const std::string &name) { return name.size() >= 7 && name.compare(0, 7, "online_") == 0; }
    struct Creation {
        int race = 0, gender = 0, characterClass = 0, face = 0, haircut = 0; // class 0 warrior … 3 rogue
        std::array<uint8_t, 3> colour{255, 200, 50}, hover{255, 255, 255}; // §3.6 step 4
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
    // §3.3, §4.3: the selected character index and world index (N, M: the "New ..." panel).
    int selectedCharacter = 0, selectedWorld = 0;
    int activeCharacter = -1; // §4.2: the character last selected or created (P), -1 none
    bool leftHeld = false; // the interface's left-button bit (set by presses, releases and the game)
    void frontType(char16_t ch, bool backspace); // typed into the focused edit
    // §3.3: every mouse-button press, on any screen: the drag smoothing restarts at the cursor
    // and a flung character list stops; a left press sets the held bit.
    void pointerPress(float mouseY, int button);
    // A press on the screens (0 left): the event-2 widgets (style arrows, palette, edit focus).
    bool frontPress(glm::vec2 window, glm::vec2 mouse, int button);
    // §2.1: the left release (event 3), delivered to the visible button under the cursor.
    void frontRelease(glm::vec2 mouse);
    // §3.3 dragging: a cursor move (window pixels) at a millisecond clock.
    void frontMouseMove(float mouseY, uint32_t timeMs);
    // The screens' per-frame update (§3.2–§3.4, §4.2, §4.3), run by `draw` before drawing.
    void frontUpdate(glm::vec2 window, glm::vec2 mouse, float dtMs);
    uint32_t worldSeed() const; // §5.4: the seed edit's value
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
    // §1.5 steps 2 and 4: time, then the 3 s auto-advance and closing, before the frame's talk
    // press; §1.5 step 6 / §1.6 step 1: the glide after each creature, after the talk press.
    void advanceBubbles(float dtMs);
    void followBubbles(float dtMs);
    // §1.7: the single shared gameplay stream the voice blips draw their pitch from, and the
    // pitches of the blips the last draw produced (sound 0x32 at the player, volume 1), taken
    // by the game.
    std::function<int()> speechDraw;
    std::vector<float> voiceBlips;
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
    QuickItemWheel quickWheel;
    void setQuickWheelLabel(const creatures::Stack *selection);

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
    // §5.2.1, §5.2.6: a first release selects the portal. A second release over
    // the selected portal moves the controlled actor.
    bool teleportMode = false;
    glm::ivec2 hoveredPortal{-1}, selectedPortal{-1};
    std::u16string cursorLabel; // shown at the cursor ("Teleport")
    void mapRelease(creatures::Body *body);

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
    enum class ControllerEdit { None, CharacterName, WorldName, WorldSeed, Server, Chat };
    ControllerEdit keyboardTarget = ControllerEdit::None;
    ControllerKeyboard keyboard;
    ControllerEdit controllerEdit() const;
    bool controllerKeyboardValid() const;
    void drawControllerKeyboard(Painter &painter, glm::vec2 window, glm::vec2 mouse);
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
    std::optional<plasma::Document> crosshair, zoomCrosshair;
    std::vector<plasma::Document> menuButtons;
    // §5.5: six ability buttons and the quick-item button, with the cooldown overlay's rest
    // deformation, and per ability id the texture ids of its icon in colour and in grey (the
    // same in every button).
    std::vector<plasma::Document> abilityButtons;
    std::optional<plasma::Document> quickItemButton, quickItemSelector;
    glm::mat4 cooldownRest{1};
    std::map<int32_t, std::pair<int32_t, int32_t>> abilityIcons;
    void drawAbilityBar(Painter &painter, glm::vec2 window, const HudState &state);
    void drawQuickItems(Painter &painter, glm::vec2 window, const HudState &state);

    void drawFloating(Painter &painter, float dtMs);
    // §8.1, in InventoryScreen.cpp.
    void loadInventory();
    void drawInventory(Painter &painter, glm::vec2 window, glm::vec2 mouse);
    void drawCharacter(Painter &painter);
    // With `world`, the item is the world item under the target (its compare box sits above).
    void drawTooltip(Painter &painter, glm::vec2 window, glm::vec2 at, const creatures::ItemRecord &item, bool world = false);
    // `nameLine` false leaves the name line out but still skips its height (item services
    // §2.3: the Adaption preview starts with the Power line one line lower).
    void drawItemText(Painter &painter, glm::vec2 at, const creatures::ItemRecord &item, float wrap, bool classLine,
                      bool nameLine = true);
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
    // §3.3: a list's column of 600 × 220 panels (top-left corners) and its scroll speed (px/s).
    struct PanelColumn {
        std::vector<float> y;
        float x = 300, speed = 0;
        bool rebuild = true; // §3.1: placed again at (300, 50 + 230 i) at the next update
    };
    // §3.4: a list's Select / small / Delete buttons, placed on the selected panel.
    struct ListButtons {
        glm::vec2 at{0};
        bool select = false, small = false, remove = false, create = false; // create: "Create" caption
    };
    // §3.2, §4.2: a figure's yaw (degrees) and motion fields. (The animation blend +0x1188 and
    // clock +0x118c have no counterpart in the engine's pose, which keeps its own walk cycle.)
    struct PanelFigure {
        float yaw = 0;
        glm::vec3 velocity{0}, intent{0};
    };
    enum class FrontButton { Back, Multiplayer, ConnectToServer, Connect, CreateCharacter, CreateWorld, Select, Small, Delete };
    struct FrontHit {
        FrontButton button;
        glm::vec2 at, size;
    };
    void stepList(bool characters, glm::vec2 window, glm::vec2 mouse, float dtMs);
    void stepFigure(PanelFigure &figure, bool hovered, float dtMs) const;
    int worldPanel(int worldIndex) const; // §4.3: the panel of a world index, -1 none
    bool activeLastWorld(const WorldSummary &world) const; // §4.2: P is drawn on this world's panel
    std::vector<FrontHit> visibleFrontButtons(glm::vec2 window) const;
    void activateFront(FrontButton button);
    bool characterNameTaken() const;
    bool worldNameTaken() const;
    bool worldReady() const;
    bool createCharacterShown() const;
    std::optional<plasma::Document> listPanel, stylePanel, bigButton, editBox;
    PanelColumn characterColumn, worldColumn;
    ListButtons characterButtons, worldButtons;
    std::vector<PanelFigure> characterFigures;
    std::vector<float> worldYaw;         // §4.2: each world panel's model yaw (widget +0x164)
    PanelFigure activeFigure;            // §4.2: P walking on its last world's panel
    std::vector<uint32_t> listedWorlds;  // panel j -> world index, this mode
    glm::vec2 frontWindow{0};            // the window at the last update (the drag recentres x)
    float smoothY = 0, moveY = 0;        // §3.3: the smoothed and the previous cursor y
    uint32_t moveTime = 0;
    bool moveTimeKnown = false;
    // The buttons visible when the left button went down (engine reading of open point C-9).
    std::optional<std::pair<Front, std::vector<FrontHit>>> pressedButtons;
    bool multiplayerToggled = false; // (the start-up caption has a capital W)
    bool nameCaptionCleared = false; // §3.6 step 3: Create clears the "Name" caption (C-10)
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
    creatures::ItemRecord benchCursor{}; // §3.4, §3.5: the cursor item, rebuilt every draw
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
    std::optional<creatures::ItemRecord> vendorHovered;
    // Talk §3.4: one unit sold from a stack (the money, the sound, the buy-back list).
    void sell(creatures::Stack &stack);
    vendor::Services vendorServices();
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
    void drawQuestTag(Painter &painter, glm::vec2 window);
    // Screens §5.3: `manacubebar`, its icons chosen (showManaCubeIcons) in the HUD block before
    // the interface update (`advanceSequences`) of the same frame (rulings manacubebar §2).
    std::optional<plasma::Document> manaCubeBar;
    void advanceSequences(float dtMs);
    std::string landNameShown, landDetailShown;
    float landNameMs = 0;
    double lastSeconds = -1;

    Fonts fonts;
    plasma::Document start, gui;
    std::optional<plasma::Document> logo, picroma, systemPanel;
};

} // namespace ui
