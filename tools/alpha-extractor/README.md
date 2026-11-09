# Alpha extractor

A small 32-bit mod for the original Cube World Alpha that records ground truth from the
running game, so AlphaVulkan's world generator (and later its creatures) can be checked
block for block against Alpha itself. It only reads the game's memory and patches nothing.
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

## Compare

```bat
build\ZoneCompare.exe AlphaExtract\region_512_512.avrg AlphaExtract\*.avzd
build\ZoneCompare.exe --grid AlphaExtract\zone_32800_32800.avzd
build\ZoneCompare.exe --list AlphaExtract\spawns_32800_32800.avsp
build\ZoneCompare.exe --strict --prefix AlphaExtract\spawns_32800_32800.avsp
```

`ZoneCompare` builds the same world (seed and whether it is named come with each recording)
and reports the share of solid blocks that match, the kinds of block that differ and,
with `--grid`, where in the zone they are. Air and water are compared by their light, which
Alpha keeps in their colour bytes (rendering §9.2); near zone edges Alpha's light also takes
in the neighbouring zones (it floods again when chunks are meshed), so a little of it differs.

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
