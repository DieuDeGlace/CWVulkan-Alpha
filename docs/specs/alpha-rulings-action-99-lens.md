# Alpha ruling for held binding 99 and the lens meter

**FINAL / STABLE / HELD**

## Supported image and scope

The evidence is the original 32-bit Alpha client `binary/Cube.exe` in the
`disassembled alpha cube world` evidence root. The file is 3,885,568 bytes,
has preferred image base `0x400000` and SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses below are client RVAs except named object offsets. The original
listing `decompiled/client/assembly.txt` skips the held case body between
`0x09B6E2` and `0x09BD43`. The dispatch tables and body were read directly
from the image.
The controlled creature is the object at client `+0x8006D0`.

## Ruling and supersession

When a held slot containing binding 99 passes admission, its branch sets
mask `0x0400` in the controlled creature's unsigned 16-bit status word at
`+0x124`. This confirms the row for 99 in `alpha-client-action-input-phase.md` under
**Held dispatch and start helpers**. It supersedes the client-side claims in
`alpha-rulings-camera-lens-pan.md` §2.2 that no call sets this bit and that
the bit can reach a creature only through a whole-field copy or an
unidentified route. It also resolves that document's **Unresolved** entry
about the setter for this client path.

## Admission and writes

The action phase requires a zero cached UI result from the client virtual
entry `+4` and a zero client byte `+0x800704`. The walk visits binding slots
in ascending order starting at index 1. At a visited slot the binding is a
four-byte value in the vector at client `+0x800814` and the held condition is
a nonzero client byte at offset `4 + index`. The held configured slot sets
the common suppression flag before calling readiness helper `0x03E350`.
A false result rejects index 1. Later indices proceed to per-binding
admission regardless of that result. Evidence is `0x091F44–0x091F54` and
`0x09B58E–0x09B67A`.

For binding 99, per-binding helper `0x03E5A0` checks signed four-byte stun
`+0x12C` first and rejects a positive value. It then compares the cost with
binary32 resource `+0x170`. Binding 99 takes the zero-cost path through
`0x044AE0`, so an ordinary finite resource of zero passes and a negative
resource fails. Finally, cooldown key 99 in the map at creature `+0x139C`
must be absent or have a four-byte value exactly zero. Evidence is
`0x03E5A0–0x03E60A` and `0x044AE0–0x044B78`.

After the common item check at creature byte `+0x1020` and any resulting
transfer and replacement, the held dispatch at `0x09B6C1–0x09B6E2` selects
`0x09BC21` for full binding value 99 through image tables `0x09D57C` and
`0x09D52C`. The branch first sets `0x0400` through `0x02F160` with set
argument 1 on the record at creature `+0x10`.
The status word is record `+0x114`. The helper sets just the selected bit.
Next the branch finds or creates key 99 through `0x068910` in the same
creature's cooldown map. It queries `0x03E6A0` with 99 and signed `-1`,
stores the four-byte result, and resumes the slot walk at `0x09BF4D`.
An admitted pass assigns the cooldown even if the status bit was already
set. The branch's direct creature state changes are limited to the status
bit and cooldown. Evidence is `0x09B680–0x09B6BC`, `0x09BC21–0x09BC71`,
`0x02F160–0x02F185` and `0x068910–0x068991`.

Binding 99 bypasses the release changes, even if the current action byte
is 99. The end-of-walk cleanup driven by held-slot suppression clears
mask `0x0001`. Evidence is `0x09BDBA–0x09BDF9` and `0x09C016–0x09C02A`.

## Skill scalar and cooldown

Both the cooldown and the meter with the bit set use the binary32 skill
scalar from `0x03ED60` for arguments 99 and `-1`. Binding 99 selects slot 8
through `0x03C9C0`. The `-1` argument selects the signed four-byte allocation at
creature `+0x1158`. When that allocation is zero and creature byte `+0x60`
is nonzero, use the signed four-byte value at `+0x190` halved toward zero,
plus the unsigned byte at `+0x1A8`, plus one. Convert the selected integer
to binary32, multiply by binary32 0.1, add one, divide one by that sum and
subtract the quotient from one. Each floating-point operation rounds to
binary32. Evidence is `0x03C9C0–0x03C9F5`, tables `0x03CA08` and `0x03C9F8`,
`0x03ED60–0x03EDDD` and constant `0x2FCB7C`.

In cooldown helper `0x03E6A0`, binding 99 selects `0x03E828` through tables
`0x03E980` and `0x03E940`. Multiply the skill scalar by 10,000, then subtract
that product from 12,000 with separate binary32 rounding. Truncate the
result toward zero to a signed 32-bit integer. Evidence is
`0x03E6A0–0x03E6B9`, `0x03E828–0x03E852` and constants `0x2FD908` and
`0x2FD90C`.

## Meter, clear and frame order

On the creature world-step path at `0x2188A1–0x2188CA`, a strictly positive
squared length of the three-component binary32 intent at creature `+0x40`
clears `0x0400` through `0x02F160`. Helper `0x024860` rounds each square,
adds the first two squares and then adds the third, all in binary32. This
clear precedes the meter's status read on a creature path that reaches both.

The meter region tests the current action byte at `0x2195BA`. Action 79
takes its own meter route before the shared final checks. The other actions
test the current `0x0400` bit at `0x219C17`. The step duration for these
calculations is the signed duration of this individual call to `0x20C510`,
converted to binary32 at `0x20C83D–0x20C84A`.

- With the bit set, multiply the skill scalar by binary32 0.0005, then add
  binary32 0.00001. Multiply the sum by the step duration, then add the
  old binary32 meter at `+0x1190`. Store the result and cap it at one.
- With the bit clear, subtract the step duration multiplied by binary32
  0.0005 from the old meter. Store the result with a lower bound of zero.
- At the shared check, a resident type-3 entry found by `0x043F10` in the
  list at `+0x1178` forces the meter to one. Then multiply the step duration
  by binary32 0.00025, multiply that product by the resulting meter, add
  it to resource `+0x170`, and cap the resource at one.

The meter and resource operations round separately to binary32. The meter
and resource evidence is `0x2193FA–0x219509`, `0x219530–0x2195BE`,
`0x219BE9–0x219CEF` and `0x043F10–0x043F3E`. The rate constants are at
`0x31E1DC`, `0x302A20` and `0x31E1D8`.

When the respective paths execute in client update `0x088EE0`, the world
calls at `0x08CF2E` and `0x08CF49` finish before the crosshair reads and the
late held dispatch. These are the update's two direct world-step call sites.
Binding 99 therefore cannot change a meter pass already executed in that
update. A later admitted world step uses the then-current status after its
earlier intent clear on paths that reach both points.

The crosshair reads at `0x0927FF` and `0x0928CE` use this same bit. With an
existing node and a zero result from the corresponding fresh virtual `+4`
UI query, the normal crosshair at client `+0x8008AC` shows when the bit is
clear and client `+0x14` is zero. The zoom crosshair at `+0x8008B0` shows
when the bit is set. These visibility decisions precede the late setter
when both run in an update. The projection in render routine `0x0AC260`
reads the controlled creature's status itself and selects nominal vertical
field of view 30 degrees with the bit set or 50 degrees with the bit clear.
Evidence is `0x0927DA–0x09281B`, `0x0928AE–0x0928E4` and
`0x0AE4B3–0x0AE5D3`.

## Open boundaries

The established input is a held, admitted slot containing 99. Its player-facing
ability name, slot and loadout assignment, physical input in a given loadout,
and runtime reachability remain open here. Server writers, synchronization,
other client writers, the full action-79 branch and rendered presentation
timing remain open. The numeric relations cover valid objects and containers,
finite scalars, and ordinary representable integer times and conversions.
