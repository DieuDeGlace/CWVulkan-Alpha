# Alpha extractor

A small 32-bit mod for the original Cube World Alpha that records ground truth from the
running game, so AlphaVulkan's world generator (and later its creatures) can be checked
block for block against Alpha itself. It only reads the game's memory and patches nothing,
except in the optional light-write trace below.
One test helper writes: `AlphaExtract	eleport.now` holding `x y z` (blocks) moves the local
player, to reach a dungeon or a far zone in a copy of the game.
Where everything lives in Alpha's memory is written down in
`docs/specs/alpha-extractor.md`.

## Build

```bat
tools\alpha-extractor\build.cmd
```

It needs the Visual Studio 2022 C++ tools (the 32-bit compiler); the DLL lands in
`build\alpha-extractor\AlphaExtractor.dll`.

## Use

1. Use a **copy** of the Alpha install, not your own game folder.
2. Put `AlphaExtractor.dll` in its `Mods` folder and start the game through the mod launcher
   (`Cube-World-Mod-Launcher.exe`).
3. When the world you want is loaded (the start screen's world counts too), press **F9** in the
   game window, or create the empty file `AlphaExtract\dump.now` beside `Cube.exe`.
4. The recordings appear in `AlphaExtract\`:
   * `zone_<zx>_<zy>.avzd`: every loaded zone within 3 of the player, block for block;
   * `spawns_<zx>_<zy>.avsp`: each zone's spawn objects (where its creatures come from);
   * `contents_<zx>_<zy>.avzc`: each zone's sprites, statics and world items;
   * `region_<rx>_<ry>.avrg`: their regions' location records and per-zone tables;
   * `creatures.avcr`: every creature's id and record (`CreatureCompare seed named|unnamed
     creatures.avcr` checks the ones that came from spawns against ours);
   * with `AlphaExtract\track.now` instead: `track.avct`, every creature ten times a second for
     20 s (compare with `CreatureCompare seed named --simulate x y z`);
   * `capture.txt` and `extractor.log`: what was recorded, and why a dump was refused.

## Light-write trace

To record the order in which the original client writes terrain light, create
`AlphaExtract\lighttrace.txt` **before** starting the game. Each line is one probe cell,
`X Y Z` in whole world cells (up to eight). The trace installs only when the DLL loads
before the game's threads start, and only on the supported `Cube.exe` (SHA-256 `84a7a132…30bf`).
It adds passive observation points to the game's code, at the places listed in
`docs/specs/alpha-native-light-write-observation-points.md`.

* Every dump (F9 or `dump.now`) also writes `lighttrace.avlt`, with the dump's start and end
  marked in the same order as the light events. `lighttrace.save` writes it without a dump.
* `lighttrace.probes` (same format as `lighttrace.txt`) replaces the probe list from then
  on. Up to 16 lists are kept, and each event records which list its probe bytes belong to.
* `python tools\alpha-extractor\ReadLightTrace.py AlphaExtract\lighttrace.avlt` summarizes it;
  `--events` lists every event and `--probe K` lists the events at which probe K changed.

Each event has a global sequence number, thread, timestamp, kind, eight values and the four
stored bytes of each probe (when stored at that moment). The values by kind:

| Kind | Values |
| --- | --- |
| light-enter | caller, low X, low Y, high X, high Y, margin, explicit zone |
| light-precommit, light-exit | caller, unknown, low Y, high X, high Y, margin, explicit zone (low X comes from the same thread's light-enter) |
| admit-yes, admit-no | chunk X, chunk Y, resident 3×3 zones (bit 3(dx+1)+(dy+1)), containing zone X, Y, earlier identical denials skipped |
| build-locked | chunk X, chunk Y, cache slot, the slot's dirty byte at that moment |
| relight-chosen | chunk X, chunk Y, dirty byte used, chunk origin X, origin Y |
| build-light-done | chunk X, chunk Y |
| step-test | route (0 bomb, 1 building, 2 variable radius), world +0xB8, model (building) |
| cell-write, clear | X, Y, Z, new record (cell-write), explicit zone, caller |
| cell-write-exit, clear-stored | probe bytes after the write |

Zone presence at admission is re-read without a lock, so it can differ from what the game saw
(see the contract's safety limits). Writes into an unpublished zone during generation are not
recorded. The buffer holds 262,144 events; later events are counted as dropped.

## Compare

```bat
build\ZoneCompare.exe AlphaExtract\region_512_512.avrg AlphaExtract\*.avzd
build\ZoneCompare.exe --grid AlphaExtract\zone_32800_32800.avzd
build\ZoneCompare.exe --list AlphaExtract\spawns_32800_32800.avsp
build\ZoneCompare.exe --strict --prefix AlphaExtract\spawns_32800_32800.avsp
```

`ZoneCompare` builds the same world (seed and whether it is named come with each recording)
and reports the share of solid blocks that match, the kinds of block that differ and,
with `--grid`, where in the zone they are. Air and water both store the last copied light
output in their first byte. The next two bytes also serve as propagation scratch.
The comparator reports scratch differences separately. Its default compares the recorded
light with fresh generation. The `--edges` option models static floods with all eight
neighbours present. A live capture has its own residency and mesh history.

The recorder holds the world's C0 lock while it copies resident columns. A mesh worker
can update their light during that copy. AVZD stores the bytes observed without a frame
number or lighting phase. The `zones` line in new manifests counts resident zones sampled.
Earlier manifests counted the buffered zone and auxiliary files together. Inventory an
older recording from its actual AVZD files. See the [live column observation contract](../../docs/specs/alpha-live-column-observation-locks.md).

Use `--strict` for a failing exit code on compared differences; plain reporting
mode only requires a readable recording. The current writer uses AVSP version 2:
after the version, seed, named-world flag and zone coordinates, it writes the full
count followed by the native recorded generation-prefix count, then all rows.
Version 1 omitted that prefix count. `--prefix` accepts version 2 only, compares
the explicit prefix and still validates every suffix record and trailing bytes.
It does not infer a prefix from a shorter list or promise that later simulation
leaves every prefix record unchanged. See the held zone-spawn lifecycle contract.

The asset setup/import tool for players is `AlphaSetup`; this native recorder is
a separate developer tool and is not included in runtime packages.

For a named world with towns, join an `AlphaServer` from the Alpha copy (online worlds are
named) and walk to a town; `ZoneDump seed x y locations` lists where the towns are (type 1).

Recordings are output of the original game: keep them out of this repository.
