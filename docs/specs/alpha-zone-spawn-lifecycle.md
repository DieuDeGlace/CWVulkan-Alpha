# Alpha zone spawn-list lifecycle

Status: **FINAL / STABLE / HELD**. Bounded native-source contract; author writes cease after final readback.

## Scope and evidence

This contract distinguishes the baseline prefix established during zone generation, the list present when the generator returns, and a list retained after world updates. These are different native boundaries. Region population can replace and append records after the zone generator has returned. Mission start alone is a different operation and does not append records in the examined transition.

The supported image is the original 32-bit Alpha client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014C`, image base `0x00400000`. All routine identifiers below are RVAs for this image. Server-image equivalence and other Alpha builds remain unverified.

Source paths in citations are relative to the original evidence directory `disassembled alpha cube world`. Original assembly is authoritative where decompiler control flow is incomplete. Reconstruction-owned Alpha files supplied search leads only; no reconstruction statement is used as independent evidence. No replacement engine, exporter, SDK, tests, captures, logs, existing specifications or implementation history were read.

## Native list and boundary facts

The native zone owns a vector of 32-bit pointers to separately allocated spawn records. Its descriptor occupies zone offsets `0x18`, `0x1C` and `0x20`: beginning, logical end and allocation end. The count is the pointer span divided by four. Zone offset `0xA0` is a 32-bit prefix count, initialized to zero. Zone offset `0xA4` is a separate 32-bit population accumulator, also initialized to zero. Neither is the live vector length. Zone coordinates are 32-bit values at `0x60` and `0x64`. [N1, N2]

The generator at RVA `0x1E4850` accepts zone coordinates from zero through 65,535. It obtains the surrounding regions, returns early for an already loaded zone, otherwise constructs a new zone and reseeds the native random stream using the world generation seed and the zone coordinates. The row contribution uses a 16-bit shift. The zone is not published into its region until the end of the routine. [N3]

The following boundaries must remain distinct:

| Boundary | Native position | Meaning |
| --- | --- | --- |
| Prefix established | RVA `0x1EDD5E` | The current vector count is stored at zone `0xA0`. All records already present belong to this prefix. |
| Population adjustment within generation | Call at RVA `0x1EDD64` to `0x1D9460` | The generator immediately performs the population refresh described below. This can append a suffix beyond `0xA0`. |
| Saved zone state applied | RVA `0x1EDE46`, conditionally calling `0x04CDB0` | In a named nonclient world, saved zone state is applied after population adjustment. In particular, saved respawn counters are applied only when the saved spawn count matches the current complete count. |
| Zone published | RVA `0x1EE05A` | The fully processed zone pointer is stored into its region. The remaining normal path returns that zone. |
| Later world update | RVA `0x20C510` | Region population, mission transitions, schedules, timers and creature activation can alter the significance or contents of the retained list. |

Thus a stop at the prefix-count store excludes an operation that is part of normal generation. Conversely, a generator-return list is still not necessarily the list present after a normal world update. The prefix count remains unchanged by the suffix replacement and removal operations examined here. [N3–N8]

## Population refresh and append ordering

RVA `0x1D9460` returns without changing the list when the world name is empty. Otherwise it processes every record from the prefix count to the current end, in increasing list order. For each such record it removes any matching live creature found through the record's 64-bit creature identifier, destroys the spawn record, and eventually truncates the vector to the prefix count. It then consults the zone's eight-by-eight-zone scenery record. [N4]

If that scenery record contains a nonzero monster race and its stored monster zone equals this zone, the routine creates and appends exactly one monster record. Its position is the zone center in the horizontal plane and the top of the center terrain column vertically. Race, level and rarity come from the scenery record's monster state. Mission kind `1` adds the mission-resident appearance flag. Appearance, equipment, inventory and behavior are prepared before the pointer is appended. Otherwise it calls RVA `0x1D6040` with a zero population increment. [N4]

RVA `0x1D6040` can leave the current list unchanged. It requires an existing zone cell of kind zero, excludes the world's spawn-point zone, refuses replacement while a creature represented by any suffix record remains live, excludes a player strictly within 100 blocks of the zone's terrain-centered point, and requires an existing nearest landscape record. It obtains the lowest and highest player levels and draws the new group's level from that inclusive range. These conditions are tested before suffix destruction. [N5]

When this operation proceeds, it adds the supplied increment to zone `0xA4`, destroys the old suffix in increasing order and truncates to the prefix count. It appends a leader first, then its followers in their creation order. After the group is appended, it visits the entire new suffix in list order to assign identifiers, appearance and equipment. The verified direct caller is the refresh fallback, which supplies increment zero; this fallback is reached from generator and later region-refresh paths. This contract does not establish another native caller supplying a nonzero increment. [N4, N5]

The generated 64-bit identifier depends on zone Y, zone X and the current list ordinal. The native arithmetic combines zone Y using radix 65,536, adds zone X, scales by 256, then adds the ordinal. It is arithmetic with carries, rather than a proof that every ordinal fits an eight-bit slot. A retained list's order is therefore significant to identifiers and companion references. This contract does not establish a universal maximum list count. [N4, N5]

These refresh and group-population routines draw from the current native random stream. They do not reseed it by day or by zone. Regenerating a suffix after a later event is therefore not established to reproduce the suffix originally appended during zone generation. [N4–N6]

## Later region and mission operations

RVA `0x1D9AD0` is a later region-population operation. It returns for a client world, an empty world name, or a missing region. Its working inputs include existing scenery and landscape records, the current players and their level range, and the current random stream. It clears prior mission and monster state for eligible scenery records, creates new monster selections and mission candidates, selects up to two scenery missions and up to two monster missions, then refreshes every currently loaded zone in the region through RVA `0x1D9460`. The region's 64-by-64 zone array is traversed in storage order. The region populated flag at `0x15A18` is set only after this pass. Consequently this operation can destroy a generator-return suffix and append a different suffix without rerunning the zone generator. [N6]

The normal update at RVA `0x20C510` performs a nearest-region lookup for each player and invokes this operation when that region exists and its populated flag is clear. This check precedes the nearby mission-start transition. The native region generator at RVA `0x1DA280` can restore saved mission and monster state before any zone is constructed; a successful mission-state restoration sets the region populated flag. Whether that restoration succeeded therefore affects whether the next player update regenerates the region's population. [N7, N9]

Mission creation at RVA `0x1D8750` requires a nonempty world name. It first removes live creatures carrying the mission-resident flag whose home zone belongs to the affected eight-by-eight-zone square. It then destroys and truncates the suffix of every loaded zone in that square. Only afterward does it reset and select mission metadata. The examined routine does not append replacement spawn records itself. When invoked from region population, its list removal precedes the final all-zone refresh described above. [N6, N8]

The mission-start transition in the update requires a nonzero mission kind, progress zero and positive influence at the player's position. It writes progress one and queues mission state. The examined transition does not refresh the zone or append records. It must not be treated as synonymous with region population. [N7]

Mission completion through RVA `0x195A60` writes progress two and destroys every suffix record in the affected square's loaded zones, truncating each list to its prefix count. A normal creature death can instead leave its spawn record present and set its respawn timer to 1,200,000 milliseconds and its recorded death day to the world's current day. List membership and live-creature membership are separate states. [N10]

## Day, activation and record mutation

The update advances the native world clock before the region-population check. On each rollover past 86,400,000 world milliseconds it increments the day, subtracts that interval and calls RVA `0x1F0C80`. That routine clears the populated flag in every loaded region of a nonclient world. It does not itself edit spawn lists. A subsequent qualifying player-region check can then replace suffixes through the ordinary region-population path. Exactly 86,400,000 does not yet take the strict-greater-than rollover branch. [N7, N11]

During the examined active-zone pass, schedule entries whose times are due overwrite the retained spawn's position in their stored order, and the elapsed update interval is subtracted from its respawn timer. During creature activation, equal start/end times impose no time-window exclusion; otherwise the current world time must lie inclusively between them. An existing live creature with the record's identifier prevents another activation. Negative respawn time is clamped to zero; a nonzero timer blocks activation only while its recorded death day equals the current day. Player proximity is also required. These checks consume records already in the vector. [N7]

For mission kind `5`, an eligible retained record can be changed immediately before creature creation. Its level and rarity take mission values except for native hostility value `6`; equipment is rebuilt after reseeding from the mission seed plus the low 32 bits of the record's identifier. Further appearance and inventory adjustments occur in that branch. It mutates the existing spawn; it does not append a new spawn pointer in the examined activation branch. The live creature is a separate allocation. Accordingly, even the prefix's membership can remain stable while its record contents change after generation. [N7]

Relevant native record fields, sufficient to identify the lifecycle decisions above, are:

| Spawn-relative offset | Width | Established role |
| --- | --- | --- |
| `0x10`, `0x18`, `0x20` | Three 64-bit values | Position components |
| `0x28` | 32 bits | Native hostility category |
| `0x2C` | 32 bits | Race identifier |
| `0x34` | 32 bits | Level |
| `0x38` | 32 bits | Respawn time remaining |
| `0x3C` | 32 bits | Recorded death day |
| `0x40`, `0x44` | Two 32-bit values | Activation time limits |
| `0x48` | 64 bits | Creature identifier |
| `0x58` | 8 bits | Rarity |
| `0x7A` | 16 bits | Appearance flags used by the mission branches |

The constructor initializes the timer, death-day and identifier words to zero. The observed allocation size is `0x10F0`; this table is not a complete serialization or replacement type layout. [N4, N7, N10, N12]

## Inputs needed to explain a retained list

A seed and zone coordinate are insufficient to identify an arbitrary retained native list. Its observation boundary must also identify whether the prefix-count store, generator-internal refresh, saved zone application, later region population, mission replacement/completion and activation mutation have occurred.

For the examined operations, the relevant additional state is the empty/nonempty world-name condition; client/server role flag; loaded-region and loaded-zone membership and iteration order; region populated flags; scenery, mission and monster state; current day and world time; player positions and levels; world spawn point; live creature identifiers; retained timers and death days; and the native random stream's current state or the preceding draw/reseed history. The world name's characters are not read by the examined late population routines; its empty/nonempty condition gates them, while the named world's persistence context determines the restored data. [N3–N12]

Within each scenery record, mission seed, kind, race and level occupy 32-bit fields at `0x30`, `0x34`, `0x38` and `0x3C`; rarity and progress are bytes at `0x40` and `0x41`; kill counters occupy `0x44` and `0x48`; target zone coordinates occupy `0x4C` and `0x50`. Monster race and level occupy `0x54` and `0x58`, rarity is the byte at `0x5C`, and its selected zone coordinates occupy `0x60` and `0x64`. The scenery record stride is `0x68`. These native facts explain the refresh and activation decisions; they do not establish a complete mission-generation contract. [N4, N6, N8, N9, N13]

## Explicit limits

This is a lifecycle contract, not a complete recipe for baseline spawn generation, mission selection, random-draw sequences, record serialization, or every world edit capable of creating a spawn. Runtime building placement and its complete effects were not traced. No owner save or runtime observation was used. Thread synchronization, a universal quiescent snapshot point, cross-thread random-state behavior, byte-for-byte repeatability, every possible call path, Server.exe parity and native acceptance remain open. No claim that a replacement engine reproduces any of these behaviors follows from this document.

## Original evidence index

- **N1:** `decompiled/client/functions/006240f0.c`, RVA `0x2240F0`, zone constructor, especially zero-initialized descriptor and count fields.
- **N2:** `decompiled/client/functions/00487f50.c`, RVA `0x087F50`, pointer-vector count; `decompiled/client/functions/0040d9d0.c`, RVA `0x00D9D0`, pointer-vector resize.
- **N3:** `decompiled/client/assembly-functions/005e4850.asm`, RVA `0x1E4850`; entry checks/reseed at RVAs `0x1E489B–0x1E49B5`, prefix/refresh at `0x1EDD53–0x1EDD64`, persistence call at `0x1EDE30–0x1EDE46`, publication at `0x1EE04E–0x1EE05A`.
- **N4:** `decompiled/client/functions/005d9460.c`, RVA `0x1D9460`, entire refresh routine.
- **N5:** `decompiled/client/functions/005d6040.c`, RVA `0x1D6040`, eligibility, suffix destruction, append and final suffix pass.
- **N6:** `decompiled/client/functions/005d9ad0.c`, RVA `0x1D9AD0`, entire region population routine; direct call ordering also visible in `decompiled/client/assembly.txt` at RVAs `0x1DA13F`, `0x1DA1A7`, `0x1DA1D9`.
- **N7:** `decompiled/client/assembly-functions/0060c510.asm`, RVA `0x20C510`; clock/rollover `0x20C678–0x20C6E2`; region check `0x20CB92–0x20CBB7`; mission start `0x20CC48–0x20CCE0`; schedules/timers `0x20DF20–0x20DFA2`; activation and mission record mutation `0x20FBBB–0x20FFBA`.
- **N8:** `decompiled/client/functions/005d8750.c`, RVA `0x1D8750`, mission creation and preceding removals.
- **N9:** `decompiled/client/functions/005da280.c`, RVA `0x1DA280`, named nonclient mission/monster restoration and populated-flag write.
- **N10:** `decompiled/client/functions/00595a60.c`, RVA `0x195A60`, mission completion suffix truncation and spawn timer/day writes.
- **N11:** `decompiled/client/functions/005f0c80.c`, RVA `0x1F0C80`, clearing loaded-region populated flags.
- **N12:** `decompiled/client/functions/005adb20.c`, RVA `0x1ADB20`, spawn constructor; allocation size and field uses also in N4 and N5.
- **N13:** `decompiled/client/functions/0044cb50.c`, RVA `0x04CB50`, native mission-state restoration; `decompiled/client/functions/0044cce0.c`, RVA `0x04CCE0`, native monster-state restoration; `decompiled/client/functions/0044cdb0.c`, RVA `0x04CDB0`, count-checked restoration of retained spawn timer/day pairs.

Final author state: **FINAL / STABLE / HELD**.
