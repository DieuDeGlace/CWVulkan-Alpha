# Alpha objective receipt state one and late actor interaction

**FINAL / STABLE / HELD**

This contract covers the argument-one interaction in the late objective receipt
walk of the original 32-bit Alpha client. The evidence image is `binary/Cube.exe`,
3,885,568 bytes, PE machine `0x014C`, timestamp `0x51EA955E`, preferred base
`0x00400000`, image size `0x003BC000`, SHA-256
`84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`.
All executable addresses below are RVAs. The findings are from static evidence.

## Receipt and actor resolution

The late walk in the frame routine at `0x088EE0` begins at `0x090651`. A zero
receipt dword at `+0x18` advances directly to the next record at `0x0907DC`. A
nonzero dword and receipt state byte `+0x25` equal to one select the actor path.
The actor key is the eight bytes at receipt `+0x08`. Lookup `0x02F000` in the
client world at `+0x2E4` returns the actor pointer or zero. The first lookup at
`0x0906B8` gates the interaction. A second lookup of the same key at `0x0906CD`
supplies the pointer passed with literal one at `0x0906D5` to `0x0882E0`.

A missing first result goes directly to the state-two comparison at `0x0906DA`.
A null second result returns immediately from the interaction and reaches the
same comparison. A state-one record advances at `0x0907DC` after either miss or
after a successful interaction. The late walk schedules no retry for a miss.
The adjacent state-two branch resolves a region cell and, if found, addresses
the UI by the keys `shine` and `objective` and dispatches sound ID `0x1E`
(`missioncomplete.wav`) at `0x0907D7`. Those calls require state two.

## Interaction for a present actor

The client dialogue pool is a pointer array bounded by client `+0x80092C` and
`+0x800930`. Both actor branches prefer the first entry already bound to the
same actor pointer in slot `+0x188`. In the absence of a matching entry they
use the last entry whose `+0x188` is zero, or return if the pool has none. For an
already bound entry, argument one selects preparation again and bypasses the
ordinary repeat-interaction progression at `0x08849B` and `0x088740`.

**Actor flag `+0x7E`, bit `0x80`, set.** This interaction marks existing map
entries. The actor's signed dwords `+0x1B0` and `+0x1B4` are divided by 64,
truncating toward zero. Both region coordinates must be in `0..1023`, and their
entry in the client table at `+0x3A0` must exist. The region has 64 consecutive
104-byte records beginning at `+0x14018`. Each record is eligible with a nonzero
dword at `+0x18` and a signed level at `+0x24` at most the controlled actor's
level plus two. The controlled actor is referenced from client `+0x8006D0`,
with its signed level dword at actor `+0x190`. The map pair comes from the
record's signed 64-bit X and Y at `+0x00` and `+0x08`. Each is divided by 65,536,
with truncation toward zero. The low 32 bits of each quotient are interpreted
as signed and divided by 256 with truncation toward zero. This branch does not
open or refresh a dialogue window.

**Actor flag `+0x7E`, bit `0x80`, clear.** The starting target zone pair comes
from actor `+0x1DC` and `+0x1E0`. The client can look up the corresponding cell
by dividing each coordinate by eight, truncating toward zero. If that lookup
provides a cell and its signed level falls outside the inclusive controlled
actor level range of minus two through plus two, the working zone pair becomes
`(-1, -1)`. A failed cell lookup preserves the pair. Actor subtype byte `+0x140`
equal to `0x84` requests localized `innkeeper` dialogue through `0x0E4A20` and
proceeds to the dialogue binding gate. Other subtypes use `0x0E4BD0`, driven by
actor mode byte `+0x1D8` and the working pair, and reseed the native C runtime
random stream from the actor dword at `+0x08`. After preparing dialogue for
those subtypes, the client marks the working pair on the map. Where the
destination map entry and its source region exist, the client copies the zone's
16-byte region descriptor into map entry `+0x10` through `+0x1F`. The descriptors
begin at region `+0x18` and vary by local X then local Y, with Y contiguous.

In both branches, `0x1FC160` marks a map zone through the client map object at
`+0x800D44`. Under the map lock at object `+0x8000C0`, it uses `0x202440` to look
up an existing entry with each coordinate in `0..65535`. It sets the lowest
bit of entry byte `+0x30` and increments the map dword at object `+0x8000BC`
only on a newly set bit. It does not allocate absent map backing storage.

## Dialogue binding and output limits

At `0x08887B` the client requires at least one prepared dialogue entry and a
nonzero dword at that first entry's `+0x04`. The success path sets the selected
widget state to one, binds the actor pointer in dialogue slot `+0x188`, copies
the 24-byte actor snapshot at `+0x1350` into dialogue slot `+0x190`, and replaces
the prepared dialogue list at slot `+0x160`. It zeros slot dwords `+0x16C`,
`+0x170`, `+0x174` and `+0x184`, and empties the old text list bounded by slot
`+0x178` and `+0x17C`. When dialogue fails the binding gate, the existing window
state is preserved. An earlier map update can still have occurred.

The identified writes are to client dialogue, map and localization/random
preparation state. The resolved actor is read and can become the dialogue's
bound actor. The traced argument-one path contains no verified actor field
write, audio dispatch or world particle emission. The evidence covers this
late interaction. Earlier receipt side effects, later presentation of a bound
dialogue, exact localized text selection and on-screen map appearance are
outside this contract.

## Original evidence

- `binary/Cube.exe` and `decompiled/client/assembly.txt`, frame walk
  `0x090651..0x09081B` and interaction `0x0882E0..0x0889D1`. The binary resolves
  the innkeeper cleanup and join at `0x088707..0x088727` and the text keys at
  `0x301DEC`, `0x3018F8` and `0x301864`.
- `decompiled/client/functions/0042f000.c` and `00477d90.c`, actor key lookup.
- `decompiled/client/functions/004e4a20.c`, `004e4350.c`, `004e4bd0.c` and
  `004681e0.c`, localized dialogue preparation and copying.
- `decompiled/client/functions/005fc160.c` and `00602440.c`, map entry marking
  and lookup. `00484350.c` identifies the neighboring state-two sound.
