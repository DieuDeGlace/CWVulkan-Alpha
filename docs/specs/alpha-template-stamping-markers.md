Status: FINAL / STABLE / HELD

# Original template stamping: traversal, destination tests, the full marker chain and ordinary cells

## 0. Evidence and scope

The supported originals are the 32-bit x86 images `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, both with preferred base `0x00400000`. Both hashes were recomputed before use and matched. Every address below is an RVA in the named image, written "client X / server Y". Evidence paths are relative to the evidence root `Documents/disassembled alpha cube world`.

The routine under contract is client `0x1F0CE0` / server `0x124540`. Its instructions were read from `decompiled/client/assembly.txt` (VA `005f0ce0`–`005f4ae8`) and `decompiled/server/assembly.txt` (VA `00524540`–`00528348`). Jump tables, floating-point constants and static data were decoded from the image bytes. The earlier contract `docs/specs/alpha-class-83-descriptor-production.md` was used only to locate this routine. Everything stated here was re-read from the originals. The permitted reconstruction reference was **not** consulted, so nothing here comes from it.

Client/server agreement was established mechanically (section 11). The two instruction streams were aligned with every absolute address masked out, and all other operands, including immediates, frame offsets and field offsets, had to match exactly. All 32 nested helpers compared in section 11 were checked the same way.

Terms used:

- **Template**: the first argument's object. Its extents are signed 32-bit integers at `+0x44` (X), `+0x48` (Y) and `+0x4C` (Z). The pointer at `+0x30` points to its cells, which are three-byte triples. The triple for template coordinate (tx, ty, tz) is at byte offset 3 × ((tz × extY + ty) × extX + tx). Whenever a coordinate is negative or not below its extent, the routine reads a static all-zero triple instead (client `0x36B340` / server `0x183DFC`, zero at load and never written in either image; checked by scanning the image bytes).
- **Empty triple**: (0, 0, 0).
- **World cell**: four bytes, written here as (b0, b1, b2, b3). b0–b2 are colour bytes. In b3, bits 0–4 (mask `0x1F`) are the **cell type**, `0x40` is a flag tested by this routine, and `0x80` is maintained by the cell writer (section 9.4).
- **Fixed position**: a world coordinate held as a signed 64-bit integer equal to the integer coordinate shifted left by 16 bits (sign-extended), so one world unit is 65536. Offsets are quoted as those integers, with the unit value in brackets.
- **Draw**: one call through the process-wide random import (client import slot `0x2FC468` / server `0x1582F4`). "rem n" means the 32-bit signed remainder of the draw by n, computed by sign-extending division, so its sign follows the draw.

## 1. Interface

The calling convention is a receiver in ECX plus nine 32-bit stack arguments, released by the callee (`RET 0x24`).

| Stack arg | Width / signedness as used | Use observed in this routine |
| --- | --- | --- |
| receiver (ECX) | pointer | World object. It is passed as receiver to the column lookup, the column-value fallback, the cell getter, the world cell writer and the level service. |
| 1 | pointer | Template (layout above). |
| 2 | pointer to three signed 32-bit ints | Base world cell position (bx, by, bz), read once at entry. |
| 3 | signed 32-bit | Orientation. The traversal uses its signed remainder modulo 4 and its signed remainder modulo 2. Angle and record fields use the raw unreduced value. |
| 4 | low 8 bits used | Type byte for ordinary cells. Only its low byte is read, and it is ORed with `0x40` (section 7). |
| 5 | signed 32-bit | Type selector (sections 4, 5 and 6). |
| 6 | pointer, may be null for the early checks | Owner/zone object. If non-null, it limits the stamp to its 256×256 sector (section 2) and is passed as the sector hint to every column lookup. It owns three collections: a feature-record collection at `+0x0C` (elements 0x188 bytes, end pointer `+0x10`), an object-entry list at `+0x04` (count `+0x08`) and a descriptor pointer collection at `+0x18`. Branches that append to these dereference it without a null test. |
| 7 | 8-bit flag | Nonzero enables the downward fill pre-pass (section 3.2). |
| 8 | pointer to four signed 32-bit ints | Trim margins. They are read only when the orientation remainder is 0–3 (section 2). |
| 9 | pointer, may be null | Change-log object. When non-null, every world cell this routine writes inside the stamping pass is also appended as a 20-byte record to the linked list at `+0x18` (count at `+0x1C`). Each record is x, y, z as 32-bit ints, the 4 cell bytes, then a 32-bit zero. |

**Return value**: none. EAX is not loaded with a result on any exit path. Whether any caller reads EAX was not examined.

## 2. Entry checks and the traversal

On entry, in order:

1. The footprint size is taken as extX by extY. If the orientation's signed remainder modulo 2 is nonzero (odd orientations, including negative odd ones), the two are swapped.
2. If argument 6 is non-null, let sx = 256 × its 32-bit `+0x60` and sy = 256 × its 32-bit `+0x64`. The routine returns immediately, with no output, if any of these holds, tested in this order with signed comparisons: bx + footprintX ≤ sx; by + footprintY ≤ sy; bx ≥ sx + 256; by ≥ sy + 256.
3. A **column value** V (binary32) is obtained once. It is the binary32 at `+0x08` of the column record that the column lookup returns for (bx, by) with argument 6 as hint. If there is no such record, V is the binary32 result of a receiver service called with (bx, by) (client `0x1C4DD0` / server `0x0F8B40`; not audited). V is used only by markers M3 and M10.
4. Template X and Y ranges are set from argument 8's four values (m0, m1, m2, m3) according to the orientation remainder r. "Start" is inclusive and "end" exclusive:

| r | X start | Y start | X end | Y end |
| --- | --- | --- | --- | --- |
| 0 | m0 | m1 | extX − m2 | extY − m3 |
| 1 | m3 | m0 | extX − m1 | extY − m2 |
| 2 | m2 | m3 | extX − m0 | extY − m1 |
| 3 | m1 | m2 | extX − m3 | extY − m0 |
| negative | 0 | 0 | extX | extY |

   The table selecting these rows is client `0x1F4AF0` / server `0x128350` (entries in the order 0, 1, 2, 3).
5. If X start ≥ X end (signed), the routine returns.

The traversal is three nested loops, in this order:

- **outer**: tx ascends from X start while tx < X end;
- **middle**: ty ascends from Y start while ty < Y end;
- **inner**, for each (tx, ty):
  1. a column scan (section 3.1), with tz ascending from 0 to extZ − 1;
  2. the optional pre-pass (section 3.2);
  3. the stamping pass, with tz **descending** from extZ − 1 to 0.

### 2.1 Template-to-world mapping

For a template cell (tx, ty, tz), the destination world cell (wx, wy, wz) is:

| r | wx | wy | wz |
| --- | --- | --- | --- |
| 0 | bx + tx | by + ty | bz + tz |
| 1 | bx + ty | by + (extX − 1 − tx) | bz + tz |
| 2 | bx + (extX − 1 − tx) | by + (extY − 1 − ty) | bz + tz |
| 3 | bx + (extY − 1 − ty) | by + tx | bz + tz |
| negative | not assigned | not assigned | not assigned |

All arithmetic is 32-bit signed and wraps. The stamping-pass mapping uses its own table (client `0x1F4B10` / server `0x128370`), and the pre-pass uses an identical one for (wx, wy) (client `0x1F4B00` / server `0x128360`). For a negative orientation remainder, the stamping pass jumps past the assignment, so the destination keeps whatever the position locals last held (uninitialised on the first cell). Whether any caller passes a negative orientation is unresolved.

The fixed position used for records is P = (wx, wy, wz), each shifted left by 16 bits.

## 3. Column scan, pre-pass and the per-cell tests before the marker chain

### 3.1 Column scan

For the current (tx, ty), zmin starts at extZ and zmax at 0. For every tz from 0 to extZ − 1 whose triple is not empty, zmin = min(zmin, tz) and zmax = max(zmax, tz). A tz is **interior** when zmin < tz < zmax, both strict and signed. Many markers write the world only for interior cells.

### 3.2 Pre-pass (argument 7 nonzero)

Performed once per (tx, ty), before the stamping pass:

1. If the template triple at (tx, ty, 0) is empty, the pre-pass does nothing.
2. Otherwise (wx, wy) are computed by the mapping above. If r is negative, the pre-pass does nothing. The column for (wx, wy) is looked up with argument 6 as hint. If there is none, the pre-pass does nothing.
3. Let k start at bz − 1. While k > the column's 32-bit `+0x10` (its lowest stored z, read once before the loop):
   - read the destination cell at (wx, wy, k) with the substitution rules of section 9.2;
   - if its type is neither 0 nor 2, stop;
   - otherwise write the world cell (t0, t1, t2, 6), where t0–t2 are the template triple at (tx, ty, 0), through the world cell writer;
   - decrement k.

There are no draws, and nothing is appended to argument 9's list.

### 3.3 Stamping-pass tests, in order, for one tz

1. Compute the destination by 2.1 and read the template triple T (or the zero triple when out of range).
2. **Empty T**: if tz is interior, append the record (wx, wy, wz, (255, 255, 255, `0xC0`), 0) to argument 9's list when argument 9 is non-null, and write the world cell (255, 255, 255, `0xC0`). In either case, go to the next tz. No further tests apply.
3. **Destination protection**: read the destination cell D (section 9.2). If D's b3 has bit `0x40` set and D's type is neither 0 nor 2, skip this cell entirely. The type is read by an identical second lookup.
4. **Selector admission**: if the selector equals one of 1, 2, 6, 3, 4, 5, 7, 9, 10, 11, 12, 13, 14, 15 (tested in that order), go to the marker chain (section 4). Any other value goes to section 7.

## 4. The marker chain

T is compared byte by byte against each marker in this order. Triples are in stored byte order (byte 0, byte 1, byte 2), and the first match decides. The comparisons themselves do not depend on the selector, but most branches do.

| Order | Marker | Client / server compare |
| --- | --- | --- |
| M1 | (255, 0, 0) | `0x1F1731` / `0x124F91` |
| M2 | (0, 255, 0) | `0x1F1CB1` / `0x125511` |
| M3 | (0, 127, 127) | `0x1F1EC1` / `0x125721` |
| M4 | (255, 127, 0) | `0x1F2181` / `0x1259E1` |
| M5 | (255, 127, 127) | `0x1F2581` / `0x125DE1` |
| M6 | (127, 127, 0) | `0x1F27E8` / `0x126048` |
| M7 | (255, 255, 0) | `0x1F2BF8` / `0x126458` |
| M8 | (63, 0, 0) | `0x1F2E01` / `0x126661` |
| M9 | (0, 0, 255) | `0x1F2F8B` / `0x1267EB` |
| M10 | (255, 0, 255) | `0x1F326F` / `0x126ACF` |
| M11 | (0, 255, 255) | `0x1F33E5` / `0x126C45` |
| M12 | (0, 0, 127) | `0x1F39F2` / `0x127252` |
| M13 | (0, 255, 127) | `0x1F3B68` / `0x1273C8` |
| M14 | (0, 127, 0) | `0x1F3FBF` / `0x12781F` |
| M15 | (127, 0, 127) | `0x1F4182` / `0x1279E2` |
| none | — | ordinary cell (section 7) |

M1–M9 compare against triples built in locals. M10–M15 use the triple-compare helper (client `0x0E6BD0` / server `0x02EDB0`), which returns true only when all three bytes are equal.

### 4.1 Products

Markers produce four kinds of output. Their field layouts are given once here:

- **Feature record** (0x188 bytes, appended by copy to argument 6's `+0x0C` collection through client `0x1A0E00` / server `0x0D6670`, or by the inline equivalent in M1). The record constructor (client `0x18EAA0` / server `0x0C84B0`) sets byte `+0x30` = 1, 32-bit `+0x34`, `+0x38`, `+0x40`, `+0x44` = 0, `+0x178`, `+0x17C`, `+0x180` = −1 and `+0x184` = 0, and initialises a sub-object at `+0x48`. The routine then writes: `+0x00` kind (32-bit); `+0x08`, `+0x10`, `+0x18` fixed position x, y, z (64-bit each); `+0x20` a 32-bit integer; `+0x24`, `+0x28`, `+0x2C` binary32 sizes.
- **Footprint feature record**: a feature record whose `+0x08`–`+0x30` are filled by the footprint service (section 9.5). The service returns false when the cell is not the lower corner of its box; in that case the record is not appended and nothing that depends on it happens.
- **Object entry** (appended to argument 6's `+0x04` list; client `0x0862E0` / server `0x128450` or inline). The payload copied into the list is `+0x00` kind (32-bit); `+0x08`, `+0x10`, `+0x18` fixed position (64-bit); `+0x20` binary32; `+0x24` binary32 angle; `+0x28` binary32; `+0x2C`, `+0x30`, `+0x34` binary32 scale; `+0x38` 32-bit integer. `+0x04` and `+0x3C` are not copied. This routine never writes `+0x28`, so it is copied from an uninitialised local (unresolved value).
- **Descriptor** (0x10F0 bytes, constructed by client `0x1ADB20` / server `0x0E0F40`, pointer appended to argument 6's `+0x18` collection through client `0x26ADD0` / server `0x0F2BE0`).

**Clear write**: wherever a branch says "clear write", it does two things. If argument 9 is non-null, it appends (wx, wy, wz, (0, 0, 0, `0x40`), 0) to argument 9's list. It then writes the world cell (0, 0, 0, `0x40`) at (wx, wy, wz). "Interior clear write" means a clear write performed only when tz is interior; otherwise nothing is written for that cell.

Angles: "angle A" means the 32-bit signed product shown, converted to binary32. Conversion from binary64 constants to 64-bit offsets truncates toward zero (helper client `0x28D946` / server `0x14A946`).

### 4.2 Branches

**M1 (255, 0, 0)**: a fresh feature record goes through the footprint service. If the service succeeds, the record's kind is set to 2 when the selector equals 7 and to 1 otherwise, and the record is appended. Then, whether or not it succeeded, a clear write follows. No draws.

**M2 (0, 255, 0)**: as M1, but on success the kind is 3. Clear write follows unconditionally. No draws.

**M3 (0, 127, 127)**:
1. If V > 0.5 is false, skip the cell with no write. The test is an ordered comparison, so NaN also skips. The 0.5 is binary32 `0x3F000000` at client `0x345DA0` / server `0x1586D0`.
2. Read the cell at (wx, wy, wz) through the cell getter (section 9.2). If its type is neither 0 nor 2, skip the cell with no write.
3. Draw d1. If d1 rem 5 ≠ 0, go to step 5.
4. Draw d2 and append an object entry:

| Field | Value |
| --- | --- |
| kind | (d2 rem 3) + 41 |
| position | P + (32964, 32964, 196), i.e. about 0.503, 0.503 and 0.003 units, truncated from binary64 32964.608 (`0x40E0189374BC6A7F`, client `0x31DEC8` / server `0x173B60`) and 196.608 (`0x40689374BC6A7EFA`, client `0x31DDC8` / server `0x173A60`) |
| `+0x20` | 0.15 (`0x3E19999A`) |
| angle `+0x24` | orientation × (−90) |
| scale | 1.0, 1.0, 1.0 |
| `+0x38` | 0 |

5. Clear write, unconditionally.

**M4 (255, 127, 0)**: no draws. Two feature records are appended, in this order:

| Record | Kind | Position | `+0x20` | Sizes |
| --- | --- | --- | --- | --- |
| first | 19 | P + (32768, 32768, 0) | 0 | 2.0, 3.0, 1.0 |
| second | 20 | P + (147456, 32768, 0) | 0 | 1.0, 1.0, 1.0 |

The offsets 32768 [0.5] and 147456 [2.25] are truncated from binary64 `0x40E0000000000000` (client `0x302A88` / server `0x1737E8`) and `0x4102000000000000` (client `0x31DF28` / server `0x173BC0`). Then an interior clear write.

**M5 (255, 127, 127)**: one draw d. One feature record is appended:

| Field | Value |
| --- | --- |
| kind | (d rem 3) + 32 |
| position | P + (32768, 32768, 0) |
| `+0x20` | the raw orientation |
| sizes | 2.0, 3.0, 1.0 |

Then an interior clear write.

**M6 (127, 127, 0)**: three draws, in order d1, d2, d3.

| Record | Kind | Position | `+0x20` | Sizes |
| --- | --- | --- | --- | --- |
| first | 12 | P + (32768, 32768, 0) | d1 rem 4 | 3.0, 3.0, 1.0 |
| second | 16 | P + (176947, 32768, 0) | d2 rem 4 | 1.0, 1.0, 0.5 |
| third | 16 | P + (−111411, 32768, 0) | d2 rem 4 | 1.0, 1.0, 0.5 |

The first record is appended, then the second, then the third. The third is the second record's local storage with only its position replaced. 176947 [about 2.7] comes from binary64 `0x410599999999999A` (client `0x31DF30` / server `0x173BC8`), and −111411 [about −1.7] from `0xC0FB333333333333` (client `0x31DF98` / server `0x173C50`). d3 is reduced rem 4 and stored into that local only *after* the third record has been copied, so d3 reaches no output. It still consumes a draw. Then an interior clear write.

**M7 (255, 255, 0)**: no draws. One object entry is appended:

| Field | Value |
| --- | --- |
| kind | 13 |
| position | P + (32768, 32768, 0) |
| `+0x20` | 0.1 (`0x3DCCCCCD`) |
| angle | 0.0 |
| scale | 0.6 (`0x3F19999A`), 0.5, 0.4 (`0x3ECCCCCD`) |
| `+0x38` | 1 |

Then an interior clear write.

**M8 (63, 0, 0)**: no draws. One object entry is appended:

| Field | Value |
| --- | --- |
| kind | 48 |
| position | P + (32768, 32768, 32768) |
| `+0x20` | 0.065 (`0x3D851EB8`) |
| angle | orientation × (−90) |
| scale | 1.0, 1.0, 1.0 |
| `+0x38` | 0 |

Then an interior clear write.

**M9 (0, 0, 255)**: no draws. The selector is tested in this order: 2, 6, 3, 5, 4, 13, 14, 15. A match appends one object entry whose kind depends on the selector:

| Selector | 2 | 6 | 3 | 5 | 4 | 13 | 14 | 15 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| kind | 33 | 37 | 34 | 36 | 35 | 38 | 39 | 40 |

All other entry fields are shared:

| Field | Value |
| --- | --- |
| position | P + (32768, 32768, −32768). These come from binary64 0.5 (`0x3FE0000000000000`, client `0x345DE8` / server `0x1586F0`) scaled by 65536, and from binary64 −32768 (`0xC0E0000000000000`, client `0x302AD8` / server `0x1737C0`) for selector 2 or binary64 −0.5 (`0xBFE0000000000000`, client `0x345E90` / server `0x15AFA8`) scaled for the others |
| `+0x20` | 0.1 |
| angle | (−1 − orientation) × 90 |
| scale | 1.0, 1.0, 1.0 |
| `+0x38` | 0. For the non-2 selectors the entry initialiser (client `0x18E9F0` / server `0x0C83B0`) first sets `+0x38` = 2 and the scales to 1.0; the 2 is then overwritten |

Selectors 1, 7, 9, 10, 11 and 12 append nothing. In every case an interior clear write follows.

**M10 (255, 0, 255)**:
1. If V > 0.5 is false (NaN included), skip the cell with no write.
2. A fresh feature record goes through the footprint service. On success, draw d, set kind = (d rem 3) + 47 and append the record. On failure there is no draw and no append.
3. Interior clear write.

**M11 (0, 255, 255)**, by selector:

- **13, 14, 15**: no draws. One feature record is appended:

| Field | 13 | 14 | 15 |
| --- | --- | --- | --- |
| kind | 71 | 75 | 73 |
| sizes | 3.0, 3.0, 6.0 | 1.5, 1.5, 1.0 | 1.5, 1.5, 1.0 |

  All three share position P + (32768, 32768, 0) and `+0x20` = −orientation (32-bit negation of the raw value). Then an interior clear write.

- **Every other admitted selector** (1–7, 9–12): no draws. A descriptor is constructed and given:

| Field | Value |
| --- | --- |
| `+0x28` | 6 (32-bit) |
| `+0x2C` | 141 if the selector equals 10, otherwise 142 (32-bit) |
| `+0x54` | binary32 of (2 − orientation) × 90 |
| `+0x10` | fixed position P + (32768, 32768, 32768). The offsets come from binary32 0.5 multiplied by binary32 65536 (`0x47800000`, client `0x2FCE7C` / server `0x15878C`) and truncated |
| `+0x34` | the level-service result for P's first two components (section 9.6) |

  The descriptor pointer is appended to argument 6's `+0x18` collection. **No** class byte (`+0x30`), no `+0x350`/`+0x35D`, and no behaviour object are written on this path (client `0x1F3824`–`0x1F3946` / server `0x127084`–`0x1271A6`). Then a clear write, unconditionally (no interior test).

**M12 (0, 0, 127)**: if the selector is not 13, skip the cell with no write. For selector 13, one feature record is appended:

| Field | Value |
| --- | --- |
| kind | 77 |
| position | P + (32768, 32768, 0) |
| `+0x20` | −orientation |
| sizes | 1.8 (`0x3FE66666`), 1.8, 1.2 (`0x3F99999A`) |

Then a clear write, unconditionally. No draws.

**M13 (0, 255, 127)**: for selectors 13, 14 and 15, one feature record is appended:

| Field | 13 | 14 | 15 |
| --- | --- | --- | --- |
| kind | 72 | 76 | 74 |
| sizes | 1.0, 1.0, 1.0 | 1.8, 1.8, 1.2 | 1.5, 1.5, 3.0 |

All three share position P + (32768, 32768, 0) and `+0x20` = −orientation. Then an interior clear write. For any other selector the cell is skipped with no write. No draws.

**M14 (0, 127, 0)**, for every admitted selector:
1. Draw d1; kind = (d1 rem 9) + 56.
2. Position P + (32768, 32768, 0).
3. Draw d2; `+0x20` = d2 rem 4.
4. Sizes are 0.5, 0.5, 0.5 if kind < 60 (signed) and 1.0, 1.0, 1.0 otherwise.
5. Append the feature record, then an interior clear write.

**M15 (127, 0, 127)**: a descriptor is constructed. In order:

1. `+0x28` = 3.
2. Draw d0; `+0x2C` = d0 rem 4.
3. `+0x54` = binary32 of (2 − orientation) × 90.
4. `+0x10` = P + (32768, 32768, 32768).
5. 16-bit `+0x350` = 4.
6. Byte `+0x35D` = 6.
7. `+0x34` = level-service result for P's first two components.
8. Selector switch (section 5).
9. Behaviour object assembled and stored at `+0x109C` (section 9.7).
10. Descriptor pointer appended to argument 6's `+0x18` collection.
11. Interior clear write.

## 5. The M15 selector switch, in full

The switch tests (selector − 6) as unsigned against 9 and then indexes a ten-entry table (client `0x1F4B20` / server `0x128380`, decoded from the bytes of both images). Selectors 1–5 take the default (no further writes). Selector 8 has a do-nothing entry, but it cannot reach M15 because it is not admitted (section 3.3).

| Selector | Writes, in order |
| --- | --- |
| 6 | byte `+0x30` = `0x83` |
| 7 | 32-bit `+0x34` = 20; draw; `+0x2C` = (draw rem 2) + 2; byte `+0x30` = `0x89` |
| 13 | byte `+0x30` = `0x85` |
| 14 | byte `+0x30` = `0x86` |
| 15 | byte `+0x30` = `0x87` |
| 9, 10, 11, 12 | the common block below |

For selectors 9–12, five blocks spaced 0x118 bytes apart, at `+0x8C8`, `+0x698`, `+0x350`, `+0x468` and `+0x580`, are filled in that order. The block at `+0x7B0` is not touched. Each filled block receives:

- a byte at +0, sometimes with a byte at +1;
- byte +0xC = a draw rem 5;
- 16-bit +0x10 = the low 16 bits of `+0x34` (here 20);
- byte +0xD = a per-selector constant.

Common prefix, in order: 16-bit `+0x7A` = `0x40`; 32-bit `+0x34` = 20; 32-bit `+0x2C` = 2; byte `+0x30` = class; then the blocks.

| Selector | Class `+0x30` | `+0x8C8` / `+0x8C9` | +0xD of block `+0x8C8` | +0xD of blocks `+0x698`, `+0x350`, `+0x468`, `+0x580` | Draws in switch |
| --- | --- | --- | --- | --- | --- |
| 9 | 1 | 3 / (draw rem 3) + 15 | 1 | 1 | 6 |
| 10 | 2 | 3 / (draw rem 2) + 6 | 2 | 26 (`0x1A`) | 6 |
| 11 | 4 | 3 / 5 (one 16-bit store `0x0503`) | 1 | 27 (`0x1B`) | 5 |
| 12 | 3 | 3 / 10 (one 16-bit store `0x0A03`) | 2 | 25 (`0x19`) | 5 |

The +0 bytes of the other blocks are: `+0x698` = 7, `+0x350` = 4 (a byte store over the earlier 16-bit 4), `+0x468` = 6 and `+0x580` = 5. The `+0x580` value is the divisor register's low byte, which holds 5 at that point. The +0xD write of block `+0x350` (that is, `+0x35D`) overwrites the 6 stored there before the switch.

Order within each block is: +0 (and +1), draw for +0xC, +0x10, +0xD. For 9 and 10 the first draw belongs to `+0x8C9`, before the `+0x8C8` block's +0xC draw.

Total explicit draws on the M15 path: 1 (d0), plus 1 for selector 7, 6 for 9 and 10, 5 for 11 and 12, and 0 for the others.

## 6. Selector dependence summary

Selector 8, and any selector outside the admitted list (including 0, negative values and values above 15), never consults M1–M15 (section 7). Within the chain:

- M1 uses the selector only for the kind 1/2 choice;
- M9 for the entry kind;
- M11 to choose records (13–15) or the descriptor path (others, with 10 changing `+0x2C`);
- M12 and M13 to admit only 13 or 13–15;
- M15 for the switch.

M2–M8, M10 and M14 behave identically for every admitted selector.

## 7. Cells that match no marker (and non-admitted selectors)

For a non-empty T that passed the protection test:

1. Only when the selector is exactly 8, which is not admitted: if T equals (0, 0, 255), append (wx, wy, wz, (255, 255, 255, 2), 0) to argument 9's list if it is non-null, write the world cell (255, 255, 255, 2), and go to the next tz (client `0x1F493B`–`0x1F49E1` / server `0x12819B`–`0x128241`).
2. Otherwise, the **ordinary cell**: b0, b1, b2 are T's bytes 0, 1, 2 unchanged, and b3 = (argument 4's low byte) OR `0x40`. The record (wx, wy, wz, cell, 0) is appended to argument 9's list if it is non-null, and the cell is written.

An ordinary cell involves no draw, no colour arithmetic, no type arithmetic and no interior test. Admitted selectors reach step 2 directly when no marker matched.

## 8. Draw order per cell

Explicit draws occur only in these cases:

| Marker | Draws, in order |
| --- | --- |
| M3 | d1, then d2 only when d1 rem 5 = 0 |
| M5 | 1 |
| M6 | 3, the last unused |
| M10 | 1, only when the footprint service succeeds |
| M14 | 2 (kind, then `+0x20`) |
| M15 | as in section 5 |

No other path of this routine makes a draw directly. Cells are processed strictly in traversal order (section 2), so the draw sequence follows that order. Draws inside unaudited nested routines are listed in section 10.

## 9. Nested routines

Each nested routine below is stated as audited in full, audited for the effect described, or not audited.

### 9.1 Column lookup (client `0x0347A0` / server `0x006100`) — audited

Inputs are (x, y, hint). It returns null if x < 0, y < 0, x ≥ 0x1000000 or y ≥ 0x1000000 (signed).

- With a null hint, the sector is found through a further routine (client `0x034D10`, not audited) from x and y each divided by 256 with truncation.
- With a hint, it returns null unless 256 × hint`+0x60` ≤ x < that + 256 and 256 × hint`+0x64` ≤ y < that + 256.

The result is the sector's `+0xA8` pointer plus 32 × ((x signed-rem 256) + 256 × (y signed-rem 256)). In a column record, `+0x10` is the lowest stored z, `+0x1C` the stored count, `+0x18` the pointer to 4-byte cells, and `+0x08` the binary32 column value V. No draws.

### 9.2 Destination cell read — audited

Used inline by the protection test and pre-pass, and by the getter (client `0x02F7E0` / server `0x005FD0`) for M3. Rules, in order:

| Condition | Cell read |
| --- | --- |
| no column, or z < lowest stored z | (200, 200, 200, 1) |
| z ≥ lowest + count, z > 0 | (255, 255, 255, 0) |
| z ≥ lowest + count, z ≤ 0 | (255, 255, 255, `0x82`) |
| otherwise | the stored cell, except that when its type is 0, z ≤ 0 and its `0x40` bit is clear, it reads as (255, 255, 255, `0x82`) |

The index helper (client `0x02F730` / server `0x005F20`) is only reached in range. The substitute values are statics set by start-up initialisers in both images (inline reads: client `0x36DDF4`–`0x36DDFF` / server `0x1842BC`–`0x1842C7`; getter: client `0x36AFBC`–`0x36AFC7` / server `0x183D0C`–`0x183D17`). No draws.

### 9.3 Template cell read (client `0x0A2870` / server `0x030730`) — audited

Same addressing and zero-triple fallback as the inline reads.

### 9.4 World cell writer (client `0x04E7A0` / server `0x01FF00`) — audited for its direct effects

Inputs are the receiver, x, y, z, a pointer to a 4-byte cell, and the hint.

1. If there is no column, nothing is written.
2. If the cell's b3 is exactly 0 and z ≥ lowest + count, nothing is written.
3. If z ≤ 0 and (b3 AND `0x5F`) = 0, the static (255, 255, 255, `0x82`) (client `0x36B010` / server `0x183DB0`) is written instead.
4. The write goes through the element writer (client `0x04E700` / server `0x01FE60`):
   - a negative index grows the column downward and stores at the new bottom;
   - an index at or past the top grows it upward;
   - an existing element whose bit `0x80` is clear is overwritten with all four bytes;
   - one whose bit `0x80` is set is overwritten only when the new type is nonzero, and bit `0x80` is then set again.
5. If the write extended the column downward, every z between z + 1 and the previous lowest z − 1 is filled with type 6 and colours produced by a further routine (client `0x1F9620` / server `0x12D030`, not audited).

The growth routine (client `0x050C50`) was not audited.

### 9.5 Footprint service (client `0x1CB3B0` / server `0x0FF1A0`) and box finder (client `0x1EE0F0` / server `0x121ED0`) — audited

The box finder takes (template, tx, ty, tz, orientation). It returns false unless the triples at (tx − 1, ty, tz), (tx, ty − 1, tz) and (tx, ty, tz − 1) each differ from T; out-of-range neighbours read as empty.

Otherwise it extends a high corner from (tx, ty, tz), one axis at a time:

1. +X along row (ty, tz) while the triple equals T;
2. +Y along (tx, tz);
3. +Z along (tx, ty).

Each axis stops at the first mismatch. Both corners are then mapped by the orientation as in 2.1 without the base (negative remainder: unchanged), sorted component-wise so low ≤ high, and the high corner is incremented by 1 on each axis.

The service then writes into the record:

| Field | Value |
| --- | --- |
| `+0x08` | (bx << 16) + (lowX + highX) × 32768 |
| `+0x10` | (by << 16) + (lowY + highY) × 32768 |
| `+0x18` | (bz + lowZ) << 16 |
| `+0x20` | 0 if (highX − lowX) > (highY − lowY), else 1 |
| `+0x24` | the larger of the two horizontal spans, as binary32 |
| `+0x28` | the other horizontal span, as binary32 |
| `+0x2C` | (highZ − lowZ) as binary32 |
| byte `+0x30` | 1 |

It returns true. The ×32768 is computed by 64-bit multiply then signed divide by 65536 (helper client `0x02C900` / server `0x002DB0`). No draws.

### 9.6 Level service (client `0x19A9B0` / server `0x0D2340`) — audited except two inner calls

Inputs are two signed 64-bit fixed coordinates. The first is divided as signed 64-bit by 65536, and its low 32 bits are divided by 2048 with truncation. The second is converted to binary64, multiplied by 1/2048 (`0x3F40000000000000`, client `0x31DB20` / server `0x173740`), truncated to a 64-bit integer, and divided as signed 64-bit by 65536.

A region is looked up with those two indices (client `0x087DA0`, not audited). The service returns the region's 32-bit `+0x24` when the region exists, its `+0x18` is nonzero, and a binary32 measure from a further routine (client `0x1F8E10`, not audited) satisfies 1.0 (`0x3F800000`, client `0x345DC0` / server `0x1586DC`) ≥ measure. Otherwise, including NaN, it returns 1. No direct draws.

### 9.7 Behaviour object (M15 only) — not audited internally

In order:

1. A 12-byte object is built (client `0x0DC2D0` / server `0x01CFC0`).
2. Three parts are added to its list at +4, in this order:
   - a 20-byte part constructed with binary32 20.0 (`0x41A00000`; client `0x02C550` / server `0x0029E0`);
   - a 4-byte part (client `0x0C8130` / server `0x014720`);
   - a 28-byte part constructed with binary32 2.0, whose +4 sub-object receives the descriptor's `+0x10` position (client `0x18CFA0` and `0x1BF4F0` / server `0x0C5D50` and `0x0E1420`).
3. The object is stored at descriptor `+0x109C`.

### 9.8 Also used, simple and audited

Triple compare; fixed-position build (client `0x1A6A70`); vector add (client `0x02C800`); float-triple to fixed (client `0x02C460`, ×65536 binary32 then truncation); binary64 to fixed (client `0x059C00`, ×65536 binary64 `0x40F0000000000000` at client `0x2FD588` / server `0x1737F0`); int to fixed (client `0x012080`); list appends (client `0x086240`, `0x0862E0`, `0x1A0E00`, `0x26ADD0`). Their server counterparts are listed in section 11.

## 10. Not closed

- The column-value fallback service (client `0x1C4DD0` / server `0x0F8B40`): its result and any draws it makes.
- Draws and side effects in the descriptor constructor, the behaviour-object parts, the column-growth routine, the downward colour-fill routine of the cell writer, and the level service's region lookup and measure routine.
- The value copied into object-entry `+0x28`, which comes from an uninitialised local. The list-node allocator's handling of the uncopied `+0x04` and `+0x3C`.
- Behaviour for negative orientation remainders (destination left unassigned).
- The feature-record sub-object at `+0x48` and the full constructor state beyond the fields listed.
- Which callers pass which arguments 4, 7, 8 and 9, and which template data contain which markers. The caller census was not repeated here.
- The meaning or display name of any kind, class, selector or type value; none is asserted.

## 11. Client and server agreement

The routine was aligned instruction by instruction across the two images: 3739 client instructions against 3760 server instructions. With absolute addresses masked, every instruction matched, including all immediates (marker bytes, kinds, sizes, offsets and frame offsets). The only differences were seven places where the server inlines a deallocation or zeroing sequence (21 extra instructions in total, before client `0x1F1C43`, `0x1F1C5D`, `0x1F1E55`, `0x1F1E76`, `0x1F24F3`, `0x1F2544` and `0x1F27AD`). None of these affects outputs, draws or order.

The absolute-address correspondence was consistent at every aligned use. In particular:

- the four jump tables have identical entry orders at the same relative offsets;
- every binary64 and binary32 constant cited was decoded from both images with equal bit patterns;
- the static substitute cells have equal values set by start-up code in both images.

Thirty-two nested helpers named in section 9 were extracted from both listings and matched line for line under the same masking. They include the column lookup, getter, cell writer and element writer, footprint service and box finder, level service, fixed-point helpers, record constructor and initialiser, and the four append routines. The server counterparts include `0x006100`, `0x005FD0`, `0x005F20`, `0x01FF00`, `0x01FE60`, `0x0FF1A0`, `0x121ED0`, `0x0D2340`, `0x0D99D0`, `0x002CB0`, `0x002510`, `0x0DAB30`, `0x0CDE40`, `0x002DB0`, `0x0C84B0`, `0x0C83B0`, `0x128400`, `0x128450`, `0x0D6670` and `0x0F2BE0`.

No behavioural difference between the images was found.
