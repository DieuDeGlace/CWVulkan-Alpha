Status: FINAL / STABLE / HELD
# Inventory and equipment equality consumers

Scope: the client inventory-add routine at RVA 0x06EBE0 and three direct item-equality call sites inside the input routine at RVA 0x07B600. Supported image: `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000. All addresses are RVAs; evidence paths are relative to the original Alpha evidence root. Facts come from the original instructions in `decompiled/client/assembly.txt` (listing bytes for every cited range compared with the image file and matched) and individually named original function exports. No other contract, reconstruction or replacement/runtime material is evidence.

The inventory-add merge uses item equality RVA 0x02F4A0, after a separate category-permission test at RVA 0x07F9F0. The hand and equipment sites below call 0x02F4A0 directly as well. The category test is a gate, not a second equality algorithm. Because each caller normalises and admits operands differently, the four sites do not always compare the same kind of operands.

## The equality predicate

Item equality 0x02F4A0 compares its first operand (the object in ECX) with its second (the stack argument) and returns true only if every comparison below succeeds, performed in this order:

| Item offset | Width |
| --- | --- |
| +0x00, then +0x01 | one byte each |
| +0x04 | 32 bits |
| +0x0E | one byte |
| +0x08 | 32 bits |
| +0x0C | one byte |
| +0x10 | 16 bits |
| +0x0D | one byte |
| +0x114 | 32 bits |

Each comparison is on raw bits. No category is exempted from any field, +0x04 and +0x10 included. The predicate does not normalise quantities, add, convert to floating point or write anything. [Evidence: `binary/Cube.exe` and listing, RVA 0x02F4A0–0x02F507; `decompiled/client/functions/0042f4a0.c`.]

When the shared +0x114 value is greater than zero as a signed 32-bit integer, an entry loop follows. Entries are eight bytes each, starting at item+0x14. For each entry the predicate first compares entry byte 3, then entry bytes 0, 1 and 2 through RVA 0x028940, a helper that reports whether any of three consecutive bytes differ. Entry bytes 4 to 7 are never compared. The first entry thus checks item offsets +0x17 and then +0x14, +0x15, +0x16; the second checks +0x1F and then +0x1C, +0x1D, +0x1E; and so on. Entries at or beyond the count are ignored, as are the gaps +0x02–+0x03, +0x0F and +0x12–+0x13. This is a field-by-field comparison, not a comparison of all 0x118 item bytes. [Evidence: listing and image, RVA 0x02F509–0x02F562 and 0x028940–0x028969; `decompiled/client/functions/0042f4a0.c`, `00428940.c`.]

The count is not limited to the item's capacity of thirty-two entries. A zero or negative count skips the loop once the header and count compare equal. A positive count above 32 can make the loop read past the entry area if the earlier entries keep matching; an earlier mismatch returns false first. Neither case is turned into a safe malformed-count rule here. Different counts fail before the loop whatever their signs. These limits matter only when a caller actually reaches the predicate; category or other admission checks can prevent the call.

## Inventory-add normalisation and merge admission

An inventory entry is 0x11C bytes: a 32-bit entry count followed at +0x04 by a 0x118-byte item. This entry count is unrelated to item+0x10 and to the item's body count at +0x114. Routine 0x06EBE0 picks the destination group (when the passed group is −1, from classification RVA 0x07FA30), returns at once for an item whose category byte is zero, makes sure the group exists, and takes a local copy of the item before scanning entries. [Evidence: listing and image, RVA 0x06EBFE–0x06EC51, 0x06ED12–0x06ED74; `decompiled/client/functions/0046ebe0.c`, `0047fa30.c`.]

The amount starts at 1. For categories 0x0C, 0x0D, 0x15, 0x19, 0x14, 0x18 and 0x17, the amount becomes item+0x10 sign-extended from 16 to 32 bits, and **only the local copy's** +0x10 is set to 1. Category 0x0B gets the same treatment unless its subtype byte at +0x01 is exactly 0x0E, in which case it keeps amount 1 and its +0x10. All other categories keep amount 1 and their copied +0x10. The normalisation code also lists category zero, but that case cannot be reached because category zero has already returned. [Evidence: listing and image, RVA 0x06EC1D–0x06EC20, 0x06EC56–0x06EC9C.]

Before scanning, two categories leave through counter updates on the inventory object, with no item equality:
- Category 0x0D adds the amount to the 32-bit counter at inventory+0x12C and returns.
- Category 0x0C examines the incoming item's byte at +0x0D. For value 10 it adds the amount to the 32-bit counter at inventory+0x128; for 11 it adds the amount times 10000; for 12 it adds the amount times 100; each then returns. Any other value continues to ordinary handling.

So although the category test admits 0x0D, this caller never reaches a stacking comparison for that category. [Evidence: listing and image, RVA 0x06ECA3–0x06ED12.]

During the scan, at each entry the routine first remembers the index of the first entry whose 32-bit count is exactly zero. It then applies category test 0x07F9F0 to the original incoming item. That test returns true only for categories 0x01, 0x0A, 0x0C, 0x0D, 0x0B and 0x15, without regard to subtype or quantity. For an allowed category, equality 0x02F4A0 is called at RVA 0x06EDA6 with the entry's stored item as first operand and the normalised local copy as second. The stored item is not normalised first. [Evidence: listing and image, RVA 0x07F9F0–0x07FA0F, 0x06ED80–0x06EDAD; `decompiled/client/functions/0047f9f0.c`.]

No positive entry count is required before the comparison. An entry with count zero is remembered as empty yet still compared, and an entry with a negative count is neither treated as empty nor skipped. On equality the amount is added to that entry's 32-bit count and the routine returns at once, with ordinary 32-bit wraparound and no saturation or clamp. A later equal entry therefore wins over an earlier remembered empty one, and an empty entry that compares equal becomes the merge target itself. [Evidence: listing and image, RVA 0x06ED80–0x06EDAD, 0x06EDF9–0x06EE1E.]

When nothing merges, the first remembered zero-count entry receives the normalised item and the amount as its count; with none remembered, a new entry holding them is appended. Quantity normalisation does not imply merge permission: category 0x14, for example, is normalised but fails 0x07F9F0. The routine does not validate +0x114 before equality for the categories that reach it, and its item copy transfers all thirty-two body entries regardless of that count. [Evidence: listing and image, RVA 0x06EE21–0x06EE95; `decompiled/client/functions/0040ee70.c`, `0042c5e0.c`.]

## Hand-to-inventory comparison

Input routine 0x07B600 makes a hand-to-inventory equality call at RVA 0x07C6A3. The hand count is the 32-bit word at actor+0x11E8 and the hand item is at actor+0x11EC; the selected inventory row keeps its count apart from its item, as above. Reaching the call requires: a nonzero UI state word; selected group and row indices (widget +0x184 and +0x188) that are signed nonnegative and below their collection sizes; a **nonzero** hand count (so negative counts pass); and the hand item's classification from 0x07FA30 equalling the widget's current group value at +0x1B4. A classification mismatch calls a feedback routine (RVA 0x084350, selector 0x32) and leaves without comparing. [Evidence: listing and image, RVA 0x07C54D–0x07C651.]

The site then tests the hand item's category against an inline list identical to 0x07F9F0's six categories, without calling 0x07F9F0, and calls 0x02F4A0 with the stored hand item as first operand and the stored row item as second. No temporary +0x10 normalisation is applied here; an item may already be normalised by whatever produced it, but this caller adds none. There is no row-count positivity check and no body-count limit around the call. [Evidence: listing and image, RVA 0x07C657–0x07C6A8.]

On equality the whole hand count is added to the row count in 32-bit arithmetic, then the hand count, the hand item's category byte and its subtype byte are cleared. No saturation or clamp is applied. A zero hand count, a disallowed category or a failed comparison instead leads, when the UI byte at +0x29 is zero, to an entry swap through RVA 0x04A690, which exchanges the separate counts and copies item fields without any equality test of its own. This contract does not reconstruct every surrounding UI or input gate. [Evidence: listing and image, RVA 0x07C6AC–0x07C724; `decompiled/client/functions/0044a690.c`.]

## Equipment comparisons

Two equipment sites use the same equality with their own admission rules. The equipped item is at actor+0x1020. Categories 0x13 and 0x14 are decimal 19 and 20.

At RVA 0x07C131 the first operand is the hand item at actor+0x11EC and the second the equipped item at +0x1020. The selected equipment address must be actor+0x1020, and the hand must be either empty (count zero) or hold category 0x13 or 0x14. Only a hand count **greater than 1 as a signed value, with hand category 0x14**, reaches the equality; every other admitted case takes an item-swap route without comparing. Equal items skip the single-item replacement. Unequal items: if the equipped item's category is nonzero it is passed to inventory-add; the hand item is copied into the equipment slot; the hand count is decremented, and when the result is not positive the count is set to zero and the hand category and subtype bytes are cleared. No +0x10 normalisation or body-count check precedes the comparison. [Evidence: listing and image, RVA 0x07C0DC–0x07C190, 0x07C4A0–0x07C4CC.]

At RVA 0x07D218 the first operand is a selected inventory item and the second the equipped item at +0x1020. The surrounding inventory-use path has already required the row's count to be nonzero at RVA 0x07D006, which lets negative counts through. Category 0x14 reaches the equality and skips replacement when equal. Category 0x13 skips the equality and goes straight to replacement. Replacement copies the selected item to a temporary, passes the old equipped item to inventory-add (whose own category-zero return applies), writes the temporary into the equipment slot, and decrements the row count; when the result is not positive, the count, the item's category byte and its subtype byte are cleared. The stored items are compared without call-site normalisation and without limiting either body count. [Evidence: listing and image, RVA 0x07D006–0x07D012, 0x07D1FC–0x07D277; `decompiled/client/functions/0047b600.c`.]

Neither equipment site consults the stacking category test. Category 0x14 is compared here even though ordinary inventory-add merging and the hand-to-inventory list exclude it. Category 0x13 skipping one equipment comparison is a caller decision, not an exemption inside 0x02F4A0.

## Bounds of the result

The four identified sites (0x06EDA6, 0x07C6A3, 0x07C131, 0x07D218) all use the same predicate and therefore the same entry-byte rule. Whether the call happens, whether +0x10 was normalised locally, and which entry or hand counts are admitted are decided by each caller. Neither the greater-than-one hand-count gate at 0x07C131 nor any row index bound validates item+0x114, and the loop limits above must not be replaced by an assumed safe range of 0 to 32.

Not claimed: that every item equality in the image uses this routine; that malformed inventory pointers or groups are rejected safely; or that all higher-level equipment and hand actions are listed. The equality and category predicates draw no random numbers and do no floating-point work. Alias invalidation during allocation, allocator failure, concurrent mutation, full input dispatch and any server counterpart are outside scope.

## Re-certification (2026-11-14)

Re-checked by a source-side author against the original client image and its listing (bytes compared with the file for all cited ranges) and the named exports. The Infinite-Cubes reconstruction was not consulted.

Confirmed:
- Equality field order and widths, signed count gate, eight-byte entry stride from +0x14, byte-3-then-bytes-0..2 order through 0x028940, ignored bytes and gaps.
- Category test 0x07F9F0 list (0x01, 0x0A, 0x0C, 0x0D, 0x0B, 0x15).
- Inventory entry layout (0x11C stride, count then item); category-zero return; normalisation category set and the 0x0B/0x0E exception; local-copy-only +0x10 rewrite.
- Scan order (remember first zero-count entry, category test on the incoming item, equality with stored item first), 32-bit unclamped merge, fall-back placement.
- Hand-to-inventory gates, inline allowlist, operand order, merge and clears.
- Both equipment sites' admission, operand order, replacement sequence and clearing.

Corrected:
- The category 0x0C and 0x0D counter paths are now specified (counters at inventory+0x128 and +0x12C; multipliers 1, 10000 and 100 for +0x0D values 10, 11 and 12) instead of being described only as "counter paths".
- "positive hand-count gate at 0x07C131" corrected to the actual greater-than-one test.
- The hand-to-inventory fall-back swap is now stated to require the UI byte at +0x29 to be zero, and a zero hand count is stated to go to that fall-back rather than to classification.
- At 0x07C131 the old equipped item goes to inventory-add only when its category is nonzero; at 0x07D218 inventory-add is called unconditionally.
- Clearing at the equipment sites is now stated precisely (count, category byte and subtype byte).
- Prose rewritten in independent wording.

Unresolved:
- Meaning of the inventory counters at +0x128/+0x12C and of the UI fields used as gates.
- Whether other item-equality routines exist in the image; complete input dispatch; allocator and alias behaviour; server counterpart.
