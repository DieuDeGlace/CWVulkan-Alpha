# Alpha combat behavior target selection

FINAL / STABLE / HELD. Independent source-side review, 2026-11-21.

## Evidence and scope

The owner's original 32-bit Alpha client, Cube.exe, has SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are relative to its preferred image base of 0x400000. The original
client instruction listing and individually scoped function exports were read against
the executable. Its type metadata identifies CombatBehavior, whose first virtual
entry is RVA 0x02CB20. This record ends at the normal no-target return or the
selected-target continuation at 0x02D2AE. It covers the call made to publish a fresh
choice. All actor offsets are measured from the creature object.

RVA 0x02C550 constructs the behavior with a caller-supplied binary32 radius at
behavior+0x08, an initial signed countdown of 20000 at +0x04 and zero at +0x0C
and +0x10. The copy-producing routine at 0x02EFC0 retains +0x04 and the radius
and initializes +0x0C and +0x10 to zero.

## Events that preempt selection

At 0x02CB60–0x02CC42, clear bit 0x04 in the actor's 16-bit flags at +0x124.
Clear the action byte (+0x68) when signed elapsed time (+0x6C) exceeds 10000,
except for actions 0x53, 0x52, 0x54 and 0x4F. A nonzero 32-bit field at +0x128,
or action 0x50 with elapsed time below 3000, immediately returns true.

Below one quarter of the maximum HP result from 0x044DB0, the actor can use an
item when elapsed time strictly exceeds the duration returned by 0x03D1A0.
Read the stored bag tabs in forward order and their 0x11C-byte entries in forward
order. The first entry with nonzero 32-bit quantity and leading item bytes 1, 1
qualifies. Set action 0x50 and elapsed time zero, copy the item to actor+0x1E8,
then decrement its source quantity. A signed result at or below zero clears the
quantity and both source item bytes. An unsuccessful search continues to selection.

The successful item use makes two CRT random draws at 0x02CCC1 and 0x02CCE8,
first for X (+0x40), then for Y (+0x44). Each integer draw is converted to
binary32, multiplied by 4, divided by 32767, and decreased by 2. Z (+0x48) is
zero. The update returns true. Candidate traversal, tie handling and publication
contain no random choice.

## Remembered and inherited identities

An actor's own score map is at +0x13A4. RVA 0x044BF0 visits it in ascending
signed 64-bit identity order. It begins at identity zero and binary32 score zero,
skips identity zero in the map, and replaces the current result for an ordered
score greater than or equal to the current best. Zero scores qualify. The last
identity wins an equal qualifying score. After traversal, the update makes one
exact lookup of the resulting identity in the world's creature map at world+0x04.
This includes a lookup of zero when the helper returns zero. An absent identity
or a null creature pointer yields an empty result. The scan never backtracks to
a lower-scoring identity. The resolved pointer bypasses fresh-candidate tests.

Type 5 in the actor byte at +0x60 triggers an additional exact lookup using the
actor's 64-bit link at +0x198. A missing or null linked creature preserves the
actor's previous result. A resolved link discards that result. If the linked
creature's action is 0x6A, the update returns false immediately, before the
readiness and timer writes below.

Otherwise, visit the linked creature's map at +0x13AC in ascending identity
order, starting without a target and at score zero. A score must be strictly
greater than the current best and its identity must resolve to a nonnull world
creature. A failed lookup leaves the best score available to subsequent entries.
The updater re-looks up an accepted identity to obtain the selected pointer.
The first resolvable identity wins ties for the highest positive score. This
path has no explicit identity-zero filter. An empty map leaves the actor with
no target. These operations occupy 0x02CC42–0x02CC88 and 0x02CD25–0x02CE36.
World exact lookup at 0x077D90 and the score-slot operation at 0x068AD0 order
keys by signed high word, then unsigned low word, each word 32 bits wide.

With no resolved target, a type-3 actor strictly below half maximum HP instead
enters action 0x50, clears elapsed time, writes 0x0401 to the first 16 bits of
+0x1E8 and the low 16 bits of its level (+0x190) to +0x1F8, zeros all three
motion components and returns true. This path at 0x02CE7C–0x02CEE7 draws no
random numbers. Both HP fractions multiply the binary32 maximum HP result.

## Fresh search

Subtract the supplied signed 32-bit time step from behavior+0x04, replacing a
negative signed result with zero. Scan the actor's status list at +0x1178 for
a record with byte 8 at record+0x08. Its presence blocks fresh acceptance.
An already selected pointer proceeds directly to readiness. Without a target,
the fresh search requires behavior+0x10 at or below zero as read before its
end-of-selection decrement. A type-5 actor also requires its 64-bit link to
be zero. A stale nonzero link can therefore suppress the search.

The initial best distance is the behavior radius squared in binary32. The
world creature map is visited from its least signed identity to its greatest.
Skip null creature pointers, the actor itself, candidates of type 5, and
candidates with finite HP (+0x16C) at or below zero. The native HP comparison
by itself also lets an unordered HP through. Each prospective winner must
finish all of the following checks before it changes the current best.

1. Subtract the actor's X, Y and Z from the candidate's corresponding signed
   64-bit fixed coordinates at +0x10, +0x18 and +0x20. Convert each difference
   to binary32 and multiply by 1/65536. Sum the binary32 squares in the order
   Y squared plus X squared, then Z squared.
2. When candidate+0x1190 is strictly positive, subtract the actor's signed
   32-bit level from the candidate's level. Convert the 32-bit result first
   to binary32 and then to binary64; raise 1.5 to that exponent and narrow
   the result to binary32. Multiply by the candidate's +0x1190 value and
   subtract the product from 1. Clamp that factor from below to binary32 0.1.
   Divide the original squared distance by the square of the factor. The
   multiply, subtract, square and divide here each use binary32.
3. The actor must lack status 8 and the resulting distance must be strictly
   below the best distance. Equality at the initial radius excludes a
   candidate. Equality with a prior winner preserves the earlier identity.
4. Evaluate the relationship with the actor first and candidate second.
   A failed relationship still passes this stage for a type-5 actor with
   link zero when the candidate has the matching action and item described
   below. All other failures leave the prior winner intact.
5. Check geometric visibility from the actor to the candidate. Then, if the
   actor satisfies the classification below, require the matching action and
   item even after a successful relationship test. Acceptance now replaces
   both the saved candidate pointer and the best distance.

The relationship at 0x196CA0 rejects a type-5 first creature paired with a
type-0 second creature, and any pair containing type 6. For other pairs,
exactly one type-1 creature qualifies. Two type-1 creatures qualify if their
classifications differ. All remaining pairs need bit 0x20 in the flags at
+0x124 of at least one creature. The fresh-search exception in step 4 applies
to a false return from this whole relationship test.

Classification at 0x044680 requires the creature's 16-bit field at +0x7E to
have none of mask 0x1A00 set, and its 32-bit species value at +0x64 to equal
one of these decimal identifiers: 19, 20, 22, 23, 25, 26, 27, 30, 31, 32,
33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106,
107, 145, 146, 147. A matching action and item requires candidate action
0x52, candidate byte +0x1020 equal to 0x14, and its unsigned byte +0x1021
equal to the actor's full 32-bit species value.

RVA 0x19EE90 receives the original fixed positions, mode 1 and maximum 200.
It accepts unadjusted squared distance strictly below binary32 0.0001 and
rejects squared distance strictly above 40000. Otherwise it takes a binary64
square root, narrows the length to binary32 and normalizes the three binary32
deltas by that length. The world ray query at 0x1A35D0 receives the actor
position, direction, length, zero and mode 1. Its returned binary32 length
must be at least the geometric length minus binary32 0.01. The level-adjusted
selection score has no effect on this visibility limit. The caller's fresh
survey spans 0x02CEF1–0x02D209.

## Publication and readiness

Only the completed fresh search publishes a new winner. At 0x02D213–0x02D232,
write binary32 1 to the actor's +0x13A4 score slot keyed by the selected
creature's own 64-bit identity at +0x08. RVA 0x1A0970 appends the actor ID,
target ID and current score to the output list at the supplied aggregate+0x50.
It then visits the world map in the same ascending order, skipping null
pointers, the actor and the chosen creature.

For each observer, require the ordinary relationship from observer to target.
Measure the unadjusted three-dimensional squared distance from observer to
the original actor. Only a distance strictly below 64 qualifies. Find or
create the observer's score slot at +0x13A4 for the chosen target. A missing
slot starts at zero. Only an ordered score equal to zero changes to 0.5 and
produces a further output record containing observer ID, target ID and score.
The observer survey has no additional HP, line-of-sight or matching-item test.

At 0x02D237–0x02D2A9, no selected pointer clears readiness bit 0x40 at
actor+0x124. A selected pointer and action zero set it. With a nonzero
action and selected pointer, the actor supplies its current action through
the -1 argument to each of three signed-duration helpers. Take half the
result of 0x044270 rounded toward zero, add 0x03CAA0, then add 0x047310.
The bit is set only if signed elapsed time strictly exceeds this 32-bit sum.

Subtract the time step from behavior+0x10, clamping a negative signed result
to zero. A positive countdown reaching zero here first enables a survey on
a later eligible update. The ordinary no-target exit also zeroes behavior+0x0C
and returns false. A selected pointer continues at 0x02D2AE for either state
of readiness. The update's earlier returns bypass these end-of-selection writes.

## Limits

The scope covers native static control flow for this client image and valid
containers in ordinary execution. Full duration and maximum-HP tables,
world ray collision internals, producers of species/status/score values,
selected-target movement, attack decisions and later random draws lie outside
this record. Signed 32-bit timer and level arithmetic and signed 64-bit
coordinate differences have their native widths. No cross-build, runtime
timing or exceptional floating-point environment was established.
