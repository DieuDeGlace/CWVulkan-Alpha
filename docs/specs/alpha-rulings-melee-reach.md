Status: FINAL / STABLE / HELD

# Alpha ruling: melee creature beyond its reach L

Scope: the melee branch of the creature combat behaviour step (in-range branch, step 11),
for a creature that passed the weapon gate and has neither appearance flag 0x10 nor 0x08.
Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, image base 0x400000. RVAs below are image-relative.

## Ruling

When d² ≥ L², the melee branch goes straight to the common exit. It does **not** run the
mana test, the chain starter, the dodge or the attack choice, and it makes **no** random draw.

L = (own box size obj +0x80 + target box size +0x80) × 3.0, squared in binary32. The 3.0 is
at client RVA 0x2FCD70 and server RVA 0x158718 (both 0x40400000). d² is the binary32 squared
separation computed earlier in the routine and held in a stack slot (client store RVA
0x2D85B, server 0x3C7B). Nothing else writes that slot before the reach test.

## Tests in order, up to the exit (beyond L)

| # | Test | Client RVA | Server RVA | Result when taken |
|---|---|---|---|---|
| 1 | Type is 104 | 0x2E25E | 0x467E | ranged branch (not this path) |
| 2 | Main-hand subtype obj +0xAA9 is 6, 7 or 8 | 0x2E267 … 0x2E282 | 0x4687 … 0x46A2 | ranged branch (not this path) |
| 3 | Appearance flag 0x10 | 0x2E288 | 0x46A8 | flag-0x10 rule (not this path) |
| 4 | Appearance flag 0x08 | 0x2E2CF | 0x46EF | chain starter (not this path) |
| 5 | Form L², compare with d² | 0x2E2DF … 0x2E2FF | 0x46FF … 0x471F | — |
| 6 | Branch if L² ≤ d² (also taken if unordered) | 0x2E304 | 0x4724 | **common exit** |

The common exit is client RVA 0x2EAEF, server RVA 0x4F0F: it stores the aim triplet
(obj +0x160, +0x164, +0x168) as described in alpha-created-actor-combat.md §6.5 and returns
handled. The current-action duration query (client 0x3D1A0) is not called on this path.

So, beyond L, per update: zero draws, no write to action, time, velocity, dodge time or
charge; only the common aim store. Equality d² = L² also exits (the test needs d² < L²
strictly). A NaN comparison exits as well.

The mana test (mana < 0.8, client 0x2E546, server 0x4966) and its chain-starter call (client
0x2E56C jumping to 0x2E2D3, server 0x498C to 0x46F3) are reachable only from inside reach:
from the blocking-action test (actions 0x3B, 0x3F, 8), the nonzero dodge time, the
one-in-ten draw failing (draw site client 0x2E338, server 0x4758), or a failed dodge
condition.

## Which contract statement is right

* **Right:** alpha-created-actor-combat.md §6.4 (i) step 3, "Nothing happens unless d² is
  strictly less than the square of that reach." (Read "nothing" as: only the common aim
  store and handled return of its §6.5.)
* **Wrong:** alpha-open-points-4.md §6.1 step 1, "If *d*² ≥ L²: go to step 5." It must read:
  common exit, no draw, no mana test, no chain starter. Its §6.3 example "no draw at all;
  straight to the mana test" is wrong in the same way (the no-draw part is right). Its
  summary "0 outside L" draws is right.

## Evidence

Static disassembly (dumpbin) of both original images, routine bodies read instruction by
instruction from the weapon gate through the dodge, mana test and attack choice; constants
read from the image bytes. Client and server bodies are byte-for-byte parallel in this region
(same branch displacements). Reconstruction sources were not consulted for this ruling.

## Unresolved

* Not verified at runtime; this is a static reading of both images.
* The meaning and producer of the stack value used as d² were taken from the store at client
  0x2D85B / server 0x3C7B; the components it squares were not re-derived here (see the held
  contracts for the separation vector).
* The common exit's handled return is taken from alpha-created-actor-combat.md §6.5; only the
  aim stores at client 0x2EAEF … 0x2EB17 were re-read for this ruling.
