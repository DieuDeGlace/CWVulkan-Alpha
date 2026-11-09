# Alpha terrain candidate sort ordering

Status: FINAL / STABLE / HELD. This original-only contract closes the ordinary value-order relation, size thresholds, small-range stability, an all-equal invariant and an exact larger-range tie family. It does not supply a complete permutation rule for arbitrary larger mixed-key inputs with ties. That remaining limit affects parity when a selected prefix cuts a tie group.

The relative evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x400000`. All routine addresses below are RVAs. Evidence consists of original executable bytes and individually identified original function exports. No other contract, replacement implementation, runtime experiment or reconstruction supplied facts.

## Inputs and conserved record contents

The terrain renderer calls RVA `0x0ABAE0` at `0x0B0E84` with the complete temporary candidate range. The wrapper derives its record count from the byte span using 48-byte stride and passes that count as the initial partition budget to RVA `0x0AA580`.

The sort compares only the binary32 key at candidate `+0x28`. Its inspected movement helpers carry the first 44 bytes of each record, including the key, together. These meaningful contents are conserved as a multiset through sorting. The four bytes at `+0x2C`–`+0x2F` are outside the inspected semantic copies and swaps; this is not a promise that all 48 physical bytes move together.

For a well-formed range with ordinary finite comparable keys, output keys are nondecreasing. Distinct keys therefore determine a unique meaningful-record order. Equal keys do not invoke a comparison of position, radius, color, source identity, original index or address. Positive and negative zero compare equal; no sign-bit tie breaker is present. The extra context argument carried through the sort helpers does not supply an inspected secondary comparator.

The sort does not recompute, normalize or modify key values. Holding input key order and count fixed while changing another meaningful payload field does not introduce a new comparison criterion. No random draw occurs in these inspected sort helpers. A fixed valid input has a deterministic native permutation, even where the exact permutation has not been fully contracted below.

This relation excludes corrupt spans, exceptional floating-point state, NaNs, infinities and unclosed subnormal/control-mode behavior. The exact examples below use only binary32 zero and one and do not depend on those exceptional cases.

Sources: `binary/Cube.exe`, call RVAs `0x0B0E42`–`0x0B0E84`, wrapper `0x0ABAE0`–`0x0ABB0D`; `decompiled/client/functions/004abae0.c`, `004aa580.c`, `004abb10.c`, `004a9520.c`, `004aa0d0.c`, `004a9200.c` and `004aa160.c`. All shortened function filenames in this document are relative to `decompiled/client/functions/`.

## Exact size and budget boundaries

The following thresholds apply to the current subrange, not just the original total candidate count.

| Current subrange size | Established behavior |
| --- | --- |
| 0 or 1 | No record reordering |
| 2 through 32, inclusive | Insertion ordering; equal ordinary keys preserve their order on entry to this subrange |
| 33 through 41, inclusive, with positive budget | Partition route whose preliminary sampling uses the first, middle and last records |
| 42 or more, with positive budget | Partition route whose preliminary sampling uses nine record positions |
| More than 32, with exhausted budget | Heap construction followed by heap extraction |

The middle position is the integer half of the subrange count, rounded down, measured from its first record. The last position is count minus one. The native sampling threshold compares the distance from first to last against 40, so a range of 41 still uses three sampled records and a range of 42 uses nine.

For the nine-position case, the spacing is the integer eighth of the subrange count, rounded down. The sampled positions form three groups: the first record and the next two spacing-separated positions; the middle position and the positions one spacing to either side; and the last record and the two spacing-separated positions before it. Selection of the middle-of-three values in those groups and among their representatives can exchange actual meaningful records before the main partition. Sampling is not merely a read of key values.

The initial budget equals the original full count. After each partition, the next budget is the sum of the previous budget's integer half and integer quarter, each rounded down separately. This is not always the same as rounding down three quarters of the previous budget. Both remaining outer partitions inherit that reduced budget. The strictly smaller outer partition is processed recursively first; an equal-size comparison takes the right outer partition first. The pivot-equal middle interval is not recursively sorted.

Small subranges reached after earlier partitioning preserve their order as it exists at that point. Their insertion stability does not restore the original relative order of tied records already displaced by sampling or partitioning.

Sources: `binary/Cube.exe`, main thresholds and budget RVAs `0x0AA594`–`0x0AA6AB`; sampling threshold `0x0A9F13`–`0x0A9F39`; `004a9f00.c`; middle selection in `004ab190.c`, native RVAs `0x0AB1AF`–`0x0AB1E2`; insertion movement in `004a9520.c` and `004aa0d0.c`.

## Closed tie behavior

For an entire input of at most 32 records, the output is stable by key: among numerically equal ordinary keys, meaningful records retain input order. A record with a strictly smaller key may move before them, but equal-key records are not displaced past one another by this insertion route.

For an entire input whose keys are all numerically equal, the meaningful-record sequence remains unchanged at any ordinary valid size, including sizes above 32 and above 41. The sampling comparisons make no strict-key exchange, the initial equal interval expands to cover the range, and no nonempty outer partition remains. Thus large-input instability must not be interpreted as reversal or scrambling of every all-equal group.

Larger mixed-key inputs have no general stability guarantee. In particular, a preliminary exchange of records having different keys can move one record across other records tied with it. The absence of a direct equal-key comparison exchange does not protect relative tie order.

Sources: `004a9520.c`; `004a9f00.c`; `004ab190.c`; original native initial equal-interval scan at RVAs `0x0AB1EA`–`0x0AB2C9`, termination checks `0x0AB4BF`–`0x0AB4C7` and returned interval `0x0AB848`–`0x0AB855`.

## Exact finite-key family at the first partition threshold

The following result is derived directly from the original comparisons and record movements. The record numbers are analytical labels for input positions, not native source identifiers or added fields.

Take 33 input records numbered 0 through 32. Give record 16 key zero and every other record key one. Their other meaningful contents may differ.

The exact meaningful-record output is: record 16 first; records 1 through 15 in their original order; record 0; then records 17 through 32 in their original order. The first and middle input records have exchanged positions and the remaining positions are unchanged. The subsequent equal interval and its one-record outer range make no further exchanges.

For comparison, take the 32-record input numbered 0 through 31, again with only record 16 carrying key zero. Its exact output is: record 16 first; records 0 through 15 in their original order; then records 17 through 31. This is the stable insertion result.

Consequently, adding input record 32 with the same key one changes the selected first-16 membership even though that additional record is itself far outside the resulting prefix. With 32 records, the prefix consists of record 16 and records 0 through 14. With 33 records, it consists of record 16 and records 1 through 15. Record 0 is replaced by record 15 at this prefix boundary.

The same exchange-only family extends to original counts 33 through 41: when the sole smaller-key record occupies the middle position and every other key is the same larger value, the first and middle input records exchange positions, with every other position unchanged. The key values may be any two ordinary finite values in strict increasing order. This statement does not extend that positional result to the nine-sample route at 42 or more.

Sources: native three-sample strict comparison/exchange RVAs `0x0AA07E`–`0x0AA0BB`; record swap helper `0x0ABB10`; middle and equal-range handling `0x0AB1AF`–`0x0AB2C9`, `0x0AB3AD`–`0x0AB4C7`, `0x0AB848`–`0x0AB855`; outer-range handling `0x0AA5C6`–`0x0AA66F`. These are static deductions from original evidence, not results of executing a substitute sort or a native probe.

## Heap fallback and selection dependencies

An exhausted-budget subrange above 32 takes the heap route at `0x0AA676`. The native downward heap adjustment chooses the right child when both available child keys are equal. Its upward saved-record placement stops at an equal parent rather than moving past it. Heap extraction still moves meaningful records and supplies no stable input-index ordering guarantee. These are local tie decisions, not a complete expression of the full sort's final tied-record order.

The renderer consumes the shared sorted range in order. Its per-chunk admission checks the four-byte selected count at chunk `+0x238` against 16 at `0x0B1180`–`0x0B1187`; candidates that qualify for an already full chunk are skipped. Therefore the exact first-16 example above becomes a selection difference when those records all qualify for the same chunk. If some records fail a chunk's spatial admission, its retained sequence is instead the qualifying subsequence of the native shared order. The sort does not rerank that subsequence for each chunk.

This establishes that count and the surrounding input sequence can affect a chunk's tie selection. A candidate that does not itself enter a particular chunk can still affect shared sampling or partitioning. Source traversal order, duplicate-source append order and the keys of other candidates are therefore potentially relevant inputs, rather than just the unordered set of lights spatially eligible for one chunk.

Sources: `004a9a00.c`, `004aa840.c`, `004a9200.c` and `004aa160.c`; native fallback RVAs `0x0AA676`–`0x0AA6AB`, child comparison `0x0A9226`–`0x0A9234`; original renderer RVAs `0x0B0FC0`–`0x0B12D9`, especially capacity `0x0B1180`–`0x0B1187` and append/count advancement `0x0B123C`–`0x0B126C`.

## Precise remaining boundary

The general ascending-key relation, meaningful-content conservation, exact thresholds, local heap tie decisions, small-input stability, all-equal invariant and the stated 33–41 family are closed. A complete final meaningful-record permutation for arbitrary mixed finite tied inputs above 32 remains unclosed, including permutations arising from repeated partitioning, the nine-sample route and exhausted-budget fallback.

That limit is narrower than an unknown key order and wider than a cosmetic difference: when a retained prefix cuts a tied group, the exact native members and their order remain a dependency outside the closed cases above. This document does not authorize a stable secondary index, source-ID ordering, arbitrary tie selection or a different library sort as a native-equivalent replacement. It also does not claim that a finite list of illustrative permutations covers all mixed-key cases.

Source creation/traversal, key production, full chunk eligibility, shader consumption, exceptional floating-point environments and all other sort call sites are outside this follow-up. Earlier held artifacts remain unchanged.
