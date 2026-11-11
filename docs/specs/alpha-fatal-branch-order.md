Status: FINAL / STABLE / HELD.

# Hit application: ordered fatal branch

This independently authored, code-free contract gives the complete ordered sequence of the fatal branch of the hit-application routine, from the point where post-application health at or below zero is detected to the routine's return. The two earlier contracts `alpha-fatal-world-drop-production.md` and `alpha-fatal-loot-branches.md` were used only to locate the hit routine, its fatal call and the fatal-loot helper; nothing in them is relied on as evidence here, and the internal behaviour of the fatal-loot helper is not restated. The Infinite-Cubes reconstruction was not consulted.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`; every address below is an RVA. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

The routine and every callee named below were read in `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. For the whole hit routine in each image the listing's byte column was checked against the image bytes at the same address; there was no mismatch. The client listing omits two short instruction runs inside the fatal branch (RVAs `0x196AFC–0x196B04` and `0x196B0B–0x196B13`); those bytes were read from the client image and decode to the same instructions the server listing shows at the corresponding place. Other listing gaps in the routine are alignment padding after unconditional jumps. Client and server were then compared instruction by instruction after replacing absolute addresses. The hit routine and every callee listed in section 2 have the same instruction sequence, immediates, displacements and widths. The one exception is the fatal-loot helper, which was not compared here. Some client listings omit a short instruction run near a routine's end. Those bytes were read from the client image and match the server. Each differing absolute address (callee, constant, import slot, jump table) was resolved in its own image, and the constants and switch tables were read from each image's bytes. Import slots were resolved by walking each image's import directory. Decompiler output and the call-graph files `decompiled/*/calls.csv` were used as leads only, except where section 6 states that a negative result rests on the call graph.

## 2. Routines

| Role | Client RVA | Server RVA | Audit state |
| --- | --- | --- | --- |
| Hit-application routine | `0x195A60` | `0x0CEA80` | fatal branch fully audited |
| Fatal admission test | `0x196133` | `0x0CF153` | audited |
| Coordinate-to-presentation conversion | `0x02C4A0` | `0x002550` | audited |
| 24-byte record list append | `0x0864C0` | `0x028590` | audited (node creation `0x18D860` / `0x0F3BA0` audited) |
| Resident lookup | `0x034D10` | `0x006290` | audited, with parent lookup `0x034A90` / `0x006210` |
| Resident-entry state change | `0x1A58E0` | `0x0D8C90` | audited |
| Entry-record copy | `0x068670` | `0x022DA0` | audited |
| Entry-record list append | `0x0DD700` | `0x0284F0` | audited |
| Fatal-loot helper | `0x19B180` | `0x0D2AE0` | call boundary only; see the earlier contracts |
| Creature-map find | `0x077D90` | `0x130600` | audited |
| Map in-order successor | `0x1C3EA0` | `0x007A50` | audited |
| Raw coordinate difference | `0x02C7A0` | `0x002C50` | audited |
| Level-to-fraction curve | `0x03CA60` | `0x007D60` | audited |
| Kind-set predicate | `0x0445D0` | `0x00F650` | audited |
| Region-object lookup | `0x087DA0` | `0x0286F0` | audited |
| Creature lookup returning actor | `0x02F000` | `0x005420` | audited |
| Region-record copy | `0x068620` | `0x023C90` | audited |
| Progress-record clear | `0x065C70` | `0x022D00` | audited |
| Progress-record list append | `0x086420` | `0x0284A0` | audited |
| Pointer-vector resize | `0x00D9D0` | `0x05F080` | audited, except its growth callee |
| Temporary list sentinel | `0x06D550` | `0x0CE660` | audited |
| Temporary list node | `0x183CB0` | `0x12DFB0` | audited |
| Tree erase at node | `0x03EDE0` | `0x00A1D0` | bounded: erase, size decrement |
| Credit-record list append | `0x051D00` | `0x0203C0` | audited |
| Credit application | `0x1A0BF0` | `0x0D61C0` | audited |
| Level-up loop (inside credit) | `0x047B00` | `0x012550` | audited, except the maximum-health routine |
| Tree clear (inside credit) | `0x27E480` | `0x0F8520` | audited |
| Keyed item-record list append | `0x051CC0` | `0x020380` | audited |
| Inventory insertion | `0x06EBE0` | `0x027000` | bounded by inputs, draws and recursion only |
| Item generator P | `0x1F8AD0` | `0x12C4E0` | audited for draws and candidates |
| Item generator Q | `0x1F51E0` | `0x128BF0` | audited for draws and candidates |
| Item copy | `0x02C5E0` | `0x002A70` | not re-audited; no draws by call graph |
| Maximum-health routine | `0x044DB0` | `0x00FDA0` | not audited; no draws by call graph |

The random source is the import slot at client RVA `0x2FC468` and server RVA `0x1582F4`; in each image the slot resolves to `MSVCR110.dll!rand`. The length-error import at client `0x2FC1F0` and server `0x158160` resolves in each image to the standard-library length-error thrower.

Binary32 constants used in the branch (client RVA / server RVA, bits): 1 at `0x345DC0` / `0x1586DC` (`0x3F800000`); 20 at `0x2FCD74` / `0x15874C` (`0x41A00000`); 10 at `0x345E74` / `0x15873C` (`0x41200000`); 50 at `0x2FCD7C` / `0x158828` (`0x42480000`); positive zero at `0x345D84` / `0x158698` (`0x00000000`); 2 to the power −16 at `0x2FCD94` / `0x15869C` (`0x37800000`); nearest binary32 to 0.05 at `0x2FCDB4` / `0x1586B8` (`0x3D4CCCCD`); 1000 at `0x2FD8F4` / `0x1588F8` (`0x447A0000`).

## 3. Vocabulary

The routine receives a world receiver and three stack arguments, in order: the hit record, an output aggregate and a collection pointer. It removes 12 bytes of arguments on return.

The **creature map** is the ordered tree whose head pointer is at world `+0x04`. Its nodes hold a 64-bit key at node `+0x10` and an actor pointer at node `+0x18`. Lookup compares keys as signed 64-bit values (high word signed, low word unsigned); iteration runs in ascending key order. A lookup **fails** when it returns the head or when the found node's actor pointer is null. Each actor holds another tree of the same shape at actor `+0x13A4` (the **contribution map**), whose node value at node `+0x18` is read as binary32.

The **target** is the actor obtained at routine entry from the creature map using the 64-bit key at hit-record `+0x08` (the **hit key**). That pointer is kept for the whole branch; the target is not looked up again. The hit key itself is reread from the hit record whenever it is compared.

The **output aggregate** contains five doubly linked lists used by this branch, each with its head pointer at the stated offset and its element count in the following word: `+0x08` (24-byte presentation records), `+0x30` (`0x58`-byte entry records), `+0x40` (keyed item records), `+0x48` (24-byte credit records), `+0x60` (`0x38`-byte progress records). Every append adds at the tail after a count-limit check; exceeding the limit calls the length-error import. Records are copied member by member, so padding bytes are not established.

A **presentation record** is three binary32 coordinates, a 32-bit identifier and two binary32 values. Its coordinates are made from three signed 64-bit raw coordinates: each is loaded exactly onto the x87 stack, stored as binary32 under the x87 rounding control, and multiplied (SSE, binary32) by 2 to the power −16. Its two trailing values are always 1 in this branch.

The **level-to-fraction curve** of a binary32 L is 1 minus 1 divided by ((L minus 1) times 0.05 plus 1), as separate binary32 operations; the result is returned through the x87 stack as an exact binary32 load.

The **resident lookup** takes signed 32-bit X then Y. Either component negative or at least 65536 fails. Otherwise the parent is read from the world pointer grid at `+0xBC` at index (X divided by 64) times 1024 plus (Y divided by 64), with a failure for parent components outside 0 to 1023; a null parent fails. Otherwise the result is the pointer at parent `+0x10018` indexed by (X mod 64) times 64 plus (Y mod 64). Divisions truncate toward zero and remainders are signed; the result may be null.

## 4. Admission and the window before it

At entry the routine continues only if the target's binary32 health at actor `+0x16C` is greater than zero or is NaN (an unordered comparison passes). [Client `0x195AED–0x195AF7`; server `0x0CEB0D–0x0CEB17`.]

The fatal admission test is a single point reached by every path through the earlier hit-application work, whether or not that path wrote health. It compares the target's binary32 `+0x16C` against binary32 positive zero. The fatal branch is entered exactly when the comparison is ordered and health is less than or equal to zero; this includes negative zero, negative values and negative infinity. Positive health and NaN return immediately without any further effect. [Client `0x196133–0x196143`; server `0x0CF153–0x0CF163`.]

Between the health writes and this test:

- On the positive-damage path that subtracts damage from health, the subtraction result is stored and control jumps straight to the test; nothing intervenes. [Client `0x195FEE–0x196003`; server `0x0CF00E–0x0CF023`.]
- On the negative-damage path, health minus the (negative) value is stored; the maximum-health routine is called, and if the stored health is greater than its result, the routine is called again and its value is stored as health. Then, only when the world byte at `+0xB4` is zero, every creature-map entry is visited with a routine at client `0x196CA0` / server `0x0CFCC0` and possibly an adjustment at client `0x1A0970` / server `0x0D5F40`. These belong to the non-fatal hit work and are not audited here, except that the call graph shows no random call and no path back to the hit routine in them. A health value made non-positive by the maximum-health clamp would pass the fatal test. [Client `0x196008–0x19612D`; server `0x0CF028–0x0CF14D`.]
- Paths that make no health change also reach the test. Since entry required positive or NaN health, such a path reaches the fatal branch only if health was changed by something called earlier in the same invocation; whether that can happen is not established.

## 5. Ordered fatal sequence

Steps F1 to F12 run in this order. Each step reads target fields from the target record at the moment it uses them; where a step rereads a field after an intervening call, this is stated.

**F1. Health zeroing.** Target `+0x16C` is written with the 32-bit value zero (binary32 positive zero). [Client `0x196150`; server `0x0CF170`.]

**F2. Fatal presentation record.** A presentation record is formed from the target's raw coordinates at `+0x10`, `+0x18`, `+0x20`, with identifier 27 and trailing values 1 and 1, and appended to output list `+0x08` (limit `0x7FFFFFE` elements). [Client `0x196149–0x196181`; server `0x0CF169–0x0CF1A1`.]

**F3. Target state word.** Target 32-bit `+0x6C` is written with zero. [Client `0x19618C`; server `0x0CF1AC`.]

**F4. Resident entry handling.** The resident lookup is called with target 32-bit `+0x1B0` and `+0x1B4`. If it fails (null), F4 ends. Otherwise the resident's entry vector, begin at resident `+0x0C` and end at `+0x10`, is scanned in ascending order with `0x188`-byte stride and a zero-based ordinal. The first entry whose 32-bit `+0x00` equals 5, whose 32-bit `+0x178` equals target 32-bit `+0x1B8`, and whose byte `+0x30` is nonzero is handled, and the scan stops; if none matches, F4 ends. Handling, in order: [Client `0x196186–0x19621E`; server `0x0CF1A6–0x0CF23E`.]

1. The resident-entry state change is called with the entry, the output aggregate and a zero byte. Because the entry byte `+0x30` differs from that zero byte, it stores 1000 minus entry 32-bit `+0x34` into `+0x34`, replacing it with zero when the 32-bit difference has its sign bit set; writes zero to entry byte `+0x30`; and, because the entry type is 5, appends to output list `+0x08` a presentation record made from the entry's raw coordinates at `+0x08`, `+0x10`, `+0x18`, with identifier 51 and trailing values 1 and 1. (For other entry types, not reachable from this call, the routine uses identifier 54, or for types 6 and 7 identifiers 53 and 52 only when its byte argument is nonzero.) [Client `0x1A58E0–0x1A5A03`.]
2. An entry record is formed: target `+0x1B0` and `+0x1B4` reread after step 1, the ordinal, four unwritten bytes, and a copy of the entry's members at `+0x00` and `+0x08` to `+0x47` (32-bit words, with byte `+0x30`; entry words `+0x04` and `+0x3C` and bytes `+0x31–+0x33` are not copied). It is appended to output list `+0x30` (limit `0x2AAAAA9`).

**F5. Fatal-loot call.** The fatal-loot helper is called with the world receiver, the target pointer and the routine's third argument (the collection pointer). Its admission, branches, draws and world-drop production are specified elsewhere and are not restated. [Client `0x196221–0x19622E`; server `0x0CF241–0x0CF24E`.]

**F6. Discarded difference pass.** Every creature-map entry is visited in ascending key order. For each with a nonnull actor whose byte `+0x60` is zero, the three raw 64-bit differences between that actor's coordinates at `+0x10` and the target's are computed into a local and discarded. This pass has no observable effect. [Client `0x196233–0x196277`; server `0x0CF253–0x0CF297`.]

**F7. Linked-actor health.** If target 64-bit `+0x11C8` (the **linked key**) is nonzero, the creature map is searched for it. If the lookup succeeds, it is searched again and the found actor's `+0x16C` is written with zero. Nothing else is done to that actor here; in particular the fatal branch is not run for it. A failed lookup skips this step. [Client `0x196279–0x1962DC`; server `0x0CF299–0x0CF2FC`.]

**F8. Credit value.** A binary32 credit value V is computed once, in order: [Client `0x1962E6–0x196385`; server `0x0CF306–0x0CF3A5`.]

1. Zero-extended target byte `+0x1A8` plus signed 32-bit target `+0x190` (32-bit wrapping addition), converted to binary32, passed through the level-to-fraction curve, and multiplied by 20.
2. If 1 is greater than V (ordered), V becomes 1; a NaN V is kept.
3. If target 32-bit `+0x64` is one of 101, 107, 108, 109, 114, 115, 116, 117, 118 or 119, V is multiplied by 20 again. [Kind-set predicate, client `0x0445D0`.]
4. The 16-bit target word `+0x7E` is read once into a flags value used here and in F9. If bit `0x0200` is set, V is multiplied by 10. If bit `0x0800` is set, V becomes positive zero.

**F9. Regional progress.** A local completion flag is cleared and a local `0x28`-byte region copy is partly zeroed (bytes `+0x16` and `+0x17` are not). Then, only when world byte `+0xB4` is zero: [Client `0x19638D–0x196685`; server `0x0CF3AD–0x0CF6A5`.]

1. Region lookup. X8 and Y8 are target signed 32-bit `+0x1B0` and `+0x1B4` each divided by 8, truncating toward zero. The region-object lookup fails if either is negative or at least 8192. Otherwise it calls the parent lookup with (X8 times 8) divided by 64 and (Y8 times 8) divided by 64; a null parent fails. Otherwise the region object is at parent `+0x14018` plus ((X8 mod 8) times 8 plus (Y8 mod 8)) times `0x68`. If the lookup fails, or region byte `+0x41` equals 2, F9 ends.
2. Type gate. If flags bit `0x2000` is clear, F9 ends unless region 32-bit `+0x34` equals 5 and target byte `+0x1D38` is nonzero. If the bit is set, no gate applies here.
3. Contributor check. The target's contribution map is walked in ascending key order. A node qualifies when its binary32 value is greater than positive zero (ordered), its key is found in the creature map with a nonnull actor, and the actor returned by a second lookup of that key has byte `+0x60` equal to zero. The walk stops at the first qualifying node; that actor is not used further. If no node qualifies, F9 ends.
4. State update by region type T (32-bit `+0x34`). For T of 1 to 5 or 13: region byte `+0x41` becomes 2 and the completion flag is set. For T of 7 to 12: region 32-bit `+0x44` is incremented, byte `+0x41` becomes 1, and then, if `+0x44` is at least `+0x48` (signed), the completion flag is set and `+0x41` becomes 2. For T of 6, or outside 1 to 13 (unsigned test of T minus 1 against 12), F9 ends with no record. [Switch tables client `0x196C78` / `0x196C84`, server `0x0CFC98` / `0x0CFCA4`; identical index bytes in both images.]
5. Progress record. The region's members from `+0x2C` to `+0x53` are copied into the local region copy (32-bit words at `+0x2C`, `+0x30`, `+0x34`, `+0x38`, `+0x3C`; bytes `+0x40`, `+0x41`; 32-bit words `+0x44` to `+0x50`; region bytes `+0x42–+0x43` are not copied). A `0x38`-byte record is cleared except its bytes `+0x26–+0x27`, given X8 and Y8 recomputed from the target's current `+0x1B0` and `+0x1B4`, and given the local copy at record `+0x10`. It is appended to output list `+0x60` (limit `0x3FFFFFE`). This happens whether or not the completion flag was set.
6. Disposal. Only when the completion flag is set: for each X from (X8 times 8) up to but excluding X8 times 8 plus 8, and within it each Y from (Y8 times 8) up to but excluding Y8 times 8 plus 8, with both bounds recomputed from the target's current fields at each iteration, the resident lookup is called with X and Y. For each nonnull resident, its pointer vector (begin `+0x18`, end `+0x1C`, 4-byte elements) is walked from the signed index at resident `+0xA0` while the index is below the current element count; each nonnull element has the first slot of its virtual table called with argument 1. Then the vector is resized to the `+0xA0` count: a larger vector is truncated (element words are not cleared), a smaller one is grown with zero-filled elements, an equal one is unchanged. The called virtual routines are not resolved.

**F10. Per-actor pass.** Every creature-map node is visited in ascending key order. Nodes with a null actor are skipped. For each actor A, in order: [Client `0x196685–0x196B59`; server `0x0CF6A5–0x0CFB79`.]

1. Relationship field. If A's 64-bit `+0x11D0` equals the hit key or equals target 64-bit `+0x11C8`, it is written with 64-bit zero.
2. Contribution cleanup. A temporary list is created; A's contribution map is walked in ascending order and every node whose 64-bit key equals the hit key or target `+0x11C8` is recorded (limit `0x15555554`). Each recorded node is then erased from A's contribution map in recorded order.
3. Credit and reward, only when target byte `+0x60` equals 1 and A byte `+0x60` equals 0:
   1. The target's contribution map is searched for A's creature-map key (the node key, not any identifier inside A). If found, a credit record is formed: A's key, the hit key, target 32-bit `+0x64`, and V converted to signed 32-bit by truncation toward zero (NaN or out-of-range gives `0x80000000`). It is appended to output list `+0x48` (limit `0x7FFFFFE`), and then the credit application (section 7.3) is called with the world receiver and that record. If not found, neither happens.
   2. Whether or not credit was found, if the completion flag from F9 is set, a reward is produced. Let R be the zero-extended byte at local-copy `+0x14` (region `+0x40`) plus the 32-bit word at local-copy `+0x10` (region `+0x3C`). An item is formed with category 13, subtype 0, `+0x04`, `+0x08`, the 16-bit `+0x0C` and byte `+0x0E` zero, 256 extension bytes zero, extension count zero, and level word equal to the low 16 bits of the truncation toward zero of (curve of R converted to binary32) times 50 plus 1. Item bytes `+0x02`, `+0x03`, `+0x0F`, `+0x12`, `+0x13` are not written. A keyed record of A's key followed by this item is appended to output list `+0x40` (limit `0xDD67C7`). If A equals the world pointer at `+0xB8`, the inventory insertion is called on A's container at `+0x11DC` with the item and a count argument of −1.
   3. One direct random call is made. Odd selects generator P, even selects generator Q. The generator is called with a local destination, the region-copy word (`+0x3C`) as level, the region-copy byte (`+0x40`) as grade, and A's byte `+0x140` zero-extended as selector. Its result is copied over the record's item (the key is unchanged), the record is appended again to list `+0x40`, and if A equals world `+0xB8` the inventory insertion is called again with the generated item and −1.
4. The temporary list is released.

**F11. Object association.** Using target signed 32-bit `+0x1B0` as X and `+0x1B4` as Y, the resident lookup rules of section 3 are applied inline. If any check fails or the resident is null, F11 ends. Otherwise, if the signed 32-bit index at target `+0x1B8` is nonnegative and below the element count of the resident's pointer vector (`+0x18` / `+0x1C`), the element pointer at that index is read without a null check; its 32-bit `+0x38` is written with `0x124F80` (1,200,000) and its 32-bit `+0x3C` with the world's 32-bit `+0x800160`. [Client `0x196B5F–0x196C49`; server `0x0CFB7F–0x0CFC69`.]

**F12. Return.** The exception frame and security cookie are restored and the routine returns, removing 12 bytes of arguments. No value is returned to the caller. [Client `0x196C4C–0x196C67`; server `0x0CFC6C–0x0CFC87`.]

## 6. Random draws and recursion

Direct random calls in the branch: exactly one call site, at client `0x196A1E` / server `0x0CFA3E` (F10 step 3.3). A byte scan of each image over the whole fatal branch found exactly one reference to the random import slot. It executes once per actor A that satisfies the F10 step 3 gate while the completion flag is set; the parity of its result is the only use.

Nested draws, in branch order:

| Step | Routine | Draws |
| --- | --- | --- |
| F2, F4, F6, F7, F8, F9, F11 | all identified helpers named there | none |
| F5 | fatal-loot helper | variable; specified elsewhere; all its draws precede every F10 draw |
| F9 | virtual disposal routines | unresolved |
| F10 | credit application, level-up loop, tree erase and clear, list helpers | none |
| F10 | inventory insertion | none (by call graph; body not audited) |
| F10 | generator P, selector 1 / 2 / 3 / 4 | 6 / 4 / 5 / 4 draws, giving 3 / 2 / 3 / 3 candidates |
| F10 | generator P, any other selector | 1 draw, no candidates, then an unsigned division by zero |
| F10 | generator Q, selector 1 / 2 / 3 / 4 | 9 / 10 / 10 / 10 draws, giving 6 / 7 / 7 / 7 candidates |
| F10 | generator Q, any other selector | 5 draws, 2 candidates |

The generators were audited for their candidate rules: each candidate is built and appended in order, and each consumes the draws stated for it; the last draw, divided unsigned by the candidate count, selects the candidate copied into the destination. A group is included when the selector equals its number or is negative; the selector here is never negative. Generator P (every candidate category 3, level word from the low 16 bits of the level argument, byte `+0x0C` from the low 8 bits of the grade, `+0x04` from a draw modulo 100, `+0x08` and `+0x0E` zero, no extensions): group 1 gives a subtype draw modulo 3, a modifier draw, a subtype draw modulo 3 plus 15, a modifier draw, then subtype 13 with a modifier draw, all with byte `+0x0D` 1; group 4 gives subtypes 3, 5, 4 each with a modifier draw, byte `+0x0D` 1; group 2 gives subtype 6 with a modifier draw, one discarded draw, then subtype 8 with a modifier draw, byte `+0x0D` 2; group 3 gives subtypes 10 and 11 each with a modifier draw, byte `+0x0D` 2, then a parity draw (odd 11, even 12) for byte `+0x0D` and subtype 12 with a modifier draw. Generator Q (every candidate subtype 0, other fields as P): selector 1 gives categories 7, 4, 5, 6 with byte `+0x0D` 1; selectors 3, 2 and 4 give categories 7, 7, 4, 5, 6 with byte `+0x0D` 25, 26 and 27 respectively; then always a parity draw (odd 12, even 11) with category 8 and a modifier draw, and a parity draw with category 9 and a modifier draw. Each count in the table is the group's draws plus the final selection draw. [Client `0x1F8AD0–0x1F8E01`, `0x1F51E0–0x1F56B5`; server `0x12C4E0–0x12C811`, `0x128BF0–0x1290C5`; ranges end at each routine's return instruction.]

Recursion. No routine called in the fatal branch, nor any routine reachable from them in the call graph, reaches the hit routine or its identified callers (client `0x196D30`, `0x20C510`; server `0x0CFD50`, `0x1322D0`) in either image. The only unresolved indirect calls on those paths are the security-cookie failure trap and the F9 virtual disposal calls. The call graph also lists call sites into the hit routine that it does not attribute to any function: client `0x214BAC` and `0x214F72`, server `0x13A96C` and `0x13AD32`. The routines containing them are therefore not covered by this negative result. The linked-actor health writes in F7 and in the credit application set health to zero directly and do not invoke the fatal branch for that actor. Whether any routine run later treats those actors as fatal is outside this contract.

## 7. Actor access

### 7.1 Actors read and written

- Target (the entry pointer): reads `+0x10–+0x27`, `+0x60`, `+0x64`, `+0x7E`, `+0x190`, `+0x1A8`, `+0x1B0`, `+0x1B4`, `+0x1B8`, `+0x1D38`, `+0x11C8`, and its contribution map; writes `+0x16C` (F1) and `+0x6C` (F3). The loot helper receives the pointer itself.
- Linked actor (creature-map lookup of target `+0x11C8`): `+0x16C` written with zero (F7).
- Every creature-map actor A: `+0x11D0` possibly cleared and its contribution map edited (F10.1–F10.2); reads of `+0x60` and `+0x140`; for qualifying A, credit and reward through the routines below. A's identity in records is its creature-map node key.
- Contributor actors in F9 are only tested.
- Resident entries, region objects and resident pointer-vector elements are written as described in F4, F9 and F11.

### 7.2 Lookup failures

A failed resident lookup ends F4, skips that cell in F9 disposal, and ends F11. A failed linked-key lookup skips F7. A failed region lookup, a state byte of 2, a failed type gate or no qualifying contributor ends F9 without a progress record and without completion. A missing contribution entry for A skips that credit but not the reward. No failure ends the branch early: every later step still runs, except that F9 is skipped entirely when world byte `+0xB4` is nonzero.

### 7.3 Credit application (`0x1A0BF0` / `0x0D61C0`)

Input: the world receiver and a 24-byte record (actor key, target key, kind, signed 32-bit credit). In order:

1. Both keys are looked up in the creature map. If either fails, the routine returns.
2. If the target found equals world `+0xB8`: the creature map is searched for that actor's `+0x11C8`; if found with a nonnull actor, that actor's `+0x16C` is set to zero and its contribution map is cleared. Then world `+0xB8`'s 64-bit `+0x11D0` is set to zero, whether or not the search succeeded.
3. If world `+0xB8` is nonnull, equals the credited actor, and the target's byte `+0x60` equals 1: the credit is added (32-bit wrapping) to the credited actor's `+0x194`, then the level-up loop runs on it.
4. If world byte `+0xB4` is zero: the creature map is searched for the credited actor's `+0x11C8`. If found with a nonnull actor whose binary32 `+0x16C` is greater than zero (ordered): the credit is added to that actor's `+0x194`, the level-up loop runs on it, the credited actor's 32-bit `+0x1024` receives that actor's `+0x194`, its 16-bit `+0x1030` the low 16 bits of that actor's `+0x190`, and the item record at its `+0x1020` is copied to its `+0x1D48`.

Level-up loop (`0x047B00` / `0x012550`): if signed level `+0x190` is not positive, nothing happens. Otherwise, while `+0x194` is at least the threshold (signed), the threshold is subtracted, the level is incremented and `+0x16C` receives the maximum-health routine's result. The threshold is the truncation toward zero of (level-to-fraction curve of the level, computed inline) times 1000 plus 50.

### 7.4 Inventory insertion (`0x06EBE0` / `0x027000`)

It receives A's container at `+0x11DC`, an item and −1. It is reached only for A equal to world `+0xB8`. Its internal placement rules are not audited. By the call graph it makes no draws and does not reach the hit routine.

## 8. Client and server

Each item below was established separately in each image. The fatal branch, from the admission test to the return, has the same instruction sequence in both images. The differences are absolute addresses, all mapped in section 2. The jump tables and constants were read from each image and agree in value. Every callee in section 2 except the fatal-loot helper was compared instruction by instruction and agrees, including those whose bodies were not audited for behaviour. The client listing's omitted bytes were confirmed in the image. No client/server behavioural difference exists in the instructions of this branch.

The runtime values that steer it are not established for either program: world byte `+0xB4`, world pointer `+0xB8`, the world grids, and the output lists' later consumers. Identical instructions therefore do not imply identical outcomes in a running client and server.

## 9. Unresolved

- The game meaning of the fields, kinds, region types, entry types, identifiers 27 and 51, and of the output lists is not asserted.
- The F9 virtual disposal routines: their identity, draws and possible recursion.
- The fatal-loot helper's effects and draws, beyond its call position.
- The internals of the maximum-health routine and the inventory insertion, beyond the absence of draws and recursion in the call graph.
- Whether a non-damage path can reach the fatal test with non-positive health. Also whether the maximum-health clamp can store a non-positive value.
- Whether generator P can be reached with a selector outside 1 to 4 (a division fault).
- The routines containing the unattributed call sites in both images, and everything upstream of the hit routine.
- Runtime values of world `+0xB4` and `+0xB8` in each program, and how the output lists are consumed or transmitted.
- Padding and unwritten bytes of records and items; floating-point control state, exceptions and non-finite inputs beyond the cases stated; concurrency and actor lifetime during the branch.
