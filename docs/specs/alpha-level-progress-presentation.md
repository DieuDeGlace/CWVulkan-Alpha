# Alpha level and XP presentation at the observation boundary

Source-side contract, 2026-11-21. **FINAL / STABLE / HELD** after complete author readback.

The supported image is the original PE32 client `binary/Cube.exe`, preferred base `0x00400000`,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are RVAs. This record covers what the identified update block sends to the
presentation system, and when it publishes its observations.

## Admission and the three subsequent samples

The update reads the selected creature through client `+0x8006D0`. Current level and XP are
32-bit fields at creature `+0x190` and `+0x194`. Their observation trackers are at `+0x1D3C`
and `+0x1D40`. At `0x093D72–0x093D84` the update tests level equality first. Only equality
reaches `0x093ED5–0x093EE7`, where a **signed** comparison admits an XP notification only
when current XP exceeds tracked XP. A tie or decrease skips both XP outputs and the tracker
write. Merely reaching this block with a lower XP value does not lower its comparison baseline.

On an admitted XP increase, the following operations occur in order.

1. The update prepares the floating-text record described below and copies a freshly selected
   creature's position into it. At `0x093FBE–0x093FD8` it reloads the selected creature and
   subtracts tracked XP from current XP. It attaches the literal `XP +` followed immediately
   by that signed decimal difference to the record. There is one space before the plus sign
   and no trailing newline. At `0x094031` it appends a copy to the list at client `+0x800738`.
2. The update clears the text stream's contents. At `0x094077–0x09408F` it reloads the selected
   creature and calculates current XP minus tracked XP again. It assembles `You gain `, the
   signed decimal difference, ` XP.` and a newline. At `0x094104` it passes that line and the
   three color bytes in RGB order **(150, 128, 255)** to the message widget at client `+0x800A14`.
3. At `0x094115–0x094121` it reloads the selected creature, reads current XP a third time after
   admission, and copies that value to tracked XP. This last read supplies the tracker even
   when its value differs from the earlier readings. The equal-level XP branch has no level
   grant or level-up sound dispatch.

The two outputs each use a native 32-bit subtraction followed by signed integer stream
formatting. The comparison that admits the branch does not freeze either operand. On a stable
creature with a stable XP value both messages report the same gain. Agreement of the two
differences and the final publication across intervening changes is outside the guarantee.
For a signed difference beyond the 32-bit positive range the subtraction retains the low
32 bits for formatting. The first prefix always contains its literal plus sign.

For example, with level unchanged and tracked XP 100, successive observations at 80 and 95 send
no messages and leave the tracker at 100. At 110, with no intervening change during the admitted
branch, the floating record reads `XP +10`, the widget receives `You gain 10 XP.` followed by
a newline, then the tracker becomes 110. Several increases between observations can likewise
appear as a single gain over the last published tracker.

## Floating-text records

The update constructs both records with `0x0664B0`, fills the fields below, then appends the
record through `0x086510`. The list copies the record including its owned text, so the queued
record carries the sampled position and formatted string. Offsets below are within that record.

| Field | Equal-level XP gain | Level mismatch |
| --- | --- | --- |
| `+0x00` and `+0x04` | Integer age 0, binary32 value −100 | Same |
| `+0x08` | Three 64-bit position components copied from selected creature `+0x10` | Same |
| `+0x20` | Two binary32 values (0, −200) | (0, −250) |
| `+0x28` | Four binary32 color components (0.8, 0.6, 1, 1) | Same |
| `+0x38` | `XP +` and the first sampled XP difference | `LEVEL UP!` |
| `+0x50` | Integer duration 3000 | Same |
| `+0x54` | Binary32 presentation size value 30 | 50 |
| `+0x58` | Integer presentation selector 2 | Same |

The binary32 color components 0.8 and 0.6 have bit patterns `0x3F4CCCCD` and `0x3F19999A`.
The later text-list consumer divides the age by the duration when deriving progress, then
advances the age and marks the record for removal when the new age exceeds the duration.
These are native presentation values. Timing in real frames, pixel size and final rasterized
color remain outside this observation boundary. Original image `0x093D84–0x093E6F`,
`0x093EE7–0x094036`, `0x095A35–0x095A58`, `0x095B4F–0x095B79` and `0x095E17–0x095E40`.

## Relationship to a level change

Any observed level mismatch enters the level branch, including a decrease. On this branch
the update first appends `LEVEL UP!` at `0x093E6A`. At `0x093E8F` it calls the positioned
sound dispatcher with numeric selector `0x1D`, the freshly selected creature's position at
`+0x10`, and two binary32 arguments of 1.0. The original dispatch table at `0x085E90` maps
this selector to `0x084D1A`, which selects `levelup2.wav`. Dispatch does not establish that
the asset was available or that a sound was audible.

The update then reloads current level and publishes it to the level tracker at `0x093EA0`,
clears the progress tracker at `0x093EAE`, reads the selected creature's current level again,
and invokes the level grant once at `0x093EC4`. It jumps around the XP branch for this visit.
If the next visit sees equal levels and positive current XP, that XP can produce a gain from
the newly zeroed tracker. A current XP of zero at that next visit produces no gain. This
contract makes no promise about which later frame reaches the block or how ordinary game
actions modify the underlying XP fields.

## Original evidence and limits

The evidence is the supported client image, `decompiled/client/assembly.txt`, and the original
client functions individually identified by the RVAs above. The notification paths are in
routine `0x088EE0`, whose original function export timed out. The instruction listing and
image establish the local branches, stack arguments, list append, widget call and tracker
stores. The original narrow prefix at `0x301B4C`, wide strings at `0x301B38`, `0x301B54` and
`0x301B60`, and the wide signed-integer and newline imports at `0x2FC2E8` and `0x2FC32C`
establish the formatted inputs. Record ownership follows `0x086510`, `0x052100` and `0x066410`.
The three-byte widget color follows `0x00EA70` and `0x03A500`. This is static evidence for
normally returning calls. Full update scheduling, widget wrapping, display settings, failed
allocations and runtime or visual acceptance remain open.
