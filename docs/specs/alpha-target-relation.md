Status: FINAL / STABLE / HELD. Full author readback completed.

# Creature target relation (the sibling of the hostility test)

This independently authored, code-free contract specifies the two-creature relation at client RVA `0x196CA0` / server RVA `0x0CFCC0`, its callers, and how the result is used by the callers that choose an attack target or spread threat. The held contract `alpha-hostility-test.md` was used only to locate this routine and for terms; every statement below was re-established from the original images. The Infinite-Cubes reconstruction was not consulted.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`; every address below is an RVA. Pairs are written client / server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

- Instructions were read from `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. The per-function listing files for the relation are absent from `assembly-functions` in both images. Decompiler output was not relied on.
- Data constants were read from the image bytes through each image's section table.
- Callers were found by scanning every section of each image for relative calls and jumps (opcodes `E8`, `E9` and `0F 80`–`0F 8F`) whose destination is the relation, and for any 32-bit occurrence of its absolute address (base plus RVA). No absolute occurrence exists in either image, so the relation is not reached through a pointer table. The listing and `decompiled/client/calls.csv` agree with the scan. No short jump reaches the relation from outside it.
- Client and server routines were compared instruction by instruction after masking absolute addresses (section 8).

Terms (as in the sibling contract): a **creature** is the object held by the world's creature container; its **key** is the signed 64-bit value at `+0x08`; its **position** is the three signed 64-bit fixed-point coordinates at `+0x10` (65536 units per world unit); its **health** is the binary32 at `+0x16C`. The **threat table** of a creature is the ordered table whose header is at `+0x13A4`, mapping a signed 64-bit creature key to a binary32 value (the name is descriptive only; section 7 states what is observed). Call the relation **R(A, B)**, with A its first and B its second argument.

## 2. Interface

- Two 32-bit stack arguments, both creature pointers: first A, then B. The routine removes its own 8 bytes of arguments on return.
- The result is the low byte of the accumulator: 1 or 0. The upper 24 bits are not defined (they hold whatever the last role load or nested call left). All four callers in each image test only the low byte.
- Callers may leave the world in the counter register; the relation never reads it before overwriting it.
- The routine preserves the callee-saved registers. It reads no world or global state, performs no lookup, no floating-point work and no memory write other than its own stack. It makes no random draws, directly or nested.

## 3. Fields read

On each creature, at most:

| Offset | Width | Use |
| --- | --- | --- |
| `+0x60` | 8 bits | **Role byte.** Compared for equality with 5 (on A only), 0 (on B only), 6 and 1 (on both). Signedness is irrelevant. |
| `+0x124` | 8 bits (the low byte of the word there) | **Toggle bit 5**, tested with mask `0x20`. |
| `+0x7E` | 16 bits, zero-extended | Read only by the nested predicate E (section 4): mask `0x1A00`. |
| `+0x64` | 32 bits | Read only by E: equality with a fixed list of 32 values. |

Health is not read. Keys, positions, threat tables, owner or creator links and any game-mode or local-player state are not read.

## 4. Nested predicate E (`0x044680` / `0x00F710`), fully audited

E takes one creature in the counter register, no stack arguments, and returns 1 or 0 in the low byte. It makes no calls, no draws and no writes, and modifies only the accumulator and flags.

1. When the 16-bit value at `+0x7E` has any of bits 9, 11 or 12 set (mask `0x1A00`), E is 0.
2. Otherwise E is 1 exactly when the 32-bit value at `+0x64` equals one of these 32 values, and 0 for every other value (including negative values):
   `0x13`, `0x14`, `0x16`, `0x17`, `0x19`, `0x1A`, `0x1B`, `0x1E`, `0x1F`, `0x20`, `0x21`, `0x22`, `0x23`, `0x35`, `0x37`, `0x38`, `0x39`, `0x3A`, `0x43`, `0x44`, `0x4A`, `0x57`, `0x59`, `0x5C`, `0x5D`, `0x62`, `0x64`, `0x6A`, `0x6B`, `0x91`, `0x92`, `0x93`
   (decimal 19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146, 147).

The image tests the values in another order; the order has no effect because the test is pure. The predicate is byte-identical in the two images.

## 5. Decision

Let r(x) be the role byte and t(x) toggle bit 5 of `+0x124`. The rules are evaluated in this order and the first that applies decides:

1. **Role 5 against role 0.** When r(A) = 5 and r(B) = 0, the result is 0. (At this step B's role is read only when r(A) = 5.)
2. **Role 6.** When r(A) = 6, the result is 0. Otherwise, when r(B) = 6, the result is 0.
3. **A has role 1.** When r(A) = 1 and r(B) ≠ 1, the result is 1.
4. **Both role 1, differing exemption.** When r(A) = r(B) = 1 and E(A) ≠ E(B), the result is 1. E(A) is evaluated before E(B).
5. **B has role 1.** When r(B) = 1 and r(A) ≠ 1, the result is 1.
6. **Toggle bit.** When t(A) is set, or else t(B) is set, the result is 1.
7. Otherwise the result is 0.

When both roles are 1 and the exemptions are equal, the routine re-reads both role bytes and evaluates E(A) then E(B) a second time before reaching rule 6 (four evaluations in total); the repetition has no effect because E is pure. When exactly one creature has role 1, E is never evaluated.

### 5.1 Outcome table

Write T for "t(A) or t(B)", X for any role value other than 0, 1, 3, 5 and 6 (that is 2, 4 and 7–255), and D for "1 when E(A) ≠ E(B), otherwise T". Rows are r(A), columns r(B):

| r(A) \ r(B) | 0 | 1 | 3 | 5 | 6 | X |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | T | 1 | T | T | 0 | T |
| 1 | 1 | D | 1 | 1 | 0 | 1 |
| 3 | T | 1 | T | T | 0 | T |
| 5 | **0** | 1 | T | T | 0 | T |
| 6 | 0 | 0 | 0 | 0 | 0 | 0 |
| X | T | 1 | T | T | 0 | T |

Consequences read directly from the decision:

- Role 3 and every X behave identically in both positions. Role 0 behaves like them except as the second argument when the first has role 5, and role 5 behaves like them except as the first argument when the second has role 0.
- The relation is symmetric except for that one pair: R(A, B) is 0 when r(A) = 5 and r(B) = 0 regardless of toggles, while R(B, A) is T.
- Any role-6 creature in either position gives 0, regardless of toggles.
- A role-1 creature and a creature of any role other than 1 and 6 give 1 in both orders.
- Dead creatures are not distinguished: health is not read.

### 5.2 Same creature

There is no pointer comparison. R(x, x) is 0 when r(x) = 6, and otherwise equals t(x) (for role 1, E agrees with itself, so rule 6 decides; role 5 against role 0 cannot occur with one creature). Callers that must exclude a creature from its own consideration do so themselves (section 7).

### 5.3 Contrast with the hostility test (`0x1988A0` / `0x0D18C0`)

Re-read in the client image for this comparison only. The hostility test differs in exactly three ways: it returns 0 for the same pointer before reading any field; it returns 1, not 0, when either role is 6; and it has no role-5-against-role-0 rule. Its role-1, exemption and toggle rules give the same results as rules 3 to 7 above.

## 6. Callers

Confirmed by the byte scan in each image. Client total 4, server total 4. Each pair is the same site in corresponding routines.

| Client site (routine) | Server site (routine) | First argument | Second argument | Generic role |
| --- | --- | --- | --- | --- |
| `0x02D10C` (`0x02CB20`) | `0x00352C` (`0x002F40`) | the creature choosing | each candidate | Creature attack-target choice (7.1). |
| `0x196099` (`0x195A60`) | `0x0CF0B9` (`0x0CEA80`) | the source of a heal | each world creature | Threat from healing, inside hit application (7.2). |
| `0x1A0A40` (`0x1A0970`) | `0x0D6010` (`0x0D5F40`) | each world creature (candidate ally) | the creature threat is directed at | Threat propagation to nearby allies (7.3). |
| `0x221D1C` (`0x20C510`) | `0x147ADC` (`0x1322D0`) | the owner of a projectile or object | the creature it touched | World update, healing-object contact (7.4). |

The world-update pair differs by `0xDA240`, the other three by the offset between corresponding routines. There is no client-only or server-only caller.

## 7. How the callers use the result (bounded)

Only the parts that depend on the relation are described; the rest of each caller is not specified here. All comparisons named "ordered" are false when an operand is NaN.

### 7.1 Attack-target choice (`0x02CB20` / `0x002F40`)

A per-creature behaviour routine (the counter register holds a controller object; the creature, called the chooser, is its first stack argument) performs a nearest-candidate search over the world's creature container when earlier parts of the routine (not specified here) have not already produced a target, and skips the search when the chooser has role 5 with a nonzero 64-bit creator key at `+0x198`, or when the controller's signed 32-bit `+0x10` is positive. The container is walked in ascending key order. For each entry:

1. Skipped when the entry is null, is the chooser itself (pointer comparison), has health that is not greater than 0 (a NaN health is not skipped), or has role 5.
2. A squared distance q from the chooser's position to the candidate's is formed in binary32 (each 64-bit difference converted to binary32 and multiplied by 2^−16, then y² + x², then + z²). When the candidate's binary32 `+0x1190` is greater than 0, q is divided by a candidate-specific factor derived from that value (not specified here).
3. When a routine-local flag (set earlier, not specified) is set, or when q is not less than the best value so far (ordered; initially the square of the controller's binary32 `+0x08`), the candidate is passed over without calling the relation.
4. Otherwise R(chooser, candidate) is evaluated.
   - **True:** go to step 5.
   - **False:** the candidate is passed over, unless all of the following hold, in which case go to step 5: the chooser has role 5 and a zero creator key at `+0x198`; the candidate's byte `+0x68` is `0x52`; its byte `+0x1020` is `0x14`; and its byte `+0x1021`, zero-extended, equals the chooser's 32-bit `+0x64`.
5. The line-of-sight test of the sibling contract (`0x19EE90` / `0x0D4D80`) is called from the chooser's position to the candidate's position with flag 1 and range 200. When it fails, the candidate is passed over.
6. When E(chooser) is 0 the candidate is accepted. When E(chooser) is 1 it is accepted only when the three candidate conditions of step 4 (`+0x68`, `+0x1020`, `+0x1021`) hold.
7. An accepted candidate becomes the best so far and its q the new best value. Because the comparison is strict, the earliest candidate in key order wins a tie.

After the walk, when a best candidate exists, the chooser's threat-table entry for that candidate's key is found or created and set to 1.0, and the propagation routine (7.3) is called with that candidate as the subject, the chooser as the reference creature, and the behaviour routine's fourth stack argument as the output collection.

### 7.2 Threat from healing (`0x195A60` / `0x0CEA80`)

This routine applies a hit record to the world (the counter register). The record holds the source key at `+0x00`, the struck key at `+0x08` and a binary32 amount at `+0x10`; both creatures are looked up by key, and a missing source gives a null source. The struck creature missing ends the routine. The relation is used only on the healing path, which is reached only under conditions earlier in the routine (not specified here) and when the amount is less than 0 (ordered): the struck creature's health is first raised by the amount's magnitude and limited to a maximum. Then, when the world's byte `+0xB4` is 0, the world's creature container is walked in ascending key order; for each non-null entry C (C may be the source or the struck creature):

- R(source, C) is evaluated.
- **False:** nothing happens for C.
- **True:** when C's threat table has no entry for the struck creature's key (a pure search, no insertion), nothing happens. Otherwise C's entry for the source's key is found or created, and the binary32 product amount × 0.1 is subtracted from it (so a heal raises it by a tenth of the healed amount); then the propagation routine (7.3) is called with the source as the subject, C as the reference creature, and the hit routine's second stack argument as the output collection.

No null check of the source was observed on this path between the lookup and the relation call; the behaviour with a missing source is unresolved.

On the damage path (amount greater than or equal to 0, ordered, and source present), the same routine adds to the struck creature's threat-table entry for the source and then calls the propagation routine with the source as the subject and the struck creature as the reference; the relation itself is not called there by this routine.

### 7.3 Threat propagation (`0x1A0970` / `0x0D5F40`)

Called with the world in the counter register and three stack arguments: (1) the subject S (the creature threat is directed at), (2) the reference creature V, and (3) an output collection; it removes 12 bytes. Its three callers are the end of the target choice (7.1, at `0x02D232` / `0x003652`) and the damage and healing paths of hit application (7.2, at `0x195CDF` / `0x0CECFF` and `0x19610F` / `0x0CF12F`).

1. V's threat-table entry for S's key is found or created, and a record (V's key, S's key, that binary32 value) is appended to the list at `+0x50` of the output collection.
2. The world's creature container is walked in ascending key order. An entry C is skipped when it is null, is S or is V (pointer comparisons).
3. R(C, S) is evaluated. **False:** C is skipped. **True:** continue.
4. A squared distance q from V's position to C's position is formed in binary32 exactly as in 7.1 step 2 (without any divisor). C is skipped unless 64 is greater than q (ordered), that is unless C is less than 8 world units from V.
5. C's threat-table entry for S's key is found or created (so the entry exists from this point even if it was absent). When its value compares equal to 0.0 (including −0.0; a NaN or any nonzero value fails), it is set to 0.5 (`0x3F000000`) and a record (C's key, S's key, 0.5) is appended to the same list. Otherwise nothing else happens for C.

So the relation's first argument is the creature that would join, and its second is the creature it would turn against.

### 7.4 Healing-object contact (`0x20C510` / `0x1322D0`, world update)

In the world update's loop over projectiles or objects touching creatures, when the object's byte `+0x64` is 2 and its owning creature is non-null, R(owner, touched creature) is evaluated:

- **True**, or when the touched creature's role is 6: no hit is produced by this branch.
- **False** and the touched role is not 6: a hit record is built with the object's first 64-bit value as the source key, the touched creature's key as the struck key, and an amount equal to the negated value prepared by the loop (multiplied by 0.5 when the owner's role is 1); the record is queued, and under world conditions not specified here it is applied at once by the hit routine (7.2), where the negative amount heals.

When the owner is null this branch produces no hit either. So a healing object heals exactly the creatures its owner would not target, excluding role 6.

## 8. Client and server

Each item below was established in its own image:

- The relation: 60 instructions in each image, byte-identical except for the four relative call displacements, each of which reaches that image's E.
- E: 72 instructions in each image, byte-identical.
- The propagation routine: 191 instructions in each image, identical after masking absolute addresses. Its binary32 constants have identical bytes: 2^−16 (client `0x2FCD94`, server `0x15869C`) and 64 (client `0x2FD330`, server `0x15882C`).
- The hit routine and the behaviour routine are identical after masking from their first instruction through and beyond the call sites described above; their first differences (client `0x196B05`, client `0x02D964`) lie after those sites and were not characterised. The healing constants have identical bytes: 0.1 (client `0x2FCB7C`, server `0x1586BC`); 0.5 (client `0x345DA0`, server `0x1586D0`); the sign mask (client `0x345F70`, server `0x1587C0`).
- The world-update caller agrees in a window of 120 instructions on either side, after masking.

No behavioural difference was found.

## 9. Unresolved

- The game meaning of the role values, of E's list and flag bits, of toggle bit 5, and of the candidate conditions on `+0x68`, `+0x1020` and `+0x1021` (no producers were traced for this contract).
- The parts of the target-choice routine outside the candidate search (when the search runs, the routine-local flag, the `+0x1190` divisor) and of the hit routine outside the described branches.
- The contents and consumer of the output collection's record list, and the full layout of its records.
- The behaviour of the healing path of the hit routine when the source creature is missing.
- The meaning of the world's byte `+0xB4` and the conditions under which the healing-object hit is applied at once.
- The differing portions of the hit and behaviour routines between the two images, which lie outside the described sites.
