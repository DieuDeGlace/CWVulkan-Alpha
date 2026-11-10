# Inventory and equipment equality consumers

This independent contract covers the original client inventory-add routine RVA `0x06EBE0` and the three direct item-equality call sites in input routine `0x07B600` identified below. The supported image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, base `0x00400000`. Addresses are RVAs; relative evidence paths refer to the original Alpha evidence root. Findings were checked against original native instructions and original function exports. No previous project contract or replacement/runtime material supplies evidence.

The inventory-add merge uses **item equality RVA `0x02F4A0`**, after a separate category-permission predicate at `0x07F9F0`. The examined hand and equipment comparison sites also call `0x02F4A0` directly. The category predicate is not an alternative item-equality algorithm. Different caller normalization and admission rules nevertheless mean these sites do not compare identical kinds of operands in every case.

## The actual equality predicate

Item equality `0x02F4A0` returns true only after these comparisons succeed, in native order:

| Item-relative field | Width |
| --- | --- |
| `+0x00`, then `+0x01` | One byte each |
| `+0x04` | 32 bits |
| `+0x0E` | One byte |
| `+0x08` | 32 bits |
| `+0x0C` | One byte |
| `+0x10` | 16 bits |
| `+0x0D` | One byte |
| `+0x114` | 32 bits |

The field values are compared as supplied bits. There is no category-specific exemption inside equality for `+0x04`, `+0x10` or any other listed field. The predicate does not perform quantity normalization, integer addition, floating-point conversion or mutation. [Evidence: `decompiled/client/functions/0042f4a0.c`; `binary/Cube.exe`, RVAs `0x02F4A0–0x02F507`.]

When the common `+0x114` value is positive as signed 32 bits, it controls an entry loop. Each entry occupies eight bytes beginning at item `+0x14`. For each entry, equality first compares byte 3, then compares bytes 0, 1 and 2 through three-byte inequality helper `0x028940`. Entry bytes 4 through 7 are not compared. Thus the first entry contributes item offsets `+0x17`, then `+0x14/+0x15/+0x16`; the next contributes `+0x1F`, then `+0x1C/+0x1D/+0x1E`, and so on. Entries beyond the supplied count are ignored. Alignment gaps `+0x02–+0x03`, `+0x0F` and `+0x12–+0x13` are also ignored. This is field-wise equality, not a comparison of all `0x118` item bytes. [Evidence: `decompiled/client/functions/0042f4a0.c`, `00428940.c`; `binary/Cube.exe`, RVAs `0x02F509–0x02F562`.]

The common count is not clamped to the item's thirty-two-entry body capacity. Equal zero or negative count fields skip the entry loop after the header/count comparisons. A positive count above 32 can cause inspection beyond that body if preceding comparisons continue to match; an earlier mismatch can return false first. Neither outcome is converted here into a safe malformed-count rule. Differing count fields fail before the entry loop, regardless of their signs. These limits apply only when a caller actually reaches this predicate; a category or other admission return can prevent the comparison altogether.

## Inventory-add normalization and merge admission

Inventory entries have stride `0x11C`: a separate 32-bit entry count followed at `+0x04` by a `0x118`-byte item. This entry count differs from both item `+0x10` and the item's body count `+0x114`. Inventory-add `0x06EBE0` selects its destination group, rejects category zero, ensures the group exists and makes a local item copy before its ordinary-entry scan. [Evidence: `decompiled/client/functions/0046ebe0.c`, `0047fa30.c`; `binary/Cube.exe`, RVAs `0x06EBFE–0x06EC51`, `0x06ED12–0x06ED74`.]

Its initial amount is 1. For categories `0x0C`, `0x0D`, `0x15`, `0x19`, `0x14`, `0x18` and `0x17`, it instead sign-extends item `+0x10` from 16 to 32 bits as the amount and changes **only the local copy's** `+0x10` to 1. Category `0x0B` receives that treatment unless subtype byte `+0x01` is exactly `0x0E`. Other categories retain amount 1 and their copied `+0x10` unchanged. The category-zero normalization branch visible later in the routine does not make category-zero items reach the ordinary scan, because the earlier category-zero return already applies. [Evidence: `binary/Cube.exe`, RVAs `0x06EC1D–0x06EC20`, `0x06EC56–0x06EC9C`.]

Before the ordinary scan, category `0x0D` returns after updating its inventory counter. Category `0x0C` also returns through a counter path for item byte `+0x0D` values 10, 11 or 12; its other values can proceed to ordinary handling. Those counter paths perform no item equality. Accordingly, the category-permission predicate's inclusion of `0x0D` does not imply an ordinary stacking comparison for that category in this caller. [Evidence: `binary/Cube.exe`, RVAs `0x06ECA3–0x06ED12`.]

At each candidate entry, `0x06EBE0` first remembers the first entry whose separate 32-bit count is exactly zero. It then calls category predicate `0x07F9F0` on the original incoming item. This predicate returns true only for categories `0x01`, `0x0A`, `0x0C`, `0x0D`, `0x0B` and `0x15`, without examining the subtype or quantities. For a permitted category that has reached this point, the caller invokes equality `0x02F4A0` at RVA `0x06EDA6`, with the candidate entry's item as its first operand and the normalized local copy as its second. The candidate's stored item is not normalized before that call. [Evidence: `decompiled/client/functions/0047f9f0.c`, `0046ebe0.c`; `binary/Cube.exe`, RVAs `0x07F9F0–0x07FA0F`, `0x06ED80–0x06EDAD`.]

There is no positive-entry-count requirement before this equality. A zero-count entry is remembered as empty but still participates in the comparison, and a negative-count entry is not treated as empty or excluded from comparison. On equality, the incoming amount is added to that entry's 32-bit count and the routine returns immediately. The result has native 32-bit wraparound with no saturation or nonnegative clamp in this merge. Therefore a later equal entry can win over an earlier remembered empty entry; when the empty entry itself compares equal, it can be the immediate merge destination. [Evidence: `binary/Cube.exe`, RVAs `0x06ED80–0x06EDAD`, `0x06EDF9–0x06EE1E`.]

If no merge occurs, the first remembered zero-count entry receives the normalized item and the amount, or a new entry is appended if none was remembered. Categories normalized for quantity are not necessarily permitted to merge: for example, `0x14` takes quantity normalization but fails `0x07F9F0`. The caller does not validate item body count `+0x114` before equality for those categories that do reach equality. Its local copy transfers all thirty-two body entries independently of that count. [Evidence: `binary/Cube.exe`, RVAs `0x06EE21–0x06EE95`; `decompiled/client/functions/0040ee70.c`, `0042c5e0.c`.]

## Hand-to-inventory comparison

Input routine `0x07B600` has a hand-to-inventory merge call at RVA `0x07C6A3`. The hand's separate count is actor `+0x11E8`, with its item at `+0x11EC`; the selected inventory row again stores its count separately from its item. The shown route requires the relevant UI state to be nonzero and selected group/row indices to be signed nonnegative and in range. The hand count must be **nonzero**, which also admits a negative count. The hand item's classification from `0x07FA30` must equal the widget's current group value at `+0x1B4`. [Evidence: `binary/Cube.exe`, RVAs `0x07C54D–0x07C651`.]

This route uses an inline category allowlist equal to `0x07F9F0`'s six categories, then directly calls `0x02F4A0`. It does not call `0x07F9F0` itself. Its first operand is the hand item as stored and its second is the selected row item as stored. There is no temporary `+0x10` quantity normalization at this comparison site. Earlier item producers can already have normalized a stored item; this caller supplies no new such normalization. There is no target-row positivity check or item body-count clamp immediately around the call. [Evidence: `binary/Cube.exe`, RVAs `0x07C657–0x07C6A8`.]

On equality, it adds the entire separate hand count to the selected row's count as 32-bit integer arithmetic, then clears the hand count and its item category/subtype. No saturation or nonnegative-result clamp appears in that merge. Failure or disallowed-category routes can reach other handling, including entry swapping through `0x04A690`; that helper swaps the separate counts and copies item fields without performing its own equality. This contract does not reconstruct every surrounding UI or input gate. [Evidence: `binary/Cube.exe`, RVAs `0x07C6AC–0x07C724`; `decompiled/client/functions/0044a690.c`.]

## Equipment comparisons

Two examined equipment routes use the same equality with different category/count admission. Their equipped item is local actor `+0x1020`. Here `0x13/0x14` are hexadecimal categories 19/20 decimal.

At RVA `0x07C131`, the first operand is the hand item at actor `+0x11EC` and the second is equipped item `+0x1020`. The selected equipment address must match `+0x1020`; the earlier admission accepts a zero hand count or hand category `0x13/0x14`. Only **signed hand count greater than 1 with category `0x14`** reaches this equality. Other admitted cases take a swap route without this comparison. Equal items skip the one-item replacement. Unequal items can return the old nonempty equipped item through inventory-add, copy the hand item into equipment and decrement the separate hand count; nonpositive resulting counts are cleared. No `+0x10` normalization or body-count validation occurs before this equality. [Evidence: `binary/Cube.exe`, RVAs `0x07C0DC–0x07C190`, `0x07C4A0–0x07C4CC`.]

At RVA `0x07D218`, the first operand is a selected inventory item and the second is equipped item `+0x1020`. The enclosing inventory-use path has already required its separate entry count to be nonzero at RVA `0x07D006`; that test also admits negative counts. Category `0x14` reaches equality and skips replacement on equality. Category `0x13` bypasses equality and takes replacement directly. In the replacement sequence, the selected item is copied temporarily, the old equipped item goes through inventory-add, the temporary replaces equipment, and the selected row count is decremented and cleared if nonpositive. The comparison uses the two stored items without quantity normalization at the call site, and it does not clamp either item's body count. [Evidence: `decompiled/client/functions/0047b600.c`; `binary/Cube.exe`, RVAs `0x07D006–0x07D012`, `0x07D1FC–0x07D277`.]

These equipment comparisons do not consult the ordinary stacking category predicate. In particular, category `0x14` can be compared here despite being excluded from ordinary inventory-add merging and the hand-to-inventory merge allowlist. Conversely, category `0x13` bypassing one equipment equality is not a general exemption inside `0x02F4A0`.

## Bounds of the result

The compared entry-byte rule is the same predicate at all four identified call sites. Whether the call occurs, whether item `+0x10` was normalized locally, and which separate entry/hand counts are admitted belong to the actual caller. Neither the positive hand-count gate at `0x07C131` nor an inventory row's index bounds validate item `+0x114`. The body-loop limits stated above must not be replaced by an inferred safe 0–32 range.

This audit does not claim that every item equality in the image uses this routine, that all malformed inventory pointers/groups are safely rejected, or that all higher-level equipment and hand actions have been enumerated. The identified equality/category predicates perform no random draw or floating-point arithmetic. Source/destination alias invalidation during allocation, allocator failures, concurrent mutation, complete input dispatch and any server counterpart remain outside scope.
