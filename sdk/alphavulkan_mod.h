/* AlphaVulkan mod SDK: the one header a code mod includes.
 *
 * A code mod is a DLL in its mod folder (docs/modding.md) that exports
 *
 *     int avModLoad(const AvHost *host, AvMod *mod);
 *
 * The engine calls it once at start-up. The mod checks `host->apiVersion`, fills in `mod`
 * (any callback may stay null) and returns 1, or 0 to decline. Everything the engine offers is
 * a function pointer in AvHost; the mod never links against the engine.
 *
 * Stability: this is a plain C interface. New members are only ever added at the end of
 * AvHost and AvMod, and `size` says how many bytes each side filled in, so a mod built against
 * an older header keeps working with a newer engine. AV_API_VERSION goes up when members are
 * added; an engine never removes or reorders them.
 *
 * Mods are opt-in: the engine only loads the mods listed in AlphaVulkan.ini [Mods] Load.
 * The SDK is AlphaVulkan's own; it contains nothing from Cube World. */
#ifndef ALPHAVULKAN_MOD_H
#define ALPHAVULKAN_MOD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AV_API_VERSION 9

#if defined(_WIN32)
#define AV_EXPORT __declspec(dllexport)
#else
#define AV_EXPORT __attribute__((visibility("default")))
#endif

/* Version 3: the local player as getPlayer reports it. */
typedef struct AvPlayer {
    double x, y, z; /* position in world blocks (the centre of the collision box) */
    float health, maxHealth;
    int32_t level, experience;
} AvPlayer;

/* Version 4: a static object of a zone (a door, a chest, a market stand, a campfire...). */
typedef struct AvStatic {
    int32_t kind;    /* static kind (alpha-worldgen-structures.md; e.g. 10 chest, 65 campfire) */
    double x, y, z;  /* world blocks: the centre of its foot */
    int32_t rotation; /* quarter turns */
    float scale[3];  /* size in blocks */
} AvStatic;

/* Version 5: a map cell's mission (alpha-missions.md §1.1). A map cell is 8 x 8 zones. */
typedef struct AvMission {
    int32_t cellX, cellY;
    int32_t kind;       /* 0 none, 1 monster hunt, 3/4/9 town, 5 dungeon (§1.3) */
    int32_t targetType; /* the creature type to defeat */
    int32_t level, tier;
    int32_t state;      /* 0 not discovered, 1 discovered, 2 complete */
    int32_t count, required;
    int32_t targetZoneX, targetZoneY;
} AvMission;

/* Version 6: a creature the engine runs (an NPC, a pet, a bomb) or another player. */
typedef struct AvCreature {
    uint64_t id;
    int32_t type;       /* creature type (alpha-creatures-models.md) */
    int32_t controller; /* 0 player, 1 hostile, 3 townsperson, 5 pet, 6 inert */
    int32_t level;
    float health, maxHealth;
    double x, y, z;     /* centre, world blocks */
} AvCreature;

/* One of the local player's quests (docs/design/quests.md). */
typedef struct AvQuest {
    uint32_t id;
    int32_t kind;   /* 1 rescue, 2 battle, 3 hunt, 4 champion */
    int32_t state;  /* 1 under way, 2 done: hand it in to the giver */
    uint64_t giver; /* the giver's creature id */
    int32_t level;
    int32_t count, required; /* progress */
    double siteX, siteY, siteZ; /* the quest's place, world blocks (z 0 until it is loaded) */
} AvQuest;

/* What the engine offers a mod. `context` is the engine's; pass it back to every call. */
typedef struct AvHost {
    uint32_t size;       /* sizeof(AvHost) as the engine knows it */
    uint32_t apiVersion; /* AV_API_VERSION of the engine */
    void *context;

    /* Writes a line to the engine's log, prefixed with the mod's name. */
    void (*log)(void *context, const char *message);

    /* Reads a content key (e.g. "airship.cub", "dict_en.xml"), with other mods' overrides
     * applied. Copies up to `capacity` bytes into `buffer` and sets `*size` to the full size.
     * Returns 1 if the key exists, 0 if not. Call with a null buffer to ask only the size. */
    int (*readContent)(void *context, const char *key, void *buffer, uint32_t capacity, uint32_t *size);

    /* Plays a sound key (e.g. "menu-select.wav") at `volume` (1 = as stored). 1 if started. */
    int (*playSound)(void *context, const char *key, float volume);

    /* The world's time of day in milliseconds (0 .. 86,400,000; noon is 43,200,000). */
    double (*getTimeOfDay)(void *context);
    void (*setTimeOfDay)(void *context, double milliseconds);

    /* Version 2. */
    /* Reads the block at world block (x, y, z) of a zone the engine holds: its colour and its
     * type (alpha-worldgen-terrain.md §1.3; 0 air, 2 water, 4 grass, ...). Returns 1, or 0
     * when that zone is not held (the outputs are then left alone). */
    int (*getBlock)(void *context, int32_t x, int32_t y, int32_t z, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *type);

    /* Version 3. */
    /* Fills `out` with the local player. Returns 0 when nobody is playing (fly mode). */
    int (*getPlayer)(void *context, AvPlayer *out);

    /* Version 4. */
    /* Floating text over world point (x, y, z), rising and fading for a second like the game's
     * combat numbers (alpha-ui-screens.md §5.6). `size` in pixels (20 is a normal hit). Returns
     * 0 when no game is showing. */
    int (*showText)(void *context, double x, double y, double z, const char *text, float r, float g, float b, float size);
    /* The land-name banner (alpha-ui-screens.md §5.9) with a title and a line under it. */
    int (*showBanner)(void *context, const char *name, const char *detail);
    /* The static objects of a held zone: how many, and one by index. getStatic returns 0 for a
     * zone not held or an index past the end. */
    uint32_t (*getStaticCount)(void *context, int32_t zx, int32_t zy);
    int (*getStatic)(void *context, int32_t zx, int32_t zy, uint32_t index, AvStatic *out);

    /* Version 5. */
    /* Fills `out` with map cell (cellX, cellY)'s mission. Returns 0 when the engine knows no
     * mission there. */
    int (*getMission)(void *context, int32_t cellX, int32_t cellY, AvMission *out);
    /* Writes a block into a held zone as a block edit (alpha-block-edits.md §2): `type` is the
     * whole type byte (type and flags; 0 removes the block). The edit is kept and saved with its
     * zone and expires like the game's own after three days. Returns 0 when the zone is not held
     * or the game is a client of a server (only the server changes blocks there). */
    int (*setBlock)(void *context, int32_t x, int32_t y, int32_t z, uint8_t r, uint8_t g, uint8_t b, uint8_t type);

    /* Version 6. */
    /* The creatures the engine holds now (offline: those it runs; online: those the server
     * lists), and one by index. getCreature returns 0 for an index past the end. */
    uint32_t (*getCreatureCount)(void *context);
    int (*getCreature)(void *context, uint32_t index, AvCreature *out);

    /* Version 7. */
    /* A burst of `count` particle cubes at (x, y, z) blocks (alpha-particles.md §1, §2): each
     * flies with (vx, vy, vz) blocks/s plus up to `spread` on each axis, `size` blocks on a side,
     * in (r, g, b) (0..1). `style` 0 lit like the world, 1 random fire colours, 2 full light,
     * 3 still, 4 drifting without gravity. Shown on this machine only (bursts are never sent
     * to a server). Returns 0 when no world is showing. */
    int (*spawnParticles)(void *context, double x, double y, double z, float vx, float vy, float vz, float r,
                          float g, float b, float size, int32_t count, int32_t style, float spread);

    /* Version 8. */
    /* The local player's quests (none when quests are switched off), and one by index;
     * getQuest returns 0 for an index past the end. */
    uint32_t (*getQuestCount)(void *context);
    int (*getQuest)(void *context, uint32_t index, AvQuest *out);

    /* Version 9. */
    /* Says a chat line as the local player (to the server when online). Returns 0 when no game
     * is showing. */
    int (*say)(void *context, const char *text);
    /* Adds a line to this machine's chat box only, in (r, g, b) (0..255). */
    int (*showChat)(void *context, const char *text, float r, float g, float b);
} AvHost;

/* What a mod gives the engine. The engine zeroes it and sets `size` before avModLoad. */
typedef struct AvMod {
    uint32_t size;       /* sizeof(AvMod) as the engine knows it; leave it */
    uint32_t apiVersion; /* set to AV_API_VERSION, the header the mod was built with */
    void *user;          /* the mod's own pointer, passed back to its callbacks */

    /* Once per frame, before the frame is drawn; `seconds` is the time since the last one. */
    void (*onFrame)(void *user, double seconds);
    /* Once, before the engine unloads the mod. */
    void (*onShutdown)(void *user);

    /* Version 2. */
    /* A generated zone (256 x 256 columns, zone index zx, zy) has just been taken in. */
    void (*onZoneReady)(void *user, int32_t zx, int32_t zy);

    /* Version 3. */
    /* A hit is about to be applied to the local player. `kind` is the hit kind
     * (alpha-creatures-behaviour.md §5: 0 damage, 1 blocked, 3 immune, 5 absorbed); the mod may
     * change `*amount`. */
    void (*onPlayerHit)(void *user, uint64_t attacker, uint8_t kind, float *amount);
    /* The local player has reached `level`. */
    void (*onLevelUp)(void *user, int32_t level);

    /* Version 4. */
    /* A hit the local player dealt or took has landed (after onPlayerHit). `kind` as above;
     * `amount` is the damage (negative heals). */
    void (*onHitSeen)(void *user, uint64_t attacker, uint64_t target, uint8_t kind, float amount);

    /* Version 5. */
    /* The mission of map cell (cellX, cellY) has just been completed by a kill this engine
     * worked out (offline play, or the server). */
    void (*onMissionComplete)(void *user, int32_t cellX, int32_t cellY);

    /* Version 6. */
    /* A creature this engine runs has died (offline play, or the server); `credited` is the
     * player in its threat map, 0 when none. */
    void (*onCreatureDeath)(void *user, uint64_t id, int32_t type, int32_t level, uint64_t credited);

    /* Version 8. */
    /* The local player handed a quest in to its giver (the reward is given after this call). */
    void (*onQuestComplete)(void *user, uint32_t quest);

    /* Version 9. */
    /* A chat line arrived or was said: `sender` is the player's id (0 for the local player
     * offline), `text` UTF-8. */
    void (*onChat)(void *user, uint64_t sender, const char *text);
    /* The local player received an item in its bags (loot, a gift, a purchase is not one). */
    void (*onItemReceived)(void *user, int32_t type, int32_t subtype, int32_t level);
} AvMod;

typedef int (*AvModLoadFunction)(const AvHost *host, AvMod *mod);

#ifdef __cplusplus
}
#endif

#endif /* ALPHAVULKAN_MOD_H */
