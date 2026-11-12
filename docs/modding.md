# Modding AlphaVulkan

AlphaVulkan loads mods the way OpenMW does content files: nothing is loaded unless you list
it, and the list sets the order. Mods never change your Cube World Alpha install.

## Turning mods on

Put each mod in its own folder under `Mods\` beside `AlphaVulkan.exe`, then list the folders
in `AlphaVulkan.ini`:

```ini
[Mods]
Load=BetterSky, TimeLapse
```

Later mods win where two mods change the same thing. The log (standard error) says which mods
loaded and why any did not.

## What a mod folder holds

```
Mods\MyMod\
  mod.ini          [Mod] Name, Version, Description, Plugin
  content\         files that replace or add content
  MyMod.dll        optional code (the Plugin named in mod.ini)
```

### Content

Alpha's content lives in four stores in the install (`docs/specs/alpha-data-stores.md`), each
item named by a key such as `airship.cub`, `menu-select.wav`, `dict_en.xml`. A file in a mod's
`content\` folder with the same name replaces that item; a new name adds one. Files are plain,
not scrambled: a `.cub` is the usual voxel model layout, a `.wav` an ordinary WAV.

Mods must not contain Cube World's own files or edits of them: make your own models, sounds
and text.

### Code

A plugin is a DLL that includes `sdk/alphavulkan_mod.h` and exports

```c
int avModLoad(const AvHost *host, AvMod *mod);
```

The engine calls it once at start-up with a table of functions it may call (log, read content,
play a sound, get and set the time of day) and an `AvMod` for the mod to fill with callbacks
(`onFrame`, `onShutdown`). Return 1 to load, 0 to decline. The interface is plain C and only
grows at the end of each struct, so a mod built today keeps working with later engines;
`apiVersion` says which version the engine speaks.

`examples/mods/TimeLapse` is a complete plugin (one game hour per real second). The build
makes it into `build\Mods\TimeLapse\`; try it with `Load=TimeLapse`.

Build a plugin as a 64-bit DLL, like the engine.

What each version added:

| Version | Engine offers (`AvHost`) | Mod may set (`AvMod`) |
|---|---|---|
| 1 | `log`, `readContent`, `playSound`, `getTimeOfDay`, `setTimeOfDay` | `onFrame`, `onShutdown` |
| 2 | `getBlock` | `onZoneReady` |
| 3 | `getPlayer` (position, health, level, experience) | `onPlayerHit` (may change the damage), `onLevelUp` |
| 4 | `showText` (floating text), `showBanner` (land-name banner), `getStaticCount` / `getStatic` (a zone's doors, chests, stands...) | `onHitSeen` (hits the player dealt or took) |
| 5 | `getMission` (a map cell's mission), `setBlock` (a block edit, kept and saved like the game's own; offline only) | `onMissionComplete` (a mission finished by a kill) |
| 6 | `getCreatureCount` / `getCreature` (the creatures held: id, type, controller, level, health, position) | `onCreatureDeath` (a creature this engine runs died, with the credited player) |
| 7 | `spawnParticles` (a burst of particle cubes, shown on this machine only) | |
| 8 | `getQuestCount` / `getQuest` (the local player's quests: kind, state, giver, progress, place) | `onQuestComplete` (a quest handed in to its giver) |
| 9 | `say` (a chat line as the player), `showChat` (a line on this machine only) | `onChat` (a line arrived or was said), `onItemReceived` (an item into the bags) |
| 10 | `tell` (a chat line to one player), `getPlayerName` (a player's character name) | `onPlayerJoin` / `onPlayerLeave` (the dedicated server), `onCreatureTamed` (a creature tamed, with its owner) |

### On the dedicated server

AlphaServer loads the mods in its own `AlphaVulkan.ini` [Mods] Load the same way. From version
10 every callback there runs on the server's simulation thread with no engine lock held, so a
callback may call any host function:

- callbacks: `onFrame` (once per world step, about every 20 ms), `onPlayerJoin`,
  `onPlayerLeave`, `onChat` (each player's line, with the sender's id), `onCreatureDeath`,
  `onMissionComplete`, `onCreatureTamed`; events of a step reach the mods in the order they
  happened, after the step;
- host functions: `log`, `readContent`, `getTimeOfDay` / `setTimeOfDay` (the world clock all
  players get), `getBlock` / `setBlock` (a block edit sent to every player and saved with its
  zone), `getCreatureCount` / `getCreature` (connected players first, then the NPCs),
  `say` (a line to everyone, from the server), `tell` (a line to one player) and
  `getPlayerName`. The others (the local player, the screen, sound, particles, quests,
  missions) answer 0 on the server.

Inside the engine, `creatures::itemBonuses()` is the hook for item stat formulas (what a
weapon or armour piece adds to health, damage, armour, resistance, speed and critical
chance); it will be offered through the C interface once the item records are in.

## Source modules compiled into the engine

Source modules are linked into the 64-bit engine library, client and dedicated
server. They use C++20 and the engine's types directly; no DLL injection or binary
hooks are involved. The C ABI above remains version 9. The separate source SDK is
`sdk/alphavulkan_engine_module.h` (source API version 1) and is rebuilt against the
same engine source and compiler.

`examples/modules/StoneTint/StoneTint.cpp` is a working example. It registers a
terrain pass that colors exposed natural rock blue, preserving material flags
and protected construction. It is compiled by default and inactive until enabled:

```ini
[Mods]
Load=StoneTint, TimeLapse
```

Compiled IDs and runtime mod folders share this explicit load order. A compiled
ID is reserved by the build and takes precedence over a folder with that name;
duplicates load once. Shutdown runs in reverse order and removes the example's
terrain pass. StoneTint needs neither a mod folder nor original assets. Disable
its build with `-DALPHAVULKAN_EXAMPLE_MODULE=OFF`.

An external source module supplies a CMake directory containing, for example:

```cmake
alpha_add_engine_module(MyTerrain VERSION 1.0 MyTerrain.cpp MyTerrain.h)
```

Configure with `-DALPHAVULKAN_MODULE_DIRS="path/to/MyTerrain"` (a semicolon-separated
list for several). Each ID must be unique and use letters, digits or underscores,
starting with a letter. `VERSION` defaults to `1` and is included in saved-world
profile checks. List each source and private header used by the module. Export
its lifecycle entry using the definition provided
by that CMake function:

```cpp
extern "C" int AV_MODULE_ENTRY(const AvHost *host, AvMod *mod);
```

The entry has the same load/shutdown contract as a DLL, with access to C++ engine
services. Register a named `av::modules::TerrainPass` at startup and retain its
registration token for `unregisterTerrainPass` at shutdown. Names must be unique.
Passes run in registration order after native generation, before the swimmable-water
extension and lighting. They can change the supplied zone's blocks, plants, items,
statics and spawn records directly through `worldgen/Zone.h`.

Generation can run on several threads. Own only the supplied zone, use deterministic
seed/coordinate-derived decisions, avoid native random-stream consumption, and
do not register or unregister from inside a pass. Unregister waits for active passes
to finish before releasing callback state. Load modules before starting world jobs
and shut them down after stopping those jobs. Engine source APIs can change between
revisions; rebuild modules with the matching engine. Source modules are trusted
engine code and must obey its ownership/threading rules. Callbacks must not throw;
a module that declines startup must release any registrations it already acquired.

AlphaServer reads `[Mods] Load` from `AlphaVulkan.ini` in its `--folder` and uses
the same compiled terrain passes and content overrides. Its generic host currently
provides logging and content reads; client UI, local-player and frame callbacks
are not wired into dedicated-server gameplay. All peers need matching terrain
modules and settings; automatic module/profile negotiation is still open.
Saved worlds pin the ordered mod IDs, declared versions and input fingerprints
along with their water policy. A folder mod's fingerprint covers `mod.ini`, its
declared plugin DLL and every flat file in `content/`, including the file names.
Subdirectories are excluded because the content loader does not read them.
Enumeration order and the installation path do not affect the fingerprint.
Fingerprinting runs at startup; restart the engine after editing a mod.

A compiled source module fingerprints its explicitly listed sources/headers, the
two SDK headers and compiler identity/version. CMake automatically reconfigures
when a listed input changes. Text line endings are normalized, and checkout paths
are excluded. Unlisted headers, indirect DLLs, external files, custom compile
definitions/options and wider engine changes are outside this bounded identity;
bump the declared version when those change terrain semantics. A fingerprint is
a compatibility input check, not a proof of identical execution or a signature.

Changed inputs are rejected before opening a saved world for writing. Restoring
the exact setup reopens it; incompatible generation needs a new world. Older
modded profiles that lack fingerprints require a separate migration, which is not
implemented. Unmodded profiles keep their existing identity. Cross-peer profile
negotiation also remains open. Failed plugin startup leaves no content overrides;
shutdown removes each mod's own layer in reverse load order after its callback.
The supplied Content must outlive the loader, and registration changes must occur
outside content/world workers.

## Further API coverage

Further typed source services and C SDK services for creatures, items, the interface
and custom network messages remain work in progress,
each as new members at the end of `AvHost` and `AvMod`.
