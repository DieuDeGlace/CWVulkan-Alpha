# Alpha ruling for sound 36 cadence in common actions

Status **FINAL / STABLE / HELD**

## Supported image

Original 32-bit Alpha client `binary/Cube.exe`, 3,885,568 bytes, preferred
image base `0x400000`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All code addresses are client RVAs.

## Ruling

The sound-36 period for actions **30–33, 87 and 88 is 200 ms**.
This confirms `alpha-rulings-channel2-explosion-window.md` §1 for the
client and supersedes the 50 ms divisor in `alpha-connected-action-audio.md`
§2. The supported image uses the same divisor for every admitted actor
at these two sites, including a connected nonlocal actor.

The original selectors at `0x222CCC` and `0x222C84` route all six action
bytes to `0x212357`. A nonzero world mode byte `+0xB4` admits the common
body directly. With that byte zero, admission requires either nonzero
actor control byte `+0x60` or actor equality with world pointer `+0xB8`.
After that gate, actions 30–33 take the branch at `0x212495` and actions
87/88 take the branch at `0x21279C`. These selectors choose two sites
with identical quotient arithmetic. Neither site selects a divisor by
action, ownership, world mode or specialization. Evidence is
`0x211E04–0x211E1B`, `0x212357–0x212378` and `0x2123BA–0x2123ED`.

In either branch, the **current signed action clock** at actor `+0x6C`
must be strictly below current anticipation from `0x03CAA0`. Divide
the current clock by 200. Separately add this step's signed integer
milliseconds to the current clock and divide that sum by 200. Both
divisions truncate toward zero. Unequal quotients produce one sound
36 record and one random
draw, even if the projected clock spans several boundaries. The
quotients use absolute action time. The anticipation check reads the
current clock, so an eligible step can cross a 200 ms boundary at or
after anticipation. The sound uses the actor position, unit volume,
and pitch formed by converting the draw to binary32, multiplying by
binary32 0.25, dividing by 32767 and adding 1, in that order. It
precedes the branch's release check and the shared swing check.
Evidence is `0x212495–0x212552`, `0x21279C–0x212859` and the random
calls at `0x212502` and `0x212809`.

The signed quotient sequences at `0x2124AF–0x2124D2` and
`0x2127B6–0x2127D9` use the reciprocal constant `0x51EB851F` with a
combined 38-bit product scale and a toward-zero sign correction.
The constant times 200 equals 2 raised to 38 plus 56. The reciprocal
error over the signed 32-bit input range has the sign of the input and
is smaller in magnitude than the spacing of 1/200 between successive
quotients. It shifts a negative exact multiple just below its integer
quotient, then the sign correction recovers that integer. Other
negative inputs use the same correction to truncate toward zero.
Integer overflow in the clock addition is outside this ruling.

| Current clock to projected clock | Signed quotients | Sound-36 draw when current clock is below anticipation |
| --- | --- | --- |
| 149 to 150 | 0 to 0 | None. |
| 199 to 200 | 0 to 1 | One. |
| 199 to 600 | 0 to 3 | One. |
| −200 to −199 | −1 to 0 | One. |

Evidence is static and scoped to the common client body and the six
named action selectors.
