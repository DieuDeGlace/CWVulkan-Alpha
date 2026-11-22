# Alpha generic hit source equipment and self-hit record

**FINAL / STABLE / HELD** · Original client static evidence · 2026-11-22

The supported original is the 32-bit x86 Alpha client `Cube.exe`, 3,885,568 bytes, SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`. Its image base is 0x400000, image size 0x3BC000 and PE timestamp 0x51EA955E. All routine and instruction addresses below are RVAs. The evidence is the original executable and the individual original-client routine listings for 0x196D30, 0x03DF60, 0x03CA60 and 0x044CB0. Constants, branch tables, rounding order and record fields were checked directly in that image.

## Admission and source ratio

This is the ordinary, sourced path of generic hit 0x196D30, after the earlier recipient defenses have yielded a working hit amount. The recipient is the actor in the first stack argument, the source is the actor in the second. Source equipment and source health belong to the latter. A missing source takes a separate path. The auxiliary block begins at 0x1976D8 and ends with the update immediately before 0x197A79.

At 0x19754D the caller takes the source's signed 32-bit value at +0x70, converts it to binary32, and divides it by the separately converted signed 32-bit return value of 0x044CB0 on that same source. This is a fresh ratio. It has no upper clamp here. A prior ratio used during defense has a different clamp. The interval at 0x1976D8 receives the fresh ratio after an earlier per-item call path that runs for ratios strictly above 0.25. That earlier threshold does not gate the auxiliary block.

The denominator at 0x044CB0 uses the source's binary32 field +0x17C. Multiply it in binary32 by the factor below and truncate toward zero to a signed 32-bit integer before converting it back to binary32 in the caller. The first two bytes of the item at source +0xAA8 are its category and subtype.

| Condition | Factor |
| --- | ---: |
| Category 3, subtype 0x06 | 60 |
| Category 3, subtype 0x08 | 80 |
| Category 3, subtype 0x0B, 0x0C, 0x0F, 0x10 or 0x11 | 20 |
| Category 3, every other subtype | 50 |
| Other category, source byte +0x7E has bit 0x10 set | 20 |
| Other category, that bit clear | 50 |

The caller admits the block only when the low byte of the fifth stack argument (frame +0x18) is nonzero and the fresh ratio compares strictly greater than zero. A zero, negative or unordered ratio skips equipment selection, counter caps and the self record. The original division has no zero-denominator guard. With ordinary masked IEEE floating-point exceptions a positive numerator and zero denominator give positive infinity, while zero divided by zero fails the ordered admission test. Admission requires neither a positive working hit amount nor the separate later audiovisual flag.

## Which source items participate

At 0x1976F6 the caller obtains a fresh, ordered selection from 0x03DF60. Each entry references an equipped item on the source. Eligible items have category byte 3 at item +0 and subtype byte at +1. If the item at +0xAA8 has category 3 and subtype 6 or 7, select it alone. Otherwise, if the item at +0x990 meets that same test, select it alone. Either choice takes priority over the source action byte at +0x68.

| Source action byte, hexadecimal | Selection after the priority checks |
| --- | --- |
| 02, 04, 07, 08, 0A, 12, 27, 29 | Select +0x990 if eligible. |
| 28, 2A | Select +0xAA8 if eligible. |
| 0B, 0C, 10, 11, 14, 36, 56, 5B, 60 | Select +0x990 first and +0xAA8 second, testing eligibility independently. |
| Every other byte | Select +0xAA8 if eligible, then +0x990 if eligible and its subtype is 0x0C. |

The selection has zero, one or two entries. A zero count bypasses all remaining auxiliary work, including the two counter caps. Otherwise the common per-item weight is the binary32 constant 0.1 (bits 0x3DCCCCCD) divided in binary32 by the selected item count. The count conversion treats it as unsigned. All selected items count toward the divisor, including an item with no recognized entries.

## Item values and per-entry effects

Begin the source recovery total at positive binary32 zero. Visit the items in selection order. Item +0x114 is a signed 32-bit entry count. A nonpositive count contributes no entries. For a positive count the caller visits entries in increasing index order. The first tag byte is at item +0x17, its signed 32-bit value is at +0x18, and both advance eight bytes per entry. There is space for 32 such entries before the count. The loop trusts the count and contains no separate bound of 32. Behavior for malformed out-of-record counts is outside this contract.

For each recognized tag, convert its signed integer value to binary32. The curve at 0x03CA60 subtracts one from that input, multiplies the difference by binary32 0.05 (bits 0x3D4CCCCD), then adds one. Divide one by that result and subtract the quotient from one. Every operation is binary32, including the value returned through the native floating-point return register. There is no curve clamp or random draw. For example, the integer inputs 1 and 21 give curve values 0 and 0.5 respectively. An unknown tag skips the curve and has no effect.

| Tag | Working hit amount | Other effect of this entry |
| --- | --- | --- |
| 0x80 | Multiply curve by weight, then by ratio. Add the result to the running hit amount. | None. |
| 0x81 | Multiply curve by weight, then by ratio. Multiply that common result by 0.5 and add it to the running hit amount. | Multiply the same common result by 2 and add it to the running source recovery total. |
| 0x82 | Multiply weight by 0.25 first. Multiply the curve by that reduced weight, then by the ratio. Add the result to the running hit amount. | Advance the recipient's signed 32-bit counter at +0x134. |
| 0x83 | Use the same order and contribution as 0x82. | Advance the source's signed 32-bit counter at +0x138. |

All multiplications and running additions round separately in binary32. The counter advance is independent of the entry's curve and the number of selected items. Multiply the ratio by 500 in binary32, convert the current signed counter to binary32, add those two binary32 quantities, then truncate the sum toward zero and store the signed 32-bit result immediately. Repeated entries therefore truncate separately, including when the prior counter is negative. The native conversion produces the signed indefinite value 0x80000000 for a NaN or out-of-range result with masked exceptions.

After all entries on all selected items, cap the recipient's +0x134 and then the source's +0x138 at signed 5000. These are upper caps only. Both run even when no entry contributed or no matching timer tag was present, provided selection was nonempty. Earlier per-entry writes precede the caps. This interval neither floors the running hit amount nor limits the ratio to one.

## Positive source recovery and ordering

Only an ordered, strictly positive final recovery total produces a self-hit record. There is at most one per generic hit regardless of the number of 0x81 entries. The record is appended to the caller's first hit list before the source's health update. The source's 64-bit ID at +0x08 appears in both actor ID fields and its three unscaled 64-bit position components at +0x10, +0x18 and +0x20 appear in the position. The binary32 record amount is the recovery total with its sign bit flipped.

| Offset in record | Value |
| --- | --- |
| +0x00, +0x08 | Source ID in each 64-bit field. |
| +0x10 | Negative binary32 recovery amount. |
| +0x14 | Zero byte. |
| +0x18, +0x1C | Zero 32-bit fields. |
| +0x20, +0x28, +0x30 | Source position components, copied bit for bit. |
| +0x38, +0x3C, +0x40 | Three zero binary32 components. |
| +0x44, +0x45, +0x46 | Three zero bytes, including the outcome byte at +0x45. |

The constructor at 0x059A30 and copy at 0x0599A0 establish the initialized fields. Intervening padding has no assigned behavioral value. The append at 0x197A2E prepares the entry using 0x051EE0, checks the hit-list count, increments it and links the new last record. For a valid list already at 0x03333332 records, the native length failure occurs before linking and before adding recovery to health. On success, add the positive recovery total in binary32 directly to source +0x16C. This block applies no maximum-health cap, target-health update, random draw, secondary sound, or direct ordinary-hit application for this self record. The later main hit is separate. Earlier equipment effects above ratio 0.25 and later audiovisual or hit processing retain their own draw order.

As an arithmetic cross-check, with two selected items, ratio 0.125 and a single 0x81 entry of value 21, the selected-item weight is binary32 0.05. The working hit gains about 0.0015625 and the source gains about 0.00625 health after one self record for the negative of that latter quantity. A separate 0x82 entry at the same ratio advances a recipient counter starting at −100 to −37 before the final caps, even if the entry's curve is zero.

The evidence establishes the native field identities, selection, numerical effects and local append order. Player-facing names for the four tags and the two counters, valid gameplay ranges for the signed item values, and live acceptance across the whole generic-hit caller remain open. This review concerns the identified Alpha client image.
