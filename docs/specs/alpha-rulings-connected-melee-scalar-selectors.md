# Alpha ruling for connected melee scalar selectors

**FINAL / STABLE / HELD**

The supported original image is 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
with preferred image base `0x00400000`. Addresses below are client RVAs.
Action identifiers, control kinds and table ordinals are decimal.

This ruling covers the three disputed selectors for a living attacker in the
client common-action body, including connected nonlocal attackers. It resolves
the conflicting nominal-cost, amount-coefficient and impulse entries in the
connected melee contract. [Connected melee §§3–4](alpha-connected-melee-actions-2-through-7.md#3-resource-and-buff-preparation)

## Selector returns for live actions 1 through 7

| Action | Nominal mana cost at `0x044AE0` | Physical amount coefficient at `0x03EC90` | Impulse scalar at `0x044860` for kinds other than 3 and 5 | Impulse scalar for kinds 3 and 5 |
| --- | --- | --- | --- | --- |
| 1 | +0 | 1 | 6 divided by speed | +0 |
| 2 | +0 | 1 | 6 divided by speed | +0 |
| 3 | 0.1 | 1 | 3 divided by speed | +0 |
| 4 | 0.1 | 1 | 3 divided by speed | +0 |
| 5 | +0 | 0.5 | 4 | +0 |
| 6 | +0 | 1 | 3 divided by speed | +0 |
| 7 | +0 | 1 | 3 divided by speed | +0 |

The exact binary32 cost denoted by 0.1 is `0x3DCCCCCD`. Cost and amount
coefficient are independent of control kind for all seven actions. For
impulse, the control byte at attacker `+0x60` is tested for kind 5 and then
kind 3 at `0x044863–0x044870`. Either kind returns positive zero at
`0x0449FA` before the action lookup, including for action 5. In the divided
cases the selector calls `0x047700` once, stores its live speed result as
binary32, divides the numerator by that result in binary32 and returns the
rounded quotient. Control kind may affect the live speed through `0x047700`.
The kind-3 and kind-5 return and the constant-four return take no speed
sample within the impulse selector.

These returns agree with the ordinary zero-clock and common-hit contracts.
[Common-action processing](alpha-common-action-processing-state.md#ordinary-zero-clock-initialization),
[Common-hit input construction](alpha-common-hit-input-construction.md)

## Live action and connected-mode path

The world turn at `0x20C510` reads attacker byte `+0x68` and zero-extends it
at `0x211E04–0x211E07`. The outer dispatch subtracts one and accepts unsigned
indices through 109. Its seven bytes at `0x222CCC–0x222CD2` are all zero,
selecting the first target at `0x222C84`, namely `0x212357`. A nonzero world
mode byte at `+0xB4` enters preparation at `0x212378` via `0x212364`, bypassing
the mode-zero ownership comparison with the local pointer at world `+0xB8`.
This path therefore admits the connected nonlocal caller to the common body.

At ordinary live clock zero, `0x213520–0x213527` clears the forced-critical
byte `+0x13C0` and recorded resource `+0x13B4`. The call at `0x21353A` takes
the unsigned live action from `+0x68` and auxiliary selector minus one. The
strictly positive result for actions 3 and 4 causes a fresh action read and
a second cost call at `0x213562`. The caller records the second binary32
result at `+0x13B4` and subtracts it from mana at `+0x170` before shared
pending-resource processing. An ordinary nonzero live clock bypasses these
calls at `0x213510–0x213514`.

Action 5 enters the separate mana snapshot at `0x21367D` through the zero
cost continuation. It copies the live mana representation to `+0x13B4`,
clears mana to positive zero and proceeds to the type-11 buff path. Shared
finalization later overwrites the recorded amount when pending resource
`+0x144` is strictly positive and debits its full amount from mana. Action 5
halves the recorded pending amount after that debit. Finalization clears
the pending field and floors ordered negative mana to positive zero.
Evidence is `0x213553–0x21358B`, `0x2135A2–0x213646` and
`0x21364B–0x21374F`.

For the hit path, the connected-mode branch at `0x213B1E–0x213B25` joins
the common amount-baseline selection at `0x214084`. All seven actions
take baseline one. The admitted-candidate call at `0x2144E3` receives
the unsigned live attacker action read at `0x2144DE`. Its binary32
impulse return is saved, and the caller applies it after scaling by the
square root of the attacker-to-target size ratio. The size-ratio and
scaling operations also run for a zero impulse scalar. At `0x21452E` the
caller reads the action byte again. All seven enter the physical amount
branch and pass that byte zero-extended to `0x03EC90` at `0x214577`.
The binary32 coefficient multiplies the physical attack stat before the
product is multiplied by the separately formed resource factor, one plus
five times the squared recorded resource. This precedes the variation draw
at `0x2145C7`. Evidence spans `0x214084–0x2140AE` and `0x2144D6–0x2145C7`.

The connected nonlocal path uses the same action reads and selector branches
as other client callers that reach these sites. The selector inputs come
from the live attacker. The world-local comparison for target rejection at
`0x196D9F–0x196DAE` and the attacker comparison for success rewards at
`0x214906–0x21491B` occur after construction of the hit inputs.

## Original image table evidence

The dispatch bounds, in-scope byte ordinals, pointer targets and scalar
literals were read directly from the supported PE image. Original instructions
were checked against `decompiled/client/assembly.txt` at the corresponding
preferred virtual addresses. Each selector uses an unsigned bound on the
normalized action and indexes a byte table followed by a table of 32-bit
preferred virtual addresses. The targets stated here are converted to RVAs.

- **Nominal cost.** `0x044AE3–0x044AF9` subtracts 3 and accepts unsigned
  indices through 92, corresponding to actions 3–95. Actions 1 and 2
  reach the bounds fallback at `0x044B75`. For actions 3–7 the bytes at
  `0x044B90–0x044B94` are 0, 0, 1, 4, 4. In the pointer table at
  `0x044B7C`, ordinal 0 reaches `0x044B00`, which reads 0.1 from
  `0x2FCB7C`. Ordinals 1 and 4 both reach the positive-zero return at
  `0x044B75`.
- **Amount coefficient.** `0x03EC93–0x03ECA5` subtracts 5 and accepts
  unsigned indices through 67, corresponding to actions 5–72. Actions 1–4
  reach the bounds fallback and exact-one return at `0x03ECE8`. For
  actions 5–7 the bytes at `0x03ED10–0x03ED12` are 0, 7, 7. In the
  pointer table at `0x03ECF0`, ordinal 0 reaches `0x03ECC0`, which reads
  0.5 from `0x345DA0`. Ordinal 7 reaches `0x03ECE8`.
- **Impulse.** After the control-kind gate, `0x044876–0x04488A` subtracts
  1 and accepts unsigned indices through 87, corresponding to actions 1–88.
  The bounds fallback at `0x0449D9` divides 2 by speed. For actions 1–7
  the bytes at `0x044A38–0x044A3E` are 0, 0, 1, 1, 2, 3, 3. In the
  pointer table at `0x044A00`, ordinal 0 reaches `0x044908`, with numerator
  6 from `0x345E64`. Ordinals 1 and 3 reach `0x0448E7` and `0x044891`
  respectively and both divide numerator 3 from `0x2FCD70` by speed.
  Ordinal 2 reaches `0x04498D`, which reads constant 4 from `0x345E60`.

## Limits

This is static evidence for the identified client image and the specified
caller sites. Divided results depend on the live native speed, and the
numeric use above is bounded to finite state with positive divisors and
representable intermediate arithmetic. Ambient floating-point behavior
outside that domain, runtime acceptance and the server image remain open.
