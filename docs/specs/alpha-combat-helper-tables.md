Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha combat helper tables (timing terms and action starters)

## Scope and evidence

This contract closes the internal decision tables of the six subject helpers that the held contract `alpha-created-actor-combat.md` leaves bounded but unresolved: the timing queries `0x044270`, `0x03CAA0`, `0x047310` and `0x03D1A0`, and the action starters `0x045410` and `0x0456A0`. It also closes the selectors those starters call (`0x045020`, `0x0452A0`, `0x044230`) and the cost query (`0x044AE0`) that `0x0456A0` tests against. The held contracts `alpha-created-actor-combat.md` and `alpha-combat-stat-queries.md` were used only to locate routines and for terms. Every fact below was read again from the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs (image base removed). Evidence:

- Instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, read for each routine's full address range taken from `decompiled/<image>/function_ranges.csv`.
- Image bytes of both executables: every jump index table, jump target table and binary32 constant named here was read from the file through its section mapping.
- `decompiled/<image>/calls.csv`, for the transitive draw check (Section 10).
- A byte scan of every executable section of both images for `E8`/`E9` rel32 transfers to each helper (Section 9). Each hit was checked against the listing; hits that the listing does not decode are marked.

Decompiler function files were not needed; no decompiler output is relied on. The Infinite-Cubes reconstruction was not consulted.

## Terms

- **Subject**: the creature passed as the receiver (in the `ECX` register) of each helper. Offsets without another base are subject offsets.
- **Action**: byte `+0x68`. **Clock**: signed 32-bit `+0x6C`. **Lockout**: signed 32-bit `+0x128`. **Counter70**: signed 32-bit `+0x70`. **Mana**: binary32 `+0x170`. **Speed scale**: binary32 `+0x17C`.
- **Second slot**: the equipment record at `+0xAA8`; its kind byte is `+0xAA8` and its subtype byte `+0xAA9`. **First slot**: the record at `+0x990`; kind `+0x990`, subtype `+0x991`.
- **Class byte**: `+0x140`. **Sub-class byte**: `+0x141`. **Type value**: signed 32-bit `+0x64`. **Flag byte**: the low byte of the 16-bit word `+0x7E`.
- **Effect list**: the circular list headed at `+0x1178`. A *kind-k element is present* when a walk from the head's first link, following next links until the head is reached again, finds a node whose byte at node `+0x08` equals k. The walk stops at the first such node.
- **Q**: the subject's attack-speed query `0x047700` (server `0x012150`). It is bounded here only: it takes the subject, makes no writes and no draws, and returns binary32 through the x87 stack. Its formula is outside this contract.
- **Parity of Counter70**: the signed remainder of Counter70 divided by 2. It is −1, 0 or +1; *odd* means nonzero (so negative odd values count as odd).
- **Ranged condition P**: true when any of the following holds, tested in this order: class byte = `3`; type value = `0x75`; type value = `0x56`; second-slot kind = `3` and second-slot subtype is `0x0A`, `0x0B` or `0x0C`. This condition is written out inline in each routine that uses it; it was read from those routines directly.
- **Combo rule (c → n, d)**: if the action equals c, R0 (Section 5) is evaluated for the subject and the result is n when the clock is strictly less than (R0 + 200), computed with 32-bit wrapping addition and compared signed. In every other case the result is d. R0 is evaluated only when the action equals c.

All numbers are hexadecimal when written with `0x`, decimal otherwise.

## 1. Timing evaluation rule

The four timing queries, and the timing tables copied inline into the starters, each map an action code to one *timing value* of one of two forms:

- **fixed V**: the signed 32-bit integer V. Q is not evaluated.
- **scaled K**: Q is evaluated once (one call), its x87 result is stored as binary32, that value is multiplied by the speed scale (binary32 product, one rounding), K is divided by the product (binary32 quotient, one rounding), and the quotient is converted to signed 32-bit by truncation toward zero. An out-of-range or NaN quotient gives `0x80000000`; a zero product gives an infinite quotient and therefore `0x80000000`.

Every K is a binary32 constant with an exact integer value. Client constant locations: 10 `0x41200000` (`0x345E74`), 50 `0x42480000` (`0x2FCD7C`), 100 `0x42C80000` (`0x2FD0B4`), 150 `0x43160000` (`0x2FD8E8`), 200 `0x43480000` (`0x2FCE60`), 300 `0x43960000` (`0x2FCE68`), 400 `0x43C80000` (`0x2FD8EC`), 500 `0x43FA0000` (`0x2FCE70`), 600 `0x44160000` (`0x2FCE74`), 800 `0x44480000` (`0x2FD8F0`), 1000 `0x447A0000` (`0x2FD8F4`), 1200 `0x44960000` (`0x2FD8F8`), 1600 `0x44C80000` (`0x2FD8FC`), 5000 `0x459C4000` (`0x2FD900`).

Each table is a byte index table whose entry selects one dword of a target table; action values above the table's limit (compared unsigned) take the table's default target. Different index values that lead to targets of the same form and value are merged in the tables below.

## 2. A: `0x044270` (server `0x00F2F0`)

**Interface.** Receiver: subject. One stack argument: a signed 32-bit action code, removed by the routine. Result: signed 32-bit in `EAX`. When the argument is negative the action byte (zero-extended) is used instead; otherwise the argument itself is used. Codes above `0x68` take the default. Index table `0x04442C` (105 entries), target table `0x0443EC` (16 targets). Fields read: the action byte (only for a negative argument), the speed scale, and Q's inputs. No writes, no draws.

| A | Codes |
|---|---|
| fixed 0 | `0x30` |
| fixed 100 | `0x00`, `0x32`, `0x60` |
| fixed 200 | `0x47`, `0x48` |
| fixed 400 | `0x36` |
| fixed 600 | `0x0A` |
| scaled 10 | `0x17` |
| scaled 100 | `0x0B`, `0x16`, `0x1A`, `0x1E`–`0x22`, `0x25`–`0x2E`, `0x3C`, `0x3D`, `0x43`, `0x5E`, `0x5F`, `0x68` |
| scaled 200 | `0x07`, `0x0E`, `0x12`, `0x41`, `0x42` |
| scaled 300 | `0x03`, `0x04`, `0x05`, `0x39`, `0x3A`, `0x3E` |
| scaled 400 | `0x0F` |
| scaled 800 | `0x44`, `0x45`, `0x5D` |
| scaled 500 (default) | `0x01`, `0x02`, `0x06`, `0x08`, `0x09`, `0x0C`, `0x0D`, `0x10`, `0x11`, `0x13`, `0x14`, `0x15`, `0x18`, `0x19`, `0x1B`, `0x1C`, `0x1D`, `0x23`, `0x24`, `0x2F`, `0x31`, `0x33`, `0x34`, `0x35`, `0x37`, `0x38`, `0x3B`, `0x3F`, `0x40`, `0x46`, `0x49`–`0x5C`, `0x61`–`0x67`, and every code above `0x68` |

Row counts over the codes `0x00`–`0x68`: 1 + 3 + 2 + 1 + 1 + 1 + 24 + 5 + 6 + 1 + 3 + 57 = 105.

This table is identical, code for code, to the third (recovery) term inside R0 (Section 5), to the inline table in `0x045410` and to the first inline table in `0x0456A0`. Each of the four copies was decoded separately.

## 3. B: `0x03CAA0` (server `0x007DB0`)

**Interface.** As for A (one signed 32-bit stack argument, negative means the action byte, result signed 32-bit). Codes above `0x69` take the default. Index table `0x03CEB0` (106 entries), target table `0x03CE24`. Fields read: action byte (negative argument only), second-slot subtype `+0xAA9` (only for the conditional row), speed scale, Q's inputs. No writes, no draws.

| B | Codes |
|---|---|
| fixed 0 | `0x00`, `0x08`, `0x0B`, `0x1C`, `0x32`, `0x37`, `0x3C`, `0x3D`, `0x3E`, `0x60`, `0x62`, `0x68` |
| fixed 100 | `0x30`, `0x65` |
| fixed 200 | `0x47`, `0x48` |
| fixed 400 | `0x36` |
| fixed 5000 | `0x69` |
| scaled 50 | `0x06`, `0x07`, `0x17`, `0x18`, `0x19`, `0x1B`, `0x24`, `0x3B`, `0x3F`, `0x40` |
| scaled 100 | `0x02`, `0x03`, `0x04`, `0x05`, `0x0A`, `0x0E`, `0x11`, `0x12`, `0x13`, `0x14`, `0x15` |
| scaled 200 | `0x0C`, `0x0D`, `0x0F`, `0x10`, `0x27`, `0x28`, `0x29`, `0x2A`, `0x43` |
| scaled 300 | `0x01`, `0x09`, `0x1A`, `0x41`, `0x42`, `0x44`, `0x45`, `0x46`, `0x49`–`0x4E` |
| scaled 400 (default) | `0x16`, `0x1D`, `0x23`, `0x2F`, `0x31`, `0x33`, `0x34`, `0x35`, `0x38`, `0x4F`–`0x56`, `0x58`, `0x5A`, `0x5C`, `0x61`, `0x63`, `0x64`, `0x66`, `0x67`, and every code above `0x69` |
| scaled 500 | `0x26`, `0x2C`, `0x5E` |
| scaled 600 or 1200 | `0x25`, `0x2B`, `0x59`: scaled 600 when the second-slot subtype is `0x0C`, otherwise scaled 1200. The slot kind is not read. |
| scaled 800 | `0x1E`, `0x20`, `0x39`, `0x3A`, `0x5D` |
| scaled 1000 | `0x5B`, `0x5F` |
| scaled 1200 | `0x2D`, `0x2E` |
| scaled 1600 | `0x1F`, `0x21`, `0x22` |
| scaled 5000 | `0x57` |

Row counts over `0x00`–`0x69`: 12 + 2 + 2 + 1 + 1 + 10 + 11 + 9 + 14 + 25 + 3 + 3 + 5 + 2 + 2 + 3 + 1 = 106.

## 4. C: `0x047310` (server `0x011D60`)

**Interface.** As for A. Codes above `0x68` take the default. Index table `0x047628` (105 entries), target table `0x0475A8`. Fields read: action byte (negative argument only), speed scale, Q's inputs. No writes, no draws.

| C | Codes |
|---|---|
| fixed 0 | `0x00`, `0x31` |
| fixed 100 | `0x36` |
| fixed 200 | `0x0A` |
| fixed 500 | `0x32`, `0x4C`, `0x4D`, `0x4E`, `0x60` |
| fixed 600 | `0x1E`, `0x20`, `0x49`, `0x5D` |
| fixed 1000 | `0x44`, `0x45` |
| fixed 1200 | `0x1F`, `0x21` |
| fixed 2000 | `0x5F` |
| fixed 3000 | `0x47` |
| fixed 5000 | `0x48`, `0x56` |
| fixed 6000 | `0x5B` |
| scaled 50 | `0x16`, `0x17` |
| scaled 100 | `0x03`, `0x04`, `0x25`, `0x2B`, `0x3E` |
| scaled 150 | `0x06`, `0x07`, `0x12`, `0x13` |
| scaled 200 | `0x01`, `0x02`, `0x09`, `0x0D`, `0x0E`, `0x0F`, `0x43` |
| scaled 400 | `0x05`, `0x0C`, `0x10`, `0x11`, `0x14`, `0x15`, `0x41`, `0x42` |
| scaled 500 | `0x2D`, `0x2E`, `0x37`, `0x5E` |
| scaled 1200 | `0x1A`, `0x22` |
| scaled 300 (default) | `0x08`, `0x0B`, `0x18`, `0x19`, `0x1B`, `0x1C`, `0x1D`, `0x23`, `0x24`, `0x26`–`0x2A`, `0x2C`, `0x2F`, `0x30`, `0x33`, `0x34`, `0x35`, `0x38`–`0x3D`, `0x3F`, `0x40`, `0x46`, `0x4A`, `0x4B`, `0x4F`–`0x55`, `0x57`, `0x58`, `0x59`, `0x5A`, `0x5C`, `0x61`–`0x68`, and every code above `0x68` |

Row counts over `0x00`–`0x68`: 2 + 1 + 1 + 5 + 4 + 2 + 2 + 1 + 1 + 2 + 1 + 2 + 5 + 4 + 7 + 8 + 4 + 2 + 51 = 105.

## 5. R0: `0x03D1A0` (server `0x0084B0`)

**Interface.** Receiver: subject. No stack argument (plain return). Result: signed 32-bit. It always uses the action byte. No writes, no draws.

It evaluates three terms, in this order, each by its own inline copy of a table:

1. the B term: the Section 3 table, including its conditional row (index table `0x03DA54`, targets `0x03D9C8`, limit `0x69`);
2. the C term: the Section 4 table (index `0x03DB40`, targets `0x03DAC0`, limit `0x68`);
3. the A term: the Section 2 table (index `0x03DBEC`, targets `0x03DBAC`, limit `0x68`).

The result is (A term + C term) + B term, in wrapping 32-bit arithmetic. Each scaled term calls Q once, so one evaluation makes between zero and three Q calls. Each copy was decoded and found to match its standalone table in every code and value.

Consequence: R0 for the current action equals A(−1) + B(−1) + C(−1) up to wrapping.

## 6. Selectors and the cost query

### 6.1 Predicate `0x044230` (server `0x00F2B0`)

Receiver: subject. Result: low byte 1 or 0. True exactly when the second-slot kind is `3` and the second-slot subtype is one of `0x05`, `0x06`, `0x07`, `0x08`, `0x0A`, `0x0B`, `0x0F`, `0x10`, `0x11`, `0x12`. No other reads, no writes, no draws.

### 6.2 Melee chooser `0x045020` (server `0x010010`)

Receiver: subject. No stack argument. Result: a 32-bit action code in `EAX` (always below `0x100`). Reads: class byte, sub-class byte, type value, both slots' kind and subtype, the flag byte, the action, the clock, and R0's inputs whenever a combo rule evaluates R0. No writes, no draws.

The first matching row decides:

| Order | Condition | Result |
|---|---|---|
| 1 | P true and second-slot subtype = `0x0B` | `0x2C` if sub-class = 1, else `0x26` |
| 2 | P true and second-slot subtype = `0x0A` | `0x20` if sub-class = 1, else `0x1E` |
| 3 | P true and sub-class = 1 | combo (`0x2A` → `0x29`, `0x2A`) |
| 4 | P true | combo (`0x28` → `0x27`, `0x28`) |
| 5 | type value = `0x68` | `0x16` |
| 6 | second-slot kind = 3 and subtype `6` or `7` | `0x16` |
| 7 | first-slot kind = 3 and subtype `6` or `7` | `0x16` |
| 8 | second-slot subtype = `3` (kind not read) | combo (`0x13` → `0x12`, `0x13`) |
| 9 | second-slot subtype = `4`, or second-slot kind = `0` | combo (`0x06` → `0x07`, `0x06`) |
| 10 | second-slot subtype = `5` | combo (`0x0E` → `0x0D`, `0x0E`) |
| 11 | second-slot kind = 3 and subtype = `8` | `0x1A` |
| 12 | predicate `0x044230` true | see below |
| 13 | first-slot kind = 3 and subtype = `0x0D` | combo (`0x0A` → `0x09`, `0x0A`) |
| 14 | bit `0x10` of the flag byte set | `0x4B` |
| 15 | otherwise | combo (`0x01` → `0x02`, `0x01`) |

Rows 1–4 read the second-slot subtype without testing its kind. In row 9 the subtype test comes first. Row 12, after the earlier rows have failed, can only be reached with second-slot kind 3 and subtype `0x0F`, `0x10`, `0x11` or `0x12`. It yields `0x43` when the action is `0x39` and the clock is strictly less than R0 + 200. Otherwise it yields `0x3A` when the action is `0x43` and the clock is strictly less than R0 + 200. In every other case it yields `0x39`. R0 is evaluated once for each of those two action tests that applies.

Possible results: `0x01`, `0x02`, `0x06`, `0x07`, `0x09`, `0x0A`, `0x0D`, `0x0E`, `0x12`, `0x13`, `0x16`, `0x1A`, `0x1E`, `0x20`, `0x26`, `0x27`, `0x28`, `0x29`, `0x2A`, `0x2C`, `0x39`, `0x3A`, `0x43`, `0x4B`.

### 6.3 Special chooser `0x0452A0` (server `0x010290`)

Receiver: subject. No stack argument. Result: a 32-bit action code. Reads: both second-slot bytes, class byte, sub-class byte, type value, Counter70. No writes, no draws, no nested calls.

| Order | Condition | Result |
|---|---|---|
| 1 | second-slot kind = 3 and subtype = `6` | `0x17` |
| 2 | P true and second-slot subtype = `0x0B` | `0x2D` if sub-class = 1, else `0x2E` |
| 3 | P true and second-slot subtype = `0x0A` | `0x21` if sub-class = 1, else `0x1F` |
| 4 | P true | `0x2B` if sub-class = 1, else `0x25` |
| 5 | second-slot kind = 3 and subtype = `5` | `0x05` |
| 6 | second-slot kind = 3 and subtype in {`0x07`, `0x08`, `0x0F`, `0x10`, `0x11`, `0x12`} | `0x41` if Counter70 is odd, else `0x42` |
| 7 | otherwise | `0x04` if Counter70 is odd, else `0x03` |

The routine's own list in row 6 also names `0x05`, `0x06`, `0x0A` and `0x0B`, but earlier rows always catch those, so they cannot reach it. Possible results: `0x03`, `0x04`, `0x05`, `0x17`, `0x1F`, `0x21`, `0x25`, `0x2B`, `0x2D`, `0x2E`, `0x41`, `0x42`.

### 6.4 Cost query `0x044AE0` (server `0x00FB20`)

Receiver: subject. Two stack arguments, removed by the routine: a 32-bit action code c, then a signed 32-bit value k. Result: binary32 on the x87 stack, always loaded from a binary32 memory value or zero. The code minus 3 is compared unsigned with `0x5C`. Index table `0x044B90` (93 entries), target table `0x044B7C` (5 targets). No writes, no draws.

| Codes | Cost |
|---|---|
| `0x03`, `0x04` | 0.1 (`0x3DCCCCCD`, client `0x2FCB7C`) |
| `0x1F`, `0x21`, `0x25`, `0x2B`, `0x2D`, `0x2E`, `0x5F` | 0.0 when a kind-9 element is present in the effect list, otherwise 0.3 (`0x3E99999A`, `0x2FCDC0`) |
| `0x22` | (1.0 − (v × 0.75)) × 0.125, where v is the action-specific value `0x03ED60` with code `0x22` and k passed unchanged; 0.75 = `0x3F400000` (`0x2FCDC4`), 1.0 = `0x3F800000` (`0x345DC0`), 0.125 = `0x3E000000` (`0x2FD8D4`). The product, the difference and the second product are each rounded. |
| every other code, including below `0x03` and above `0x5F` | +0.0 |

Only the `0x22` row reads k or calls a nested routine. The internals of `0x03ED60` are described in `alpha-combat-stat-queries.md` and were not re-audited here; its transitive call set reaches no draw (Section 10).

## 7. Starter `0x045410` (server `0x010400`)

**Interface.** Receiver: subject. No stack argument. Result: only the low byte (`AL`) is defined: 0 or 1. The upper bytes of `EAX` are whatever the last computation left.

**Decision, in order:**

1. If the lockout is nonzero, return 0. Nothing else is read or written.
2. If the action is `0x30`, return 0.
3. A threshold T is set to R0.
4. If Counter70 is nonzero and the action is neither `0x1E` nor `0x20`, T becomes T minus the A-table value for the current action (inline copy at index table `0x045630`, targets `0x0455F0`, limit `0x68`, default scaled 500), in wrapping 32-bit arithmetic. Otherwise T stays R0 and no A value is computed.
5. If the clock is greater than or equal to T (signed), the melee chooser (6.2) is evaluated with the current action and clock. Its low byte is stored as the new action, and then the clock is set to 0. If the clock is below T, nothing is written.
6. Return 1.

**Writes:** the action byte and the clock, only in step 5. No other field is written. In particular the lockout, Counter70 and mana are not changed.

**Nested calls:** R0 `0x03D1A0` (once), Q (from the inline A table, at most once), the melee chooser `0x045020` (at most once; it can evaluate R0 one or two more times and calls `0x044230`). No draws.

So step 4 makes T the sum of the C and B terms of the current action (Section 5, up to wrapping) when Counter70 is nonzero and the action is not `0x1E` or `0x20`.

## 8. Starter `0x0456A0` (server `0x010690`)

**Interface.** Receiver: subject. No stack argument. Result: low byte 0 or 1 (upper bytes undefined).

**Decision, in order:**

1. T is set to R0.
2. If Counter70 is nonzero, T becomes T minus the A-table value for the current action (inline copy at index `0x045DAC`, targets `0x045D6C`, limit `0x68`). Unlike `0x045410`, no action is excluded.
3. X is the special chooser's result (6.3), evaluated once.
4. An exemption E is true when the class byte is `4`, the sub-class byte is `1` and X is `0x11`, `0x05` or `0x14` (full 32-bit comparison). Of these three codes, only `0x05` can be produced by the chooser.
5. If the clock is less than T (signed), return 0 with no write.
6. If the lockout is nonzero and E is false, return 0 with no write.
7. **Mana test.** The cost query (6.4) is evaluated with code X and k = −1, and its result is stored as binary32. If that cost is strictly greater than the mana (ordered comparison; an unordered comparison does not fail the test), return 0 with no write.
8. The special chooser is evaluated a second time. Its low byte is stored as the action, and the clock is set to 0. Nothing has been written before this point and the chooser has no side effects, so the second result equals X.
9. **Clock preset.** If the new action is `0x25`, `0x2E`, `0x5F`, `0x1F` or `0x21`, the effect list is searched for a kind-9 element. If one is present, the clock is set to the B-table value of the new action. This is an inline copy of the Section 3 table, including the subtype-`0x0C` row, at index `0x045EA4` and targets `0x045E18`, limit `0x69`. Otherwise the clock stays 0.
10. If E is false, the lockout is set to 0. If E is true, the lockout is left unchanged.
11. Return 1.

**Reachable outcomes.** The chooser never produces `0x5F`, so the clock preset is reachable only for `0x25` (scaled 600 when the second-slot subtype is `0x0C`, otherwise scaled 1200), `0x2E` (scaled 1200) and `0x1F` or `0x21` (scaled 1600). The mana cost of each possible X is:

| X | Cost |
|---|---|
| `0x03`, `0x04` | 0.1 |
| `0x1F`, `0x21`, `0x25`, `0x2B`, `0x2D`, `0x2E` | 0.0 when a kind-9 element is present, otherwise 0.3 |
| `0x05`, `0x17`, `0x41`, `0x42` | 0.0 |

So a kind-9 element both removes the cost of the six ranged codes and, for four of them, presets the clock.

**Fields read:** clock, Counter70, action, lockout, mana, class byte, sub-class byte, type value, second-slot kind and subtype, the effect list, the speed scale and Q's inputs. **Writes:** action, clock (once or twice) and lockout, only on the success path (steps 8–10). Mana is compared but not reduced. **Nested calls:** R0 (once), Q (A table, B preset), `0x0452A0` (twice), `0x044AE0` (once, only when steps 5 and 6 pass). No draws.

The charge check of the held combat contract (availability predicate `0x03E5A0`) is not called by either starter. The only resource test in these starters is the mana comparison in step 7.

## 9. Call sites

### 9.1 Inside the combat behaviour routine `0x02CB20` (server `0x002F40`)

Each server site lies at client site − `0x29BE0`, and its rel32 target was decoded to the server counterpart.

| Helper | Client sites | Use (classes as in the held combat contract) |
|---|---|---|
| A `0x044270` | `0x02D241` | busy indicator, Section 2 |
| B `0x03CAA0` | `0x02D251` | busy indicator |
| | `0x02D404` | reach exception for actions `0x56`/`0x57`/`0x59` |
| | `0x02DDDA` | class (e), clock-minus-step test |
| | `0x02E5FB` | class (i) fallback, action `0x3C` with clock = B |
| | `0x02E6DE` | class (i) fallback, action `0x0B` with clock = B |
| C `0x047310` | `0x02D25C` | busy indicator |
| R0 `0x03D1A0` | `0x02CBFC` | the routine's prefix, before the continuation entry |
| | `0x02D628` | special-state branch |
| | `0x02D88B`, `0x02D96D`, `0x02DAE7`, `0x02E9F7` | classes (a), (b), (d), (c) |
| | `0x02E06F` | class (f), middle band |
| | `0x02E28E`, `0x02E370`, `0x02E581` | class (i): step 1, evasion gate, fallback |
| | `0x02E7A8`, `0x02E8FA` | class (h): evasion gate, fallback |
| `0x045410` | `0x02DDBE` | class (e), even parity or mana at most 0.5 |
| | `0x02E2D5` | class (i) step 2 (flag bit `0x08`), which is also the target of the jumps at `0x02E56C` (class (i) fallback) and `0x02E8E5` (class (h) fallback) |
| `0x0456A0` | `0x02DDB5` | class (e), odd parity |
| | `0x02E725` | class (i), last case when `+0xAA8` is zero and the charge result is unavailable |

The routine ignores the results of both starters.

### 9.2 Other callers (byte scan)

The first table counts client hits. "Unlisted" marks hits whose bytes the listing does not decode as instructions.

| Helper | Count | Callers (client) and generic role |
|---|---|---|
| A | 5 | `0x02D241` (above); unattributed listing region `0x014000–0x01EFFF`: `0x015B01` (a binary32 ratio of (a counter − B − C) over A), `0x01C295` (a counter compared with B + C + A), `0x01CD10` (after the same B, C, A call sequence); `0x022D4B` in `0x0128F0` (the same C + A/2 + B sum as the busy indicator). All client-only; their gameplay roles are not established. |
| `0x045410` | 3 | `0x02DDBE`, `0x02E2D5` (above); `0x09BF82` in client routine `0x088EE0`, applied to the creature that routine references at its object `+0x8006D0`, after testing that this creature's action is not `0x30`. Client-only. |
| `0x0456A0` | 3 | `0x02DDB5`, `0x02E725` (above); `0x09B6EF` (unlisted, decoded by hand as a load of the creature at `+0x8006D0` followed by the call), inside a jump-table case of `0x088EE0`. Client-only. |
| `0x045020` | 2 | `0x0455D2` in `0x045410`; `0x08BA17` in `0x088EE0` (stores the result as a list element beside the constants `0x26` and `0x2E`). |
| `0x0452A0` | 2 | `0x045832`, `0x045899`, both in `0x0456A0`. |
| `0x044230` | 10 | `0x045131` in `0x045020`; `0x03840C` in `0x034E30`; `0x08BACE` in `0x088EE0`; `0x197DE5` in `0x196D30`; four in the region `0x014000–0x01EFFF` (`0x0174D8`, `0x017779`, `0x0178D3`, `0x017C04`); two in the unattributed region `0x211000–0x217FFF` (`0x2123FF`, `0x2129F5`). Roles not established. |
| B | 238 | combat 5; `0x0128F0` 8; unattributed region `0x014000–0x01EFFF` 98; `0x088EE0` 7 (4 listed + 3 unlisted); `0x0AC260` 9; routine group `0x20C510` 19; unattributed region `0x211000–0x217FFF` 92 (81 listed + 11 unlisted). Total 5 + 8 + 98 + 7 + 9 + 19 + 92 = 238. Apart from the combat sites, timing comparisons whose roles are not established. |
| C | 80 | combat 1; `0x0128F0` 3; region `0x014000–0x01EFFF` 43; `0x078800` 1; `0x20C510` group 7; region `0x211000–0x217FFF` 25. Total 1 + 3 + 43 + 1 + 7 + 25 = 80. |
| R0 | 103 | combat 12; `0x0128F0` 3; region `0x014000–0x01EFFF` 42; `0x03E350` 1; `0x045020` 9; `0x045410` 1; `0x0456A0` 1; `0x088EE0` 7 (4 listed + 3 unlisted); `0x0AC260` 12; `0x20C510` group 12; region `0x211000–0x217FFF` 3. Total 12 + 3 + 42 + 1 + 9 + 1 + 1 + 7 + 12 + 12 + 3 = 103. |

The counts for A (1 + 3 + 1 = 5) and `0x044230` (1 + 1 + 1 + 1 + 4 + 2 = 10) match their rows.

Server scan (targets as in Section 11):

| Helper | Count | Server callers |
|---|---|---|
| A `0x00F2F0` | 1 | `0x003661` (combat) only |
| `0x010400` | 2 | `0x0041DE`, `0x0046F5` (combat) only |
| `0x010690` | 2 | `0x0041D5`, `0x004B45` (combat) only |
| `0x010010` | 1 | `0x0105C2` in `0x010400` |
| `0x010290` | 2 | `0x010822`, `0x010889` in `0x010690` |
| `0x00F2B0` | 4 | `0x010121` in `0x010010`; `0x0D0E05` in `0x0CFD50`; `0x1381BF`, `0x1387B5` (unattributed) |
| B `0x007DB0` | 116 | combat 5; routine group `0x1322D0` 19; region `0x137000–0x13DFFF` 92 (81 listed + 11 unlisted); total 116 |
| C `0x011D60` | 33 | combat 1; `0x1322D0` group 7; region `0x137000–0x13DFFF` 25; total 33 |
| R0 `0x0084B0` | 38 | combat 12; `0x010010` 9; `0x010400` 1; `0x010690` 1; `0x1322D0` group 12; region `0x137000–0x13DFFF` 3; total 38 |

The server's `0x1322D0` group and its unattributed neighbours lie at the client's `0x20C510` group and `0x211000–0x217FFF` region − `0xDA240`; the server's `0x0CFD50` site lies at the client's `0x196D30` site − `0xC6FE0`. Those correspondences were matched by offset only and not compared further. The client-only callers (`0x0128F0` and its region, `0x088EE0`, `0x0AC260`, `0x034E30`, `0x03E350`, `0x078800`) have no server hits. So on the server the two starters are reached only from the combat routine.

## 10. Random draws

None of the routines in this contract calls `rand` or `srand`, directly or transitively. A transitive walk of `calls.csv` from each of the six helpers reached only the following routines. Client: `0x03D1A0`, `0x044230`, `0x045020`, `0x0452A0`, `0x044AE0`, `0x03ED60`, `0x03C9C0`, Q `0x047700` and its callees `0x0478B0`, `0x044CB0`, `0x045FF0`, `0x0C7C00`, plus the pow import. Server: the counterparts, plus the pow import. No computed call was reached. The listings show the same: every call instruction in the helpers is a direct call to those routines.

## 11. Client/server agreement

Counterparts were found from the server combat routine's call sites (at the constant offset) and from the callees of the paired starters:

| Routine | Client | Server | Tables (client → server) |
|---|---|---|---|
| A | `0x044270` | `0x00F2F0` | `0x04442C`/`0x0443EC` → `0x00F4AC`/`0x00F46C` |
| B | `0x03CAA0` | `0x007DB0` | `0x03CEB0`/`0x03CE24` → `0x0081C0`/`0x008134` |
| C | `0x047310` | `0x011D60` | `0x047628`/`0x0475A8` → `0x012078`/`0x011FF8` |
| R0 | `0x03D1A0` | `0x0084B0` | B `0x03DA54`/`0x03D9C8` → `0x008D64`/`0x008CD8`; C `0x03DB40`/`0x03DAC0` → `0x008E50`/`0x008DD0`; A `0x03DBEC`/`0x03DBAC` → `0x008EFC`/`0x008EBC` |
| Starter | `0x045410` | `0x010400` | `0x045630`/`0x0455F0` → `0x010620`/`0x0105E0` |
| Starter | `0x0456A0` | `0x010690` | `0x045DAC`/`0x045D6C` → `0x010D9C`/`0x010D5C`; `0x045EA4`/`0x045E18` → `0x010E94`/`0x010E08` |
| Melee chooser | `0x045020` | `0x010010` | none |
| Special chooser | `0x0452A0` | `0x010290` | none |
| Predicate | `0x044230` | `0x00F2B0` | none |
| Cost | `0x044AE0` | `0x00FB20` | `0x044B90`/`0x044B7C` → `0x00FBD0`/`0x00FBBC` |
| Q | `0x047700` | `0x012150` | not compared here |

Each pair was compared instruction by instruction over its whole range, with absolute addresses masked. Listed instruction counts: A 92, B 193, C 162, R0 479, `0x045410` 120, `0x0456A0` 405, `0x045020` 221, `0x0452A0` 93, `0x044230` 27, cost 44. There were no differences. Every jump table was also decoded separately in each image, code by code, into its form and value, with the server's binary32 constants read from `Server.exe`. The decoded tables are identical. The cost query's server constants (`0x1586BC` 0.1, `0x1586CC` 0.3, `0x1586D4` 0.75, `0x1586DC` 1.0, `0x1588B4` 0.125) have the client bit patterns. Inside the combat routine, every helper call site has a server counterpart at the constant offset.

On this evidence the helpers behave identically in both images. Their caller sets differ: the client also uses them in client-only routines (Section 9.2).

## 12. Unresolved

- The formula of the attack-speed query Q and of its callees. They are bounded here (pure, binary32, no draws) and covered by other contracts, but were not re-audited for this one.
- The internals of the action-specific value `0x03ED60` used by the cost of code `0x22`, which cannot be reached from these starters.
- The gameplay roles of the client-only callers in Section 9.2 and of the large unattributed listing regions. Byte-scan hits the listing does not decode were not all decoded by hand; only `0x09B6EF` and `0x09B705` were.
- What produces or consumes Counter70, the lockout, the kind-9 effect element, the class, sub-class and type values, and the slot kind and subtype numbers. The meaning of every action code is also not established.
- Behaviour when Q × speed scale is zero, negative, infinite or NaN, beyond the truncation rule in Section 1; and wrapping overflow in the sums.
