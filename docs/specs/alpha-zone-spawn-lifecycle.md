# Alpha zone spawn-list lifecycle

Status: **FINAL / STABLE / HELD**. Bounded native-source contract, re-certified 2026-11-22; author writes cease after final readback.

## Scope and evidence

This contract separates three native boundaries: the baseline prefix fixed during zone generation, the list present when the zone generator returns, and a list retained after later world updates. Region population can destroy and replace records after the zone generator has returned. Mission start alone is a different operation and does not add records in the examined transition.

The supported image is the original 32-bit Alpha client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014C`, image base `0x00400000`. All routine identifiers below are RVAs for this image. Equivalence with `Server.exe` and with other Alpha builds is unverified.

Citation paths are relative to the original evidence directory `disassembled alpha cube world`. Where recovered high-level listings and original assembly disagree or are incomplete, the assembly is authoritative. No reconstruction source was needed for this contract and none is relied on. No project implementation, exporter, SDK, verification material, captures, logs, other specifications or implementation history were read.

## Native list and boundary facts

Each native zone holds a growable array of 32-bit pointers to separately allocated spawn records. The array's three bounding pointers sit at zone offsets `0x18` (first element), `0x1C` (one past the last element) and `0x20` (one past the reserved capacity). The element count is the byte distance between the first two divided by four. Zone offset `0xA0` holds a 32-bit prefix count and zone offset `0xA4` a separate 32-bit population accumulator; the zone constructor sets both to zero, and neither is the live element count. Zone X and zone Y are 32-bit values at `0x60` and `0x64`. The zone object occupies `0xC8` bytes. [N1, N2, N3]

The zone generator at RVA `0x1E4850` accepts zone X and zone Y each from zero through 65,535 and yields no zone for any value outside that range. It first runs the region generator (RVA `0x1DA280`) over the three-by-three block of regions surrounding the zone's region, then returns the existing zone unchanged if one is already loaded. Otherwise it allocates and constructs a new zone and reseeds the native random stream with the world generation seed plus zone Y shifted left by 16 bits plus zone X. The new zone is not stored into its region until near the end of the routine. [N3]

The following boundaries must remain distinct:

| Boundary | Native position | Meaning |
| --- | --- | --- |
| Prefix fixed | RVA `0x1EDD5E` | The element count at that moment is stored at zone `0xA0`. Every record already present belongs to the prefix. |
| Population refresh inside generation | Call at RVA `0x1EDD64` to RVA `0x1D9460` | Performed immediately after the prefix store. It can add records beyond the prefix. |
| Saved zone state applied | Call at RVA `0x1EDE46` to RVA `0x04CDB0` | Only in a nonclient world with a nonempty world name, and only when a saved row for this zone is found. It runs after the refresh. Saved respawn timer and death-day pairs are written only when the saved record count equals the complete current element count. |
| Zone published | RVA `0x1EE05A` | The processed zone pointer is stored into its region's 64-by-64 zone table (region offset `0x10018`). The normal path then returns that zone. |
| Later world update | RVA `0x20C510` | Region population, mission transitions, schedules, timers and creature activation can change the contents or meaning of the retained list. |

A list observed at the prefix store therefore omits work that is part of normal generation, and a list observed at generator return need not match the list after a later update. None of the removal or replacement operations examined here changes the prefix count. [N3–N8]

When the saved zone state is applied, the timer and death-day words are restored to records by list position, in increasing order, from consecutive saved 32-bit pairs. If the counts differ, no record's timer or death day is restored. [N3, N13]

## Population refresh and append ordering

The refresh at RVA `0x1D9460` does nothing when the world name is empty. Otherwise, for every record from the prefix count up to the current end, in increasing list order, it looks up a live creature by the record's 64-bit creature identifier, destroys and unregisters that creature if present, and destroys the record. It then shortens the array to the prefix count. Next it locates the scenery record for the zone's eight-by-eight-zone square in the owning region. [N4]

If the region and scenery record exist, the scenery record's monster race is nonzero, and both of its stored monster-zone coordinates equal this zone's coordinates, the refresh creates exactly one monster record and appends it. The record gets hostility category `1`. Its horizontal position is the zone centre (zone coordinate times 256 plus 128, in blocks), and its vertical position is the top of the terrain column at that point. Race, level and rarity come from the scenery record's monster fields. If the scenery record's mission kind is `1`, appearance flag `0x2000` is set. Equipment rarity draws (four random draws in total), appearance, equipment and behavior data are prepared, and the creature identifier is assigned, all before the pointer is appended. In every other case the refresh calls RVA `0x1D6040` with a population increment of zero. [N4]

The group population routine at RVA `0x1D6040` can leave the list unchanged. Its checks run in this order: the zone must have an existing zone cell whose kind byte is zero; the zone must not be the world spawn-point zone; no record in the current suffix may still have a live creature; no creature whose category byte (creature offset `0x60`) is zero may be strictly within 100 blocks of the zone-centre point at terrain height (the squared distance in blocks must be at least 10,000); and the nearest record found by the world lookup at RVA `0x077E10` must exist. If any check fails, the routine returns before drawing from the random stream. It then takes the lowest and highest level (creature offset `0x190`) among those category-zero creatures and draws the group level uniformly from that inclusive range. [N5]

After that level draw, the routine adds the supplied increment to zone `0xA4`, destroys the old suffix records in increasing order and shortens the array to the prefix count. It appends a leader record first and then each follower in creation order. When the group is complete it walks the whole new suffix in list order, reassigning each identifier and rebuilding appearance and equipment. In the recorded call graph the only caller is the refresh fallback above, which passes zero; that fallback is reached from the generator and from region population. No native caller passing a nonzero increment has been established. [N4, N5]

A generated 64-bit creature identifier depends on zone Y, zone X and the record's list position: zone Y is weighted by 65,536, zone X is added, the sum is multiplied by 256, and the list position is added, all as 64-bit arithmetic with carry. Because this is arithmetic rather than packed bit fields, a position of 256 or more carries into the zone part. The order of a retained list therefore matters to identifiers and to anything that refers to records by position. No universal maximum list length has been established. [N4, N5]

The refresh and group-population routines draw from whatever the native random stream currently holds; they do not reseed it by day or by zone. A suffix regenerated after a later event therefore cannot be assumed to reproduce the suffix that zone generation appended. [N4–N6]

## Later region and mission operations

Region population at RVA `0x1D9AD0` returns immediately for a client world, an empty world name, or a region that is not loaded. Its inputs include the region's scenery and nearest-record data, the levels of category-zero creatures, and the current random stream. For each of the region's eight-by-eight scenery records, in storage order, that maps back to this region, it clears any prior mission kind and kill counter (recording the previous mission state in a history list), clears any prior monster race, level and rarity, and, when the record's scenery kind is zero, draws a new monster level, race and monster zone. Scenery records of qualifying kind and level become mission candidates in one of two pools split by level. The routine then makes two mission-creation attempts drawn from those pools and up to two further mission creations from the squares given new monsters, each through RVA `0x1D8750`. Finally it refreshes every loaded zone in the region through RVA `0x1D9460`, scanning the region's 64-by-64 zone table in storage order, and only then sets the region populated flag at region offset `0x15A18`. This operation can therefore destroy a suffix appended at generation and append a different one without the zone generator running again. [N6]

In a nonclient world, the update at RVA `0x20C510` finds the nearest loaded region to a player's position and runs region population if that region exists and its populated flag is clear. This check precedes the mission-start transition. The region generator at RVA `0x1DA280`, which the zone generator runs before constructing any zone, can restore saved mission and monster state for scenery records; whenever a saved mission row is restored it also sets that region's populated flag. Whether that restoration happened therefore decides whether the next update repopulates the region. [N7, N9]

Mission creation at RVA `0x1D8750` does nothing when the world name is empty. It first destroys every live creature that has mission-resident appearance flag `0x2000` set and whose stored home zone lies in the affected eight-by-eight-zone square. It then destroys the suffix records of every loaded zone in that square and shortens each array to its prefix count. Only after that does it reset and select the mission fields. It does not append spawn records itself. When called from region population, this removal happens before the closing all-zone refresh. [N6, N8]

The mission-start transition in the update needs a nonzero mission kind, progress zero, and a strictly positive influence value at the player's position. It sets progress to one and queues the mission state. It neither refreshes the zone nor appends records, and must not be equated with region population. [N7]

At mission completion, RVA `0x195A60` sets progress to two and destroys every suffix record in the loaded zones of the affected square, shortening each array to its prefix count. Live creatures are not removed by that step. For a dying creature, the same routine then uses the creature's stored home-zone coordinates (creature offsets `0x1B0`, `0x1B4`) and stored list position (creature offset `0x1B8`) to find its record; if that zone is loaded and the position is within the current count, the record stays in the list with its respawn timer set to 1,200,000 milliseconds and its death day set to the current world day. If the position is out of range, which is possible after a truncation, no record is updated. Membership in the list and the existence of a live creature are separate states. [N10]

## Day, activation and record mutation

The update advances the world clock before the region-population check. The world time grows by the elapsed interval times 100 or times 10, depending on a per-update flag; in a world with an empty name it is instead set to 32,400,000. While the world time is strictly greater than 86,400,000 milliseconds, the update increments the day, subtracts 86,400,000 and calls RVA `0x1F0C80`. In a nonclient world that routine clears the populated flag of every loaded region; it does not edit spawn lists. A later qualifying region check can then replace suffixes through region population. A time of exactly 86,400,000 does not roll over. [N7, N11]

During the examined active-zone pass, each schedule entry of a record whose time has been reached overwrites the record's position, entries being applied in stored order, and the elapsed update interval is then subtracted from the record's respawn timer. [N7]

Creature activation checks each record in list order, in this sequence. If the two activation time limits are equal there is no time window; otherwise the world time must lie between them, inclusive. A live creature already carrying the record's identifier blocks activation. A negative respawn timer is set to zero. A nonzero timer blocks activation only while the record's death day equals the current day. Finally, some player must be strictly inside the record's activation radius (squared distance less than the squared radius). These checks act only on records already in the array. [N7]

Immediately before creature creation, if the scenery record looked up for the zone being processed has mission kind `5` and the record has appearance flag `0x1000` set, the record itself is altered. Unless its hostility category is `6`, its level and rarity are replaced by the mission level and rarity. The random stream is reseeded with the mission seed plus the low 32 bits of the record's identifier, and equipment is rebuilt. Further appearance, equipment-tier and inventory-rarity changes follow in that branch, some of them random. The existing record is changed in place and no pointer is appended; the live creature is a separate `0x1E60`-byte allocation. So record contents, even in the prefix, can change after generation while list membership stays the same. [N7]

The native record fields needed for the decisions above are:

| Spawn-relative offset | Width | Established role | Constructor value |
| --- | --- | --- | --- |
| `0x08` | 32-bit float | Activation radius | 200.0 |
| `0x10`, `0x18`, `0x20` | Three 64-bit values | Position components (block coordinate times 65,536) | Not set by constructor |
| `0x28` | 32 bits | Hostility category | 1 |
| `0x2C` | 32 bits | Race identifier | 4 |
| `0x34` | 32 bits | Level | 1 |
| `0x38` | 32 bits | Respawn time remaining, milliseconds | 0 |
| `0x3C` | 32 bits | Recorded death day | 0 |
| `0x40`, `0x44` | Two 32-bit values | Activation time limits | 0, 0 |
| `0x48` | 64 bits | Creature identifier | 0 |
| `0x58` | 8 bits | Rarity | 0 |
| `0x7A` | 16 bits | Appearance flags used by the mission branches | Not established here |

The record allocation is `0x10F0` bytes. This table is not a complete record layout or serialization format. [N4, N5, N7, N10, N12]

## Inputs needed to explain a retained list

A seed and a zone coordinate are not enough to identify an arbitrary retained native list. The observation point must also say whether the prefix store, the in-generation refresh, saved-state application, later region population, mission creation or completion, and activation-time record changes have happened.

For the operations examined, the further state that matters is: whether the world name is empty; the client/nonclient flag; which regions and zones are loaded and their iteration order; region populated flags; scenery, mission and monster fields; current day and world time; player positions and levels; the world spawn point; live creature identifiers; retained timers and death days; and the random stream's current state or the full history of draws and reseeds. The late population routines test only whether the world name is empty, not its characters; the named world's saved data decides what is restored. [N3–N13]

Within each scenery record, the mission seed, kind, race and level are 32-bit fields at `0x30`, `0x34`, `0x38` and `0x3C`; mission rarity and progress are bytes at `0x40` and `0x41`; the kill counters are 32-bit fields at `0x44` and `0x48`; the target coordinates are 32-bit fields at `0x4C` and `0x50`. The monster race and level are 32-bit fields at `0x54` and `0x58`, monster rarity is the byte at `0x5C`, and the monster-zone coordinates are 32-bit fields at `0x60` and `0x64`. Records are `0x68` bytes apart, starting at region offset `0x14018`. These facts explain the refresh and activation decisions; they are not a complete mission-generation contract. [N4, N6, N8, N9, N13]

## Unresolved

- The identity of the zone cell kind tested by RVA `0x1D6040` and of the world record returned by RVA `0x077E10` is known only by position and use; their wider meaning is unconfirmed.
- Creature category byte zero is treated here as the player category on the strength of consistent use; that meaning was not separately traced.
- Exactly which players the update iterates for the region and mission-start checks (local player only, or all connected players) was not traced.
- The flag that selects the times-100 or times-10 clock rate, and the exact set of zones visited by the active-zone schedule and timer pass, were not traced.
- The full set of equipment, appearance and inventory changes in the mission kind `5` activation branch, and the exact draw count in the monster-record and group-creation paths, are not specified here.
- Whether mission completion and the death timer write always both run for the same death, and the complete set of mission kinds that end in completion, were not exhaustively traced.
- The complete layout of the saved zone row that comes before the timer and death-day pairs is outside this contract.

## Explicit limits

This is a lifecycle contract. It is not a complete recipe for baseline spawn generation, mission selection, random-draw sequences, record serialization, or every world edit that can create a spawn. Runtime building placement and its full effects were not traced. No owner save or runtime observation was used. Thread synchronization, a single quiescent snapshot point, cross-thread random-stream behavior, byte-for-byte repeatability, every possible call path, `Server.exe` parity and native acceptance remain open. Nothing in this document claims that any replacement program reproduces these behaviors.

## Original evidence index

- **N1:** `decompiled/client/functions/006240f0.c`, RVA `0x2240F0`, zone constructor; zero-initialized array bounds and the `0xA0` and `0xA4` words.
- **N2:** `decompiled/client/functions/00487f50.c`, RVA `0x087F50`, pointer-array count; `decompiled/client/functions/0040d9d0.c`, RVA `0x00D9D0`, pointer-array resize.
- **N3:** `decompiled/client/assembly-functions/005e4850.asm`, RVA `0x1E4850`; range checks, neighbouring region generation, loaded-zone check, `0xC8`-byte allocation and reseed at RVAs `0x1E489B–0x1E49B5`; prefix store and refresh at `0x1EDD53–0x1EDD64`; client, world-name and saved-row gates and the restoration call at `0x1EDD69–0x1EDE46`; publication at `0x1EE04E–0x1EE05A`.
- **N4:** `decompiled/client/functions/005d9460.c`, RVA `0x1D9460`, entire refresh routine.
- **N5:** `decompiled/client/functions/005d6040.c`, RVA `0x1D6040`, eligibility checks, level draw, suffix destruction, appends and final suffix pass; append call sites in `decompiled/client/assembly.txt` at RVAs `0x1D7514` and `0x1D7937`; distance scale and threshold constants at image addresses `0x006FCD94` (1/65,536) and `0x006FD908` (10,000.0); nearest-record lookup at RVA `0x077E10` (`decompiled/client/functions/00477e10.c`); sole recorded caller in `decompiled/client/calls.csv` at RVA `0x1D9AA7`.
- **N6:** `decompiled/client/functions/005d9ad0.c`, RVA `0x1D9AD0`, entire region-population routine; mission-creation calls, the closing refresh and the populated-flag store in `decompiled/client/assembly.txt` at RVAs `0x1DA13F`, `0x1DA1A7`, `0x1DA1D9` and `0x1DA1EF`; level-split candidate pools at `0x1D9FAE–0x1D9FF7`.
- **N7:** `decompiled/client/assembly-functions/0060c510.asm`, RVA `0x20C510`; clock and rollover at `0x20C678–0x20C6E2`; nonclient gate and nearest-region check at `0x20CB1B–0x20CBB7`, with the lookup at RVA `0x1CADE0` (`decompiled/client/functions/005cade0.c`); mission start at `0x20CC48–0x20CCE0`, with the influence threshold at image address `0x00745D84` (0.0); schedules and timers at `0x20DF20–0x20DFA2`; activation checks and the mission-kind `5` record change at `0x20FBBB–0x20FFBA`.
- **N8:** `decompiled/client/functions/005d8750.c`, RVA `0x1D8750`, mission creation and the removals before it.
- **N9:** `decompiled/client/functions/005da280.c`, RVA `0x1DA280`, mission and monster restoration in named nonclient worlds, and the populated-flag write.
- **N10:** `decompiled/client/functions/00595a60.c`, RVA `0x195A60`, mission completion, suffix truncation, and the timer and death-day writes located by the creature's stored list position.
- **N11:** `decompiled/client/functions/005f0c80.c`, RVA `0x1F0C80`, clearing the populated flag of loaded regions.
- **N12:** `decompiled/client/functions/005adb20.c`, RVA `0x1ADB20`, spawn constructor defaults; allocation size also visible in N4 and N5.
- **N13:** `decompiled/client/functions/0044cb50.c`, RVA `0x04CB50`, mission-state restoration; `decompiled/client/functions/0044cce0.c`, RVA `0x04CCE0`, monster-state restoration; `decompiled/client/functions/0044cdb0.c`, RVA `0x04CDB0`, saved zone state, including the count-checked restoration of timer and death-day pairs.

## Re-certification (2026-11-22)

A fresh source-only author, working without any project implementation material, re-checked this document statement by statement against the original client image and its disassembly. The image identity, routine RVAs, boundary order, offsets, widths and constants were confirmed. Several statements were made more precise: the saved-state step also needs a saved zone row to be found; the distance checks are strict and use creature category byte zero; activation needs the record's float radius at offset `0x08`; the mission kind `5` branch also needs appearance flag `0x1000`; the death timer write finds its record by the creature's stored list position; region population is gated to nonclient worlds in the update and draws its mission candidates from two level-split pools; and the clock is forced to a fixed time in a world with an empty name. Earlier wording that echoed recovered listings was rewritten as plain prose. The items in the Unresolved section and the Explicit limits remain open.

Final author state: **FINAL / STABLE / HELD**.
