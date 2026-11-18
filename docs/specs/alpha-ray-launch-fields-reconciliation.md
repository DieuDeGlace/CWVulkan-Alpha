# Alpha ray launch field reconciliation

**FINAL / STABLE / HELD**

This supplemental contract rechecks the ray-case launch fields and action 94
admission against the original x86 images. All addresses are RVAs.

| Image | SHA-256 |
| --- | --- |
| C, `Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S, `Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

At both launch sites in both images, projectile `+0x54` contains the
32-bit bits `0x00000000`, representing binary32 positive zero.
Projectile `+0x58` contains the 32-bit integer zero. The constructor first
sets `+0x54` to binary32 1.0, then each launch site explicitly overwrites
it with zero before appending the record. The constructor writes are at
S `0x0228d5` and C `0x0596a5`.

| Launch | Initialization and appends | Zero write to `+0x54` | Zero write to `+0x58` |
| --- | --- | --- | --- |
| S endpoint record | `0x13be50` through `0x13bf99` | `0x13be6c` | `0x13be76` |
| S actor-hit record | `0x13c5e1` through `0x13c6d3` | `0x13c5fd` | `0x13c607` |
| C endpoint record | `0x216090` through `0x2161d9` | `0x2160ac` | `0x2160b6` |
| C actor-hit record | `0x216821` through `0x216913` | `0x21683d` | `0x216847` |

The neighboring binary32 field `+0x50` is 1.0 (`0x3f800000`) and
`+0x4c` is 4.0 (`0x40800000`). These records also have zero velocity,
flag byte `+0x5c` zero, 32-bit kind `+0x60` equal to 3, effect byte
`+0x64` equal to 2, and zero 32-bit fields at `+0x68` and `+0x6c`.
Both the resident-list append at world `+0x14` and collector append at
`+0x20` copy the zeroed fields by value. The payload copies read and
write `+0x54` and `+0x58` at S `0x02285c` through `0x022865`
and C `0x05962c` through `0x059635`.

This contract supersedes the statement in
`alpha-dedicated-action-projectile-launch.md` that both `+0x50` and
`+0x54` are 1.0 for these launches. Its `+0x50` value remains
confirmed. The zero values for `+0x54` and `+0x58` stated in
`alpha-combat-beam-leftovers.md` section 5.1 agree with these original
launch-site writes.

Action 94 is action byte `0x5e` at actor `+0x68`. It enters this
family at S `0x13ba28` and C `0x215c68`. After the family's
resource and window checks, its action-hit stage opens when the stored
signed 32-bit age at actor `+0x6c` is at most the native threshold and
the 32-bit age-plus-piece sum is strictly greater than that threshold.
The comparisons are signed. Each comparison reevaluates the native threshold
helper with argument -1, S `0x007db0` or C `0x03caa0`
(S `0x13bae8` through `0x13bb16`, C `0x215d28` through
`0x215d56`).

When stored age already exceeds the threshold, the later periodic path
admits action bytes `0x5f` and `0x1c`. Action 94 exits that path at
S `0x13bc9a` or C `0x215eda`. Thus its hit stage is confined to
the threshold-crossing piece. Within that piece, positive reach can still
visit multiple ray positions and multiple admitted actors.

With mode byte at world `+0xb4` equal to zero and local pointer at
world `+0xb8` null, a nonnull source passes this family gate when its
single role byte at actor `+0x60` is nonzero. A role-zero source fails
the pointer comparison and exits the action work
(S `0x13ba28` through `0x13ba43`, C `0x215c68` through
`0x215c83`). The class-3 specialization-1 tests remain additional
requirements for the projectile launch sites.
