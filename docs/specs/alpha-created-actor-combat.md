Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha created-actor first-child combat continuation

## Scope and evidence

This contract covers what happens in the update entry of the first child of the created-actor behaviour, after a target has been selected and until that entry returns. The earlier held contract `alpha-created-actor-behavior.md` was used only to find the routine and the point where the open continuation starts. Every fact below was checked again against the original images. Target selection itself, the item-use prefix and the early admission exits stay outside this contract, except where they hand state to it.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Client evidence: the original instruction listing `decompiled/client/assembly.txt` for routine `0x02CB20` (client table `0x2FD310`, entry `+0`), the corresponding original function files under `decompiled/client/functions/`, and image bytes. Server evidence comes from the same kinds of files under `decompiled/server/`, plus `Server.exe` bytes. Decompiler output was used only to find leads. Instruction order, widths, branch conditions, constants and jump tables were confirmed in the listings and in image bytes. Where the listing skips bytes after a deallocation call that it wrongly treats as non-returning, those bytes were decoded from the image. The skipped bytes are a stack adjustment followed by a jump to the common final store. No reconstruction material was consulted.

Terms used here:

- **Subject** (S): the actor being updated.
- **Target** (T): the selected actor.
- **Child**: the first-child object.
- **World** (W): the world argument.
- **Output**: the shared output argument.
- **Step**: the signed 32-bit integer step.
- **Position**: three signed 64-bit fixed-point coordinates at actor `+0x10`, `+0x18`, `+0x20`. One world unit is 65536 fixed-point units.
- **Separation conversion**: for each of the three components, T position minus S position at 64 bits, converted from signed 64-bit integer to binary32 (one rounding), then multiplied in binary32 by `1/65536` (`0x37800000`, client data `0x2FCD94`).
- **Action**: the subject byte at `+0x68`.
- **Clock**: the subject's signed 32-bit `+0x6C`.
- **Lockout**: the subject's signed 32-bit `+0x128`.
- **Flags word**: the subject's 16-bit word at `+0x124`.
- **Movement triplet**: binary32 `+0x40/+0x44/+0x48`.
- **Velocity triplet**: binary32 `+0x34/+0x38/+0x3C`.
- **Aim triplet**: binary32 `+0x160/+0x164/+0x168`.
- **Recovery threshold** (R0): the signed 32-bit result of subject query `0x03D1A0`.
- **Duration query** (B): the signed 32-bit result of subject query `0x03CAA0` with argument −1.

These names describe identities and storage only. They do not assert gameplay meaning.

All binary32 arithmetic below rounds after each operation. Comparisons are ordered binary32 comparisons unless stated otherwise. Behaviour with NaN or infinite operands, with 64-bit products or sums that overflow, and with invalid pointers or containers is not supplied.

## 1. Entry boundary and carried state

The continuation begins at `0x02D237`. It is entered with a non-null target in two ways: directly from the preliminary target route, at the branch `0x02CEFB`, or after the candidate scan. Only the scan route first performs the selected-target publication described in the earlier contract (the map `+0x13A4` value set to binary32 `1.0` and world helper `0x1A0970`). By the time either path arrives:

- child `+0x04` has already been reduced by the step in this invocation and clamped at zero (`0x02CE3A–0x02CE4F`);
- a local indicator records whether the subject's list headed at `+0x1178` contains an entry whose byte at node `+0x08` equals `8` (`0x02CE52–0x02CEEC`). This contract calls it the *kind-8-present indicator*.

There is no facing or angle test anywhere in the continuation. The only geometric decisions are squared-distance bands, one height comparison and the visibility query described below.

## 2. Busy indicator and child `+0x10`

The following steps run in order:

1. If the action is nonzero, three subject queries run, each with argument −1, in this order: A = `0x044270`, then B, then C = `0x047310`. A is halved by signed division that truncates toward zero. The sum is formed as C + ((A halved) + B) in 32-bit arithmetic. The indicator is *ready* when the clock is strictly greater than this sum. When the action is zero, the indicator is *ready* and no query runs.
2. A ready indicator sets bit `0x0040` in the flags word. Otherwise that bit is cleared. Other bits are unchanged.
3. Child `+0x10` has the step subtracted from it in 32-bit arithmetic. A negative result is stored as zero.

(Evidence: `0x02D237–0x02D29D`.) Because the target is non-null, the earlier no-target exit at `0x02D2A0` is never taken from here.

## 3. Separation, range selection, visibility query and candidate draw

The steps run in this order (`0x02D2AE–0x02D533`):

1. The separation D is converted as defined above, giving Dx, Dy and Dz.
2. A provisional reach is set to S `+0x80` × binary32 `2.0` (`0x40000000`, `0x345E10`) + T `+0x80`.
3. **Visibility query.** World routine `0x19EE90` is called with these arguments: S position, T position, flag `1`, maximum binary32 `200.0` (immediate `0x43480000`). Its low result byte is kept as the visibility indicator V. Section 9 describes the routine.
4. If subject predicate `0x044520` is true and V is nonzero, the reach becomes `50.0` (`0x42480000`, `0x2FCD7C`), with one exception. The exception applies when the action is `0x57`, `0x56` or `0x59` and the clock is strictly less than B. The provisional reach is then kept. B is queried only when the action is one of those three values.
5. If subject `+0x64`, read as a signed 32-bit value, is `0x2F`, `0x6F`, `0x71` or `0x70`, the reach becomes `20.0` (`0x41A00000`, `0x2FCD74`).
6. If the action is `0x5F`, `0x1C` or `0x6C`, the reach becomes `50.0`. If the action is `0x48`, the reach becomes S `+0x80` + T `+0x80`, with no factor. The action byte is read once for both tests.
7. **Candidate draw.** The candidate value starts at zero. If child `+0x04` is zero and the subject's four-byte element range `+0x13E8..+0x13EC` is non-empty, one direct random draw is made at `0x02D4AD`. The element count is the byte length shifted right arithmetically by 2. The raw 32-bit draw is reduced by unsigned remainder over that count. The selected element, a full 32-bit value, becomes the candidate. A candidate of `0x5D`, `0x5B`, `0x44` or `0x45` sets the reach to `20.0`. A candidate of `0x56` or `0x59` sets the reach to S `+0x80` × `2.0` + T `+0x80`. This draw is consumed whatever route follows.
8. If V is zero, the reach becomes `1.0` (`0x3F800000`, `0x345DC0`).
9. The reach is squared (call the result Q), and the squared separation is formed as (Dy·Dy + Dx·Dx) + Dz·Dz.

## 4. Near/far decision

The far route (Section 5) is taken when the squared separation is strictly greater than Q, or when V is zero. In every other case the near route (Section 6) is taken. Equality is near. (Evidence: `0x02D575–0x02D586`.)

## 5. Far route (`0x02EB24` onward)

### 5.1 Give-up reset

The step is added to child `+0x0C` in 32-bit arithmetic. If the result is strictly greater than `20000`, the following happens in order:

1. Every node of the subject's ordered map at `+0x13A4` is released, and the map is reset to empty with count zero.
2. Path-clearing helper `0x02EF10` runs on the subject.
3. Child `+0x10` is set to `3000` and child `+0x0C` to `0`.
4. The entry returns low byte **0**.

This route does not touch the action, the clock, the movement triplet or the aim triplet.

### 5.2 Pursuit

At or below `20000`:

1. If the action is `0x5F` or `0x1C`, it is set to zero. The clock is not changed.
2. If bit `0x02` of subject byte `+0x5C` is clear, the third steering component is zero. Otherwise it is Dz. The first two components are Dx and Dy from Section 3.
3. Let s = (third component)² + (Dy·Dy + Dx·Dx). If s > 0, the steering vector is normalized. The reciprocal is `1.0` divided by the binary32 rounding of the binary64 square root of s, computed with the original precise square-root import, and each component is multiplied by it. If s is not positive, the vector is left unnormalized.
4. The squared length of the steering vector is computed as (second² + first²) + third². A speed is chosen: `40.0` (`0x42200000`, `0x2FCE40`) when Q × `4.0` (`0x40800000`, `0x345E60`) is strictly greater than that squared length, and `80.0` (`0x42A00000`, `0x2FCE50`) otherwise.
5. The movement triplet is stored as the three components times the speed, in `+0x40`, `+0x44`, `+0x48` order.
6. **Direct-approach test.** This runs only when V is nonzero. World helper `0x209480` is called with the subject, the S position, the T position and the final reach as binary32. If its low result byte is nonzero and bit `0x20` of S `+0x5C` is clear, helper `0x02EF10` runs and the entry returns **1**. When V is zero the helper is not called.
7. **Path maintenance.** The binary32 value stored under T's 64-bit identifier (T `+0x08`) in map `+0x13A4` is multiplied by `0.9` (`0x3F666666`, `0x2FD31C`). Map access helper `0x068AD0` looks up the signed 64-bit key and creates a zero-valued entry when the key is absent.
8. If S `+0x1464` is zero and T `+0x5C` has any of bits `0x07` set, a new path request is built in this order:
   - the goal at S `+0x1440..+0x1457` is set to T's position;
   - the goal's third coordinate is reduced by an amount computed in binary32 as ((T `+0x88` × `0.5`) − `0.1`) × `65536.0` (`0x3F000000` at `0x345DA0`, `0x3DCCCCCD` at `0x2FCB7C`, `0x47800000` at `0x2FCE7C`), converted to signed 64-bit by truncating helper `0x28D946`;
   - the start at S `+0x1428..+0x143F` is set to S's position, and its third coordinate is reduced in the same way using S `+0x88`;
   - binary32 S `+0x80` + T `+0x80` is stored at S `+0x1458`;
   - world routine `0x1A9F90` runs with the subject.
9. If S `+0x1410` is zero, the entry returns **1**. If the start and goal are equal in all three 64-bit coordinates, the entry also returns **1**.
10. Otherwise a bounded loop runs for at most 10 iterations. Each iteration does the following in order:
    1. If S `+0x1464` is strictly greater than `50`, the entry returns **1**.
    2. If S `+0x1464` is nonzero, the goal is converted to cell coordinates by helper `0x02F040`. Each coordinate is signed-divided by 65536 with truncation; a negative 64-bit value then has one subtracted, even when it is an exact multiple; and the low 32 bits are kept. The three 32-bit integer coordinates of the node at the tail of the list headed by S `+0x1460` (node `+0x08`, `+0x0C`, `+0x10`) have the cell coordinates subtracted. Each difference is squared and the squares are summed, all in wrapping 32-bit arithmetic. The sum is converted to binary32. If (S `+0x80`)² is at least that value, the entry returns **1**.
    3. World routines `0x1AAAB0` and then `0x1A7C90` run with the subject.

    The entry returns **1** after the tenth iteration.

The far route never writes the aim triplet, the clock or the action, apart from the action reset in step 1. It also does not write S `+0x13E0`.

## 6. Near route

### 6.1 Common prefix

These steps run in order (`0x02D58C–0x02D5B7`):

1. Child `+0x0C` is set to `0`.
2. Helper `0x02EF10` runs.
3. S `+0x13E0` is set to zero (32-bit).
4. The movement triplet is set to three zeros.

### 6.2 Special-state branch

This branch is taken only when all of the following hold:

- the kind-8-present indicator is false;
- T's action is `0x52`;
- T byte `+0x1020` is `0x14`;
- T byte `+0x1021`, zero-extended, equals S `+0x64`;
- either S byte `+0x60` is not `5`, or S's 64-bit `+0x198` is zero;
- the squared separation is at most Q. This comparison is repeated here.

The branch then does the following (`0x02D614–0x02D734`):

1. S `+0x11D0/+0x11D4` receive T's 64-bit identifier.
2. If the clock is not strictly greater than R0, the entry returns **1**.
3. If the action is not `0x6E`, S `+0x70` (signed 32-bit) is cleared.
4. If S `+0x70` is less than `5`, the action becomes `0x6E`, the clock becomes `0`, and the entry returns **1**.
5. Otherwise a 24-byte record buffer is prepared by helper `0x02C520`. That helper zeroes the byte at `+0`, the dwords at `+4` and `+8`, and the dwords at `+0x10` and `+0x14`; it does not write `+0x0C` or bytes `+1..+3`.
6. The subject list at `+0x1178` is walked from first to last. Every entry whose kind byte (node `+0x08`) is `7` is copied into the buffer, 24 bytes from node `+0x08`, so the last such entry wins.
7. If any kind-7 entry was found, the buffer's dwords `+4` and `+8` are zeroed and routine `0x1957C0` is called with the subject, the buffer and the output.
8. The buffer is then rewritten in place: kind byte `8` at `+0`, dword `+4` = `0`, dword `+8` = `20000`, and `+0x10..+0x17` = T's identifier. `+0x0C` and bytes `+1..+3` keep whatever the buffer held before (stale contents, or the copied kind-7 values). Routine `0x1957C0` is called again.
9. The entry returns **1**.

The aim triplet is not written on this branch.

### 6.3 Main near route

These steps run in order (`0x02D739–0x02D87A`):

1. D is recomputed with the separation conversion. Dx and Dy are kept for the aim store, and the stored aim third value is set to Dz.
2. Bit `0x0004` of the flags word is set.
3. The squared separation is recomputed as (Dy·Dy + Dx·Dx) + Dz·Dz. Below it is called *d²*.
4. If the candidate from Section 3 is nonzero, the following happens and the dispatch in 6.4 is skipped: child `+0x04` is set to `20000`, the action becomes the candidate's low byte, the clock becomes `0`, and the common aim store in 6.5 follows. A drawn candidate whose value is zero is treated as no candidate.

### 6.4 Dispatch

Exactly one class runs, tested in the order below. Every class ends at the common aim store (6.5).

**(a) S `+0x64` = `0x74`.** If the clock is strictly greater than R0, a temporary list [`0x45`, `0x4A`, `0x49`] of four-byte elements is built. One direct draw (`0x02D928`) is reduced by unsigned remainder over 3, and the selected element's low byte becomes the action. The clock is set to `0`, and the list storage is freed.

**(b) S `+0x64` = `0x76`.**

1. If the clock is strictly greater than R0, a list [`7`, `6`, `0x14`, X] is built. X is `0x5F` when S's 16-bit word `+0x8C` equals `0x086A`, and `0x56` otherwise. One draw (`0x02DA43`) is reduced by unsigned remainder over 4; the result sets the action, and the clock becomes `0`.
2. Afterwards, whether or not step 1 ran, an action of `0x5F` causes the aim values to be replaced by T's position minus a point P, through the separation conversion. P comes from subject routine `0x046BB0`. P equals S's position when bit `0x04` of S byte `+0x7E` is clear. Otherwise it is an offset point whose derivation is not closed here.

**(c) S `+0x64` in {`0x2F`, `0x6F`, `0x71`, `0x70`}.** If the clock is strictly greater than R0:

1. A list [`0x6C`, `0x36`, `0x4C`] is built, and one draw (`0x02EA7C`) is reduced by unsigned remainder over 3 to set the action. The clock becomes `0`.
2. If the chosen action is `0x36`, the velocity triplet is set as follows. The 64-bit difference T − S is formed. Helper `0x02C6E0` multiplies each component by the 64-bit factor `0x20000` with wrapping, then signed-divides it by 65536, which doubles it. The result is converted like D. Then `+0x3C` is overwritten with `15.0` (`0x41700000`).

**(d) S `+0x64` = `0x77`.** If the clock is strictly greater than R0, a list is built in this order:

1. `0x45` when 16-bit `+0x8C` is `0x0863`, `0x0864` or `0x0865`; otherwise `0x44`.
2. `0x46` when 16-bit `+0x96` is `0x0867` or `0x0868`.
3. `0x4B` when 16-bit `+0x90` is `0x085F` or `0x0860`; otherwise `0x4A`.
4. `0x4D` then `0x4E` when 16-bit `+0x94` is `0x085C` or `0x085D`.
5. `0x4C` when `+0x94` is `0x0857` through `0x085B`.
6. If the action is neither `0x47` nor `0x48` and `+0x94` is `0x0855`, one draw (`0x02DCB6`) is made. A nonzero signed remainder by 2 appends `0x47`.
7. Under the same action condition, if `+0x94` is `0x0856`, one draw (`0x02DCFB`) is made. A nonzero signed remainder by 2 appends `0x48`. Steps 6 and 7 are mutually exclusive.
8. `0x49` is appended unless the action is `0x49`.

The list is never empty, so one final draw (`0x02DD47`) is reduced by unsigned remainder over the count to set the action, and the clock becomes `0`.

**(e) Subject predicate `0x044610` true.**

1. If d² is not strictly less than `3600.0` (`0x45610000`, `0x2FD334`), nothing else happens.
2. Otherwise, if binary32 S `+0x170` is strictly greater than `0.5`, one draw (`0x02DDA3`) is made. An odd signed remainder by 2 calls subject routine `0x0456A0`; an even remainder calls `0x045410`. When `+0x170` is at most `0.5`, `0x045410` is called with no draw. Their results are ignored.
3. Then, if the action is now `0x1E`, `0x1F`, `0x20` or `0x21`, B is queried:
   - When (clock − step), in 32-bit arithmetic, is strictly greater than B, the aim values are replaced by the 64-bit difference S `+0x1320` − S position, converted like D.
   - Otherwise the 24 bytes at T `+0x1350` are copied to S `+0x1320`.
4. Finally, if the action is `0x1C` and d² is strictly greater than `25.0` (`0x41C80000`, `0x2FCE3C`), the first two aim values are offset. An angle a is formed in binary32 as the clock converted from signed integer, times `0.005` (`0x3BA3D70A`, `0x2FCD9C`). The binary32 rounding of the binary64 cosine of a, times `1.5` (`0x3FC00000`, `0x2FCDE0`), is added to the first aim value. The sine is handled the same way and added to the second. The precise cosine and sine imports are used.

**(f) S `+0x64` = `0x65`.**

When d² < `64.0` (`0x42800000`, `0x2FD330`):

1. The aim triplet is written directly from T − S. This value is overwritten by the common store.
2. Bit `0x0004` is set again.
3. If the clock is strictly greater than `2000`, one draw (`0x02DF1C`) is made.
   - **Odd** (nonzero signed remainder by 2): the signed 64-bit height difference S `+0x20` − T `+0x20` is compared with the truncated 64-bit value of `524288.0` (`0x49000000`, `0x2FD340`). If the difference is at most that value, `+0x3C` is set to `30.0` (`0x41F00000`) and this class changes nothing else. The common store still follows.
   - **Even, or the difference is larger:** the clock becomes `0` and the action `0x33`. The velocity triplet is set from T − S, converted like D. If its squared length, (x² + y²) + z², is strictly greater than `0.0` (`0x345D84`), the triplet is normalized with the reciprocal of the rounded binary64 square root. The components are stored in order, then each is multiplied by `10.0` (`0x41200000`, `0x345E74`).

When `64.0` ≤ d² < `3600.0` and the clock is strictly greater than R0:

1. The clock becomes `0` and the action `0x25`.
2. An intermediate aim value is written as (T − S) plus an offset. The offset is (0, 0, T `+0x88` × `0.4`) (`0x3ECCCCCD`, `0x2FD318`), converted to fixed point by helper `0x02C460`, which multiplies each component by `65536.0` and truncates to signed 64-bit.
3. This intermediate value is also overwritten by the common store, so the elevation does not survive the invocation.

**(g) Bits `0x02` and `0x10` both set in the low byte of S's 16-bit `+0x7E`.**

1. Bit `0x0004` of the flags word is cleared first.
2. When d² < `64.0`, the same sequence as the first part of (f) runs, with three differences: the draw site is `0x02E175`, the height limit is the truncated value of `131072.0` (`0x48000000`, `0x2FD33C`), and the vertical value stored at `+0x3C` is `5.0` (immediate `0x40A00000`). Bit `0x0004` is set again on this sub-route.
3. When d² ≥ `64.0`, nothing else happens, and bit `0x0004` stays cleared.

**(h) S `+0x64` = `0x68`, or S byte `+0xAA9` in {`6`, `7`, `8`}.**

1. Nothing happens unless d² < `3600.0`.
2. **Evasion attempt.** It is tried only when the action is not `0x18`, `0x19` or `0x1B` and the lockout is zero. One draw (`0x02E770`) is made, and the signed remainder by 10 must be zero. In addition, all of the following must hold:
   - subject predicate `0x0445D0` is false;
   - S `+0x80` < `2.0`;
   - the clock is at least R0;
   - S `+0x5C` has bit `0x01` or `0x02` set.
3. If the evasion is admitted:
   1. V′ = T − S is converted like D. Only its first two components are used, and the third is replaced by zero.
   2. If (V′y² + V′x²) + 0 is not positive, nothing else happens.
   3. Otherwise u is the horizontal vector times the reciprocal of the rounded binary64 square root. Its components are u₁ and u₂, plus a third component u₃ equal to that reciprocal times binary32 `0.0`.
   4. A second draw (`0x02E867`) is reduced by signed remainder by 4 and selects one of four entries in the original jump table at `0x02EEF8`. The table entries are identical to those at `0x02EEE8`, and both hold the same four targets:
      - 0: the velocity triplet = u × `20.0`, stored third component first.
      - 1: u × `−20.0` (`0xC1A00000`, `0x2FCE9C`), stored first to third.
      - 2: the perpendicular (u₂ − u₃·0, u₃·0 − u₁, u₁·0 − u₂·0) × `20.0`, stored first to third.
      - 3: the same perpendicular × `−20.0`, stored third to first.
   5. A negative remainder would skip the velocity store.
   6. Then `5.0` (`0x40A00000`, `0x2FCE24`) is added to `+0x3C`, and the lockout is set to `600`.
4. **Fallback, when the evasion is not admitted.**
   1. If binary64(S `+0x170`) < `0.8` (binary64 `0x3FE999999999999A`, `0x2FD320`) and the action is not `0x18`, `0x19` or `0x1B`, routine `0x045410` is called and nothing else happens.
   2. Otherwise nothing happens unless the lockout is zero and the clock is strictly greater than R0.
   3. Let N be S byte `+0xAA8`:
      - When N is nonzero and `+0xAA9` is `7`, the action pair is (`0x18` → `0x16`).
      - When N is nonzero and `+0xAA9` is `8`, the pair is (`0x1B` → `0x1A`).
      - When N is nonzero and `+0xAA9` is `6`, or when S `+0x64` is `0x68` and neither of the first two cases applies, the pair is (`0x19` → `0x37`).
      - Otherwise nothing changes.
   4. For a pair (first → second): if the action already equals the first value and binary32 `+0x144` ≥ `+0x170`, the action becomes the second value and the clock `0`. If the action equals the first value but `+0x144` < `+0x170`, the action stays as it is and the clock is untouched. If the action is any other value, the clock becomes `0` and the action becomes the first value.

**(i) All other subjects.** Let W be the low byte of `+0x7E`.

1. If W has bit `0x10` set and the clock is strictly greater than R0, the action becomes `0x48` when S `+0x64` is `0x19` and availability predicate `0x03E5A0` with argument `0x48` is true. Otherwise it becomes `0x4B`. In both cases the clock becomes `0`. When the clock is not above R0, nothing happens.
2. If instead W has bit `0x08` set, routine `0x045410` is called.
3. Otherwise a melee reach is formed as (S `+0x80` + T `+0x80`) × `3.0` (`0x40400000`, `0x2FCD70`). Nothing happens unless d² is strictly less than the square of that reach.
4. Within reach, an evasion attempt runs exactly as in (h). Its blocking actions are `0x3B`, `0x3F` and `8` instead, its draw sites are `0x02E338` and `0x02E42F`, and it uses the jump table at `0x02EEE8`.
5. **Fallback, when the evasion is not admitted.**
   1. If binary64(`+0x170`) < `0.8` and the action is not `0x3B`, `0x3F` or `8`, `0x045410` is called.
   2. Otherwise nothing happens unless the lockout is zero and the clock is strictly greater than R0.
   3. If byte `+0xAA9` is `5`, the action becomes `5` and the clock `0`.
   4. Otherwise, when byte `+0xAA8` is nonzero and byte `+0x990` is zero:
      - if the action is `0x3B` and `+0x144` ≥ `+0x170`, the action becomes `0x3D` with clock `0` when byte `+0x141` is `1`, and `0x3C` with the clock set to B otherwise;
      - otherwise, if `0x03E5A0(0x36)` is true, the *charge result* applies: `+0x3C` is increased by `20.0`, the action becomes `0x36` and the clock `0`;
      - failing that, the clock is zeroed unless the action already is `0x3B`, and the action becomes `0x3B`.
   5. In every other case, byte `+0x991` decides:
      - **`0x0D`:** the pair rule from (h) with (`8` → `0x68`);
      - **`3`:** action `0x11`, clock `0`;
      - **`4`:** action `0x14`, clock `0`;
      - **otherwise, when `+0xAA8` is nonzero (so `+0x990` is nonzero):** if the action is `0x3F` and `+0x144` ≥ `+0x170`, the action becomes `0x0B` with the clock set to B. Otherwise the charge result applies if `0x03E5A0(0x36)` is true. Failing that, the clock is zeroed unless the action already is `0x3F`, and the action becomes `0x3F`;
      - **otherwise, when `+0xAA8` is zero:** the charge result applies if `0x03E5A0(0x36)` is true; otherwise routine `0x0456A0` is called.

### 6.5 Common aim store and result

Every near-route path except the special-state branch ends at `0x02EAEF–0x02EB17`. The aim triplet is stored as follows: `+0x160` = the first aim value, `+0x164` = the second (including any offset from (e)), and `+0x168` = the third. In ordinary cases the triplet is the converted T − S. The replacements in (b) and (e) apply where those routes took them. The entry then returns **1**. The continuation itself writes no yaw or other orientation field. No store to subject `+0x30` occurs in the routine.

## 7. Random draws

The continuation contains 14 direct `rand` call sites (client import `0x2FC468`):

- `0x02D4AD`: candidate, on the shared path when admitted (1);
- `0x02D928`: (a) (1);
- `0x02DA43`: (b) (1);
- `0x02EA7C`: (c) (1);
- `0x02DCB6`, `0x02DCFB`, `0x02DD47`: (d) (3);
- `0x02DDA3`: (e) (1);
- `0x02DF1C`: (f) (1);
- `0x02E175`: (g) (1);
- `0x02E770`, `0x02E867`: (h) (2);
- `0x02E338`, `0x02E42F`: (i) (2).

Total: 1 + 1 + 1 + 1 + 3 + 1 + 1 + 1 + 2 + 2 = 14.

For one invocation, draws happen in this order: the candidate draw first, if admitted, and then at most the draws of the one dispatch class taken:

- a nonzero candidate: no further draw;
- (a), (b), (c), (f), (g): at most one each;
- (d): one or two;
- (e): zero or one;
- (h), (i): zero, one or two.

The far route and the special-state branch make no direct draw beyond the candidate draw. So at most three direct draws occur per invocation of the continuation.

The reductions are:

- unsigned remainder over a list count for the candidate and for every list selection;
- signed remainder by 2 for parity tests, with a nonzero remainder meaning odd;
- signed remainder by 10 for the one-in-ten gate;
- signed remainder by 4 for the evasion direction.

The routine contains no direct `srand` call. Section 9 states which nested routines are free of direct or static-transitive draws and which remain unresolved.

## 8. Ending and interaction with the companion and wander children

The continuation returns low byte **1** on every path except the give-up reset in 5.1, which returns **0**. The root stops at the first nonzero child result, so after a continuation that returns 1, neither the second child (companion vector) nor the third child (wander) runs in that update. Their counters and their bit `0x0040` handling are left as they were.

Only after the give-up reset do the second and third children run in the same update. They then overwrite bit `0x0040` and the movement triplet as their own contracts describe. The reset itself leaves the movement triplet as an earlier invocation stored it, and leaves the aim triplet untouched.

The continuation also shapes the next invocation:

- A lockout set to `600` by an evasion makes the first child's early lockout exit return 1 until another producer clears it. That producer is not closed here.
- Child `+0x04` set to `20000` stops candidate draws until the earlier reduction brings it back to zero.
- Child `+0x0C` accumulates only on the far route and is cleared on the near route.
- Child `+0x10` set to `3000` by the reset gates the earlier no-target scan, as the earlier contract describes.

## 9. Nested routines

Rand reachability was checked statically by following every direct call in the listing. Indirect calls cannot be followed this way. "No draws" below means no direct `rand` or `srand` call in the routine and none in any statically reachable direct callee.

- **Audited from the listing** (inputs and outputs as used above; no direct draws; limits noted per item):
  - `0x02C7A0`: copies a 64-bit triple and subtracts another component-wise.
  - `0x02C800`: the same with addition.
  - `0x02C6E0` with `0x02C900`: copies, then multiplies each component by a 64-bit factor using runtime helper `0x28DA90` and signed-divides it by 65536 using `0x28D9E0`.
  - `0x02C4A0`: signed 64-bit to binary32, then × `1/65536`.
  - `0x02C460`: each binary32 × `65536.0`, then truncation to signed 64-bit.
  - `0x02C5B0`: a 24-byte copy.
  - `0x02C520`: partial zeroing, as stated in 6.2.
  - `0x02F040`: cell conversion, as stated in 5.2.
  - `0x044520`: true when byte `+0x140` = `3`; or `+0x64` is `0x75`, `0x56` or `0x68`; or byte `+0xAA8` = `3` with `+0xAA9` in {`6`, `7`, `8`, `0x0A`, `0x0B`, `0x0C`}.
  - `0x044610`: true when `+0x140` = `3`; or `+0x64` is `0x75` or `0x56`; or `+0xAA8` = `3` with `+0xAA9` in {`0x0A`, `0x0B`, `0x0C`}.
  - `0x0445D0`: true when `+0x64` is one of `0x6C`, `0x6D`, `0x72`, `0x74`, `0x73`, `0x76`, `0x6B`, `0x75`, `0x65`, `0x77`.
  - `0x03E5A0(code)`: false when `+0x12C` > 0 and the code is not `0x65`. Otherwise it is false when cost query `0x044AE0(code, −1)` is strictly greater than `+0x170`, and false when a lookup in the subject map at `+0x139C` finds an entry whose `+0x14` is nonzero. Otherwise it is true. For code `0x1C` the result is instead `+0x170` > `0.0`. The internals of the cost query and of map lookup `0x03F720` are not audited; neither makes draws.
  - `0x1957C0(subject, record, output)`: applies the record to the subject through `0x046AF0`. That helper replaces the first existing `+0x1178` entry of the same kind when the kind is `1`, `2`, `3`, `6`, `7`, `9`, `0x0A` or `0x0B`, and otherwise appends. `0x1957C0` then appends to the output's list at `+0x58` a node holding the subject identifier twice and the 24-byte record, and increments the output's `+0x5C`. Its node-insertion helpers and its container-length error import were not followed; see the unresolved item below.
- **Visibility query `0x19EE90`.** This is world-relative. It returns 1 when the converted squared separation is below `0.0001` (`0x38D1B717`, `0x345D90`). It returns 0 when that squared separation exceeds the square of the maximum. Otherwise it calls world routine `0x1A35D0` with the normalized direction and the distance, and returns 1 exactly when the binary32 value returned is at least the distance minus `0.01` (`0x3C23D70A`, `0x2FCDA0`). The internals of `0x1A35D0` are not audited. It makes no draws.
- **Bounded, not fully audited, no draws:**
  - durations `0x044270`, `0x03CAA0`, `0x047310` and threshold `0x03D1A0` (signed 32-bit integer results);
  - action helpers `0x045410` and `0x0456A0`, which can change the action and clock and choose follow-up actions through further subject helpers;
  - point helper `0x046BB0`;
  - direct-approach helper `0x209480`;
  - map access `0x068AD0`;
  - path-clearing helper `0x02EF10`, described in the earlier contract;
  - truncation helper `0x28D946`;
  - temporary-list append `0x26ADD0` and release `0x1FB860`.
  `0x26ADD0` reaches an indirect call that was not followed; it is treated as container growth only.
- **Unresolved for transitive draws, because indirect calls were reached:** path routines `0x1A9F90`, `0x1AAAB0` and `0x1A7C90`, and the indirect calls reached from `0x1957C0`.

## 10. Server agreement

The server's first-child table was established independently. It is at RVA `0x1587E0`, with update entry `0x002F40` at `+0`. Constructor `0x0029E0` installs that table and stores `20000`, the supplied binary32, and two zeros at child `+0x04..+0x10`.

Server routine `0x002F40` was compared instruction by instruction with client `0x02CB20` at a constant RVA offset (server = client − `0x29BE0`) over the whole body, 2106 listed instructions. Instruction text matched everywhere once absolute data, import and callee addresses were set aside. Every differing operand was then checked:

- Each server constant has the same bit pattern as its client counterpart. Examples (client → server): `0x2FD334` → `0x158830`, `0x2FCE9C` → `0x15879C`, `0x2FD320` → `0x158818`, `0x2FD340` → `0x15883C`, `0x2FD33C` → `0x158838`. The same holds for all other pairs used in Sections 3–6.
- The two evasion jump tables at `0x005308` and `0x005318` hold the server counterparts of the same four targets.
- The bytes the listing skips after the server's deallocation calls match the client's pattern.
- The server `rand` import is `0x1582F4`, with 14 continuation sites at the same relative positions (`0x0038CD` … `0x004E9C`).

Nested callee pairs were compared as sequences while ignoring absolute addresses. All callees named in Section 9 were compared except `0x02C900`, `0x046AF0`, `0x1A35D0`, the list-container helpers and the runtime imports. Identical sequences were found, including the visibility query (`0x0D4D80`, whose tolerance constants match), the direct-approach helper and the path routines. There are two exceptions, the path-clearing helper and the last path routine. In both, the server listing decodes instructions after a deallocation call where the client listing stops, so they were not compared further. Static rand reachability gives the same outcome in the server. On this evidence, client and server agree on the whole continuation, including decision order, constants and draw order. That conclusion is limited by the unaudited helper internals listed in Section 9.

## 11. Unresolved

The following remain open:

- the internal decision tables of the duration, threshold and action helpers (`0x044270`, `0x03CAA0`, `0x047310`, `0x03D1A0`, `0x045410`, `0x0456A0`) and of the cost and availability inputs;
- the internals of the visibility ray routine, the direct-approach helper and the three path routines, including possible transitive draws behind indirect calls;
- the full derivation of the offset point from `0x046BB0`;
- the producers and consumers of the lockout, S `+0x70`, S `+0x1320`, T `+0x1350`, the `+0x1178` record kinds, the output records, the aim triplet and the velocity and movement triplets;
- the gameplay meaning of every action code and type value named here;
- behaviour with exceptional arithmetic, overflow and invalid containers.
