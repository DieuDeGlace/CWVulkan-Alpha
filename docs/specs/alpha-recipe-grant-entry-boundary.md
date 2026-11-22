# Alpha recipe grants at character and world entry boundaries

Source-side re-certification, 2026-11-21. **FINAL / STABLE / HELD** after full author readback.

The supported original is the 32-bit client `binary/Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base
`0x00400000`. Addresses below are RVAs. Evidence comes from that image, its client instruction
listing `decompiled/client/assembly.txt` and individually identified original function exports
under `decompiled/client/functions/`. The earlier entry-boundary record supplied the claims to check.

## Grant callers and creature fields

The original image contains two direct calls to the level-grant routine at `0x07FAE0`. Character
creation calls it at `0x0777B6`. Client update calls it at `0x093EC4`. The recipe membership query
is `0x044A90`. The two traced calls at `0x07BBA7` and `0x0A1604` reach a separate routine at
`0x19CFF0` that constructs recipe requirements from an item descriptor and a destination. The
requirement contents are outside this contract. Original exports `004772b0.c`, `0047fae0.c`,
`00444a90.c` and `0059cff0.c` identify these roles.

Both grant callers and the grant routine select the creature through controller `+0x8006D0`.
Offsets in the following table are relative to the creature or its recipe owner as indicated.

| Owner | Offset | Native meaning |
| --- | --- | --- |
| Creature | `+0x140` | Player class byte read during grant generation |
| Creature | `+0x190`, `+0x194` | Current level and current progress, each 32 bits |
| Creature | `+0x1D28` | 32-bit pointer to the object that owns the known-recipe list |
| Creature | `+0x1D3C`, `+0x1D40` | Last-observed level and progress, each 32 bits |
| Recipe owner | `+0x14`, `+0x18` | List sentinel pointer and 32-bit recipe count |

Each allocated recipe node occupies `0x120` bytes. Its forward and backward 32-bit links precede
the `0x118`-byte descriptor at node `+0x08`. Original exports `0044b770.c` and `00449b70.c`, the
grant block at `0x07FB64–0x07FBDF` and the level/progress checks at `0x093D64–0x093EE7` agree.

## Construction, reset and new-character setup

The creature constructor `0x03B690` calls the base-data constructor `0x03C100` on creature `+0x10`.
Base-relative `+0x180` and `+0x184` therefore refer to creature level and progress. Construction
sets progress to zero at `0x03C27D` and level to one at `0x03C2A1`. The creature constructor sets
the recipe-owner pointer to null at `0x03BB52`, the level tracker to one at `0x03BB6D` and the
progress tracker to zero at `0x03BB77`. Creating the creature alone supplies no recipes. Original
exports `0043b690.c` and `0043c100.c`, with the subobject call at `0x03B6CB–0x03B6D6`.

Reset at `0x046330` calls `0x0465C0` on the same subobject. It restores level one at `0x0465DB`
and progress zero at `0x0465E5`. The outer reset then sets the progress tracker to zero at
`0x04635F` and the level tracker to one at `0x046369`. Both the optional free path and the path
that skips it reach these stores. Reset makes no grant call. Original exports `00446330.c` and
`004465c0.c`, with the post-free continuation in the image at `0x046352–0x04635F`.

With a negative or out-of-range saved-character index, setup `0x083E70` resets the selected
creature, explicitly sets its level to one and constructs a recipe owner. Appearance/setup
`0x02C080` calls creation `0x0772B0` at `0x02C3D0`. Creation processes equipment and inventory
before it takes the first old recipe node, sets both sentinel links to the sentinel and sets the
recipe count to zero. It follows the saved forward links and frees every old node up to the
sentinel. It then supplies the literal 32-bit argument one and calls `0x07FAE0`. The image at
`0x077778–0x0777BB` recovers the traversal after each free. Original exports `00483e70.c`,
`0042c080.c` and `004772b0.c`.

Calling creation again replaces any prior list with the results of a fresh level-one grant.
Creation requires no level comparison or catch-up loop. Neither its final recipe step nor the
grant routine writes the level tracker. On this new-character path the current level and its
tracker already equal one, so observing the same pair later skips the update grant.

## Saved-character load and save

The wrapper `0x0806C0` takes a numeric character key and a destination creature. It saves the six
32-bit position words, acquires controller critical section `+0x8005B8` and reads the saved
buffer. When a long temporary key requires a free, the image at `0x08083E–0x080844` restores the
saved read-result byte before testing it. A zero result skips the parser and both tracker stores.
A nonzero result admits parsing. The wrapper does not require an independent whole-buffer validity
result before publishing the trackers. Original export `004806c0.c` and image `0x080824–0x0808DA`.

The parser `0x04BE40` resets the creature and installs a fresh recipe owner at `0x04BECA`. It
reads the four-byte progress field and then the four-byte level field, storing them at `0x04C076`
and `0x04C09E` when enough input remains. It replaces the owner again at `0x04C4F4`. This second
owner receives the saved recipe list. The original image confirms that both owner replacements
continue after freeing a previous nonnull owner. Original export `0044be40.c` and image ranges
`0x04BE7F–0x04BED0` and `0x04C4B2–0x04C4FA`.

A complete recipe section supplies a 32-bit count followed by one `0x118`-byte descriptor per
positive-count iteration. The parser appends the rows in input order, including duplicates. It
calls neither recipe admission nor the grant routine. The count defaults to zero if the four
bytes are unavailable. If a positive count was read but a whole row is unavailable, the parser
skips the row copy, advances the cursor to the buffer end and still reaches the append using the
descriptor initialized for that iteration. Its category and subtype are zero, its level is one
and its extension count is zero. This is a local short-read path, with no general guarantee for a
malformed save or a failing allocation. Original image `0x04C693–0x04C7D9`.

After the parser returns, the wrapper restores the position and mirrors it to creature
`+0x1350–+0x1367`. It copies current progress to the progress tracker at `0x0808C8`, then copies
current level to the level tracker at `0x0808D4`, before leaving the critical section. The source
of both stores is the creature as it stands after parsing. An ordinary complete load restores
saved recipes without rerolling the loaded level or backfilling earlier levels. The wrapper is
also called for saved-character list entries, including `0x064FFC` in `0x059C40` and the loop in
`0047ddd0.c`. A valid selected saved-character index in `00483e70.c` calls it directly with the
selected creature as destination.

Save wrapper `0x087520` uses a temporary character and calls serializer `0x04D790` at `0x0876FA`.
The serializer records current progress, current level, the recipe count and the `0x118`-byte
descriptors in forward list order. This format has no fields for the level and progress observation
trackers. Save performs no level grant. The format and call ordering do not establish durability
after a storage error. Original exports `00487520.c` and `0044d790.c`, especially the field reads
at `0x04D882` and `0x04D8A5` and the count/list traversal at `0x04DDBE–0x04DE41`.

## Local and network world entry

Existing-world action `0x084170` and new-world action `0x082530` call local world selection
`0x06F620` at `0x084209` and `0x082939`. Selection updates the numeric and string parts of world
identity at recipe-owner `+0x24` and `+0x28`, and updates the controller's requested world. It
collects and removes other creature-map members, excluding the selected creature by pointer. The
selected creature retains its recipe list and observation trackers across this selection step.
There is no grant in the step. Original exports `0046f620.c`, `00484170.c` and `00482530.c`.

When the controller activity byte is set, disconnect/return `0x0719F0` first calls the selected
character's save wrapper at `0x071A35`, then closes the socket and waits for the thread handles
as applicable, and calls the same world selection at `0x071B35`. The traced return path adds no
tracker reset or grant. Original export `004719f0.c`.

The accepted network-entry path in `0x06FC50` preserves the selected character's base data,
inventory and recipe-owner data, disposes of the old creature-map objects, looks up the received
identifier and constructs a creature when needed. It restores the preserved base and inventory
data, supplies a recipe owner if needed and restores its contents. Owner assignment `0x0686E0`
uses list assignment `0x0571D0` to transfer recipe descriptors in order. This operation generates
no recipe grants. After restoration, the connect path copies current level to the level tracker
at `0x0703EA`, then current progress to the progress tracker at `0x0703FC`. At `0x070408` it copies
the selected pointer to controller `+0x39C`, which is `+0xB8` within the embedded world at
`+0x2E4`. This secondary publication follows the earlier selection through `+0x8006D0`. Original
exports `0046fc50.c`, `004686e0.c` and `004571d0.c`, especially `0x070221–0x07040E`.

These boundaries explain how an already synchronized selected character can enter or reenter
without receiving another grant. The surrounding callbacks, all menu modes, world-generation
worker scheduling and incomplete network receives are outside the established entry behavior.

## Observing a later level change

In the client update routine `0x088EE0`, branches on creature `+0x70` can skip earlier notification
work and still reach the comparison at `0x093D64–0x093D84`. The update compares current level with
the level tracker. Equality goes to the progress check. Any inequality, including a decrease,
enters a notification path. After the call at `0x093E8F` to `0x084350`, the update reloads the
selected creature and its current level and publishes that level to the tracker at `0x093EA0`.
It reloads the selected creature to zero its progress tracker at `0x093EAE`, then reloads the
selection and reads the current 32-bit level again at `0x093EBE`. It passes this last read to the
single grant call at `0x093EC4`. The old tracker never serves as the start of an iteration.
Original image `0x093B0F–0x093B2F` and `0x093D64–0x093ED5`.

With a stable selected creature and level through this sequence, a jump across several levels
grants only the observed destination level. Repeating that observation skips the grant. Returning
to a level after another level was published can grant it again, subject to duplicate admission
for each candidate. A change that reverses before observation can leave both compared values
equal and cause no grant. These consequences of the comparison establish neither which changes
normal gameplay produces nor atomicity across the separate reads. Tracker publication precedes
the grant, including any failure inside the grant.

On the equal-level path, a signed comparison skips progress notification when tracked progress is
greater than or equal to current progress. Otherwise the progress notification eventually copies
current progress into the tracker at `0x094121`. Progress alone supplies no recipe grant. The
level-mismatch path jumps past this progress check after its grant. Original image
`0x093EC9–0x094147`. The parent routine's original function export `00488ee0.c` reports a timeout
and a discovered body of 77,332 bytes. The admission rule applies when this block runs. Its full
scheduler, first eligible frame, cadence, cross-thread exclusion and retry behavior remain open.

## Candidate order, randomness and duplicates

For a positive signed 32-bit level and ordinary player class bytes one through four, `0x07FAE0`
finishes each candidate's membership query and any insertion before advancing. The phases below
specify the draw order without enumerating the complete item or ingredient tables.

| Order | Grant phase | Draws |
| --- | --- | --- |
| 1 | Fixed candidates selected by signed level remainder modulo four | None |
| 2 | Applicable fixed candidate or candidates selected by signed level remainder modulo five | None |
| 3 | Fixed extras with intervening class tests, only when the whole 32-bit level equals one | None |
| 4 | Set the descriptor's low-16-bit level, draw the value for `+0x04`, then select the first randomized candidate by level modulo four and class | One |
| 5 | Change category, draw the new `+0x04` value, then select the second candidate by class and level modulo three | One, with a further draw on the branches below |

Both unconditional draws store the signed remainder modulo eleven in the descriptor's 32-bit
word `+0x04`. Class one draws again for level remainder zero or one modulo three, using the draw's
remainder modulo three to select the subtype. Class three draws again only for level remainder
two modulo three, using the draw's parity to select one of two material values. Classes two and
four have no further draw. The conditional draw occurs after the second unconditional draw and
before the second candidate's membership query. Thus a completed ordinary grant consumes two
or three draws even if every candidate is already present. There is no retry on a duplicate.

The imported function is `rand` from `MSVCR110.dll` at IAT RVA `0x2FC468`. The two unconditional
calls occur at `0x080397` and `0x08049B`. Conditional call sites are `0x0804FE`, `0x080513` and
`0x0805DD`. The grant contains no reseed. Remainder calculations preserve signed 32-bit semantics.
The descriptor's level field contains the low 16 bits of the argument. A larger level whose low
bits equal one does not satisfy the exact-level-one test. Original export `0047fae0.c` and the
three native branch tables at `0x080684`, `0x080694` and `0x0806A4`. The original runtime's random
state, its internal generator and draws in other creation, presentation, networking or world
operations are outside these counts.

Membership query `0x044A90` returns false for a null recipe owner. Otherwise it walks from the
first node to the sentinel and stops at the first equal descriptor. A match leaves the existing
record in place. On a miss the grant appends a new node at the tail, assuming a valid nonnull
owner. The query's null result supplies no owner allocation or recovery for the grant. Original
exports `00444a90.c`, `00449b70.c`, `0044d460.c` and `0047fae0.c`.

Equality `0x02F4A0` compares category, subtype, 32-bit word `+0x04`, byte `+0x0E`, 32-bit word
`+0x08`, byte `+0x0C`, 16-bit level `+0x10`, byte `+0x0D` and 32-bit extension count `+0x114` in
that order. For equal positive counts it visits eight-byte entries starting at `+0x14`. It checks
each entry's fourth byte and then its first three bytes using `0x028940`. The grant template
sets the count to zero. A new randomized value at the same level can distinguish a new candidate
from a known one. The level-one class-three extra receives two consecutive tests with identical
fields. An ordinary first insertion is visible to the second test, which consumes no random
draw and creates no second identical entry. Original exports `0042f4a0.c`, `00428940.c`, grant
template `0x07FAF6–0x07FB4C` and repeated class-three tests `0x0800C6–0x080168`.

## Re-certification and limits

The 2026-11-21 review checked the earlier record against the original image. The exported
instruction bytes matched the image across the grant routine and the cited construction, reset,
load, world-entry and observation blocks. The image also confirmed the branch tables, imported
random function, direct grant calls and continuations after frees in reset, creation and load.
The load description now distinguishes the two owner replacements, complete rows and short-row
appends. The observation description distinguishes the level comparison, publication read and
grant-argument read. The save and disconnect descriptions establish call order and serialization.

This is static evidence for the identified client paths. It establishes no persistent ledger of
completed grant levels. Complete recipe contents, ingredient construction, unrelated learning and
reward paths, the full client update scheduler and general recovery from allocation failures,
malformed extensions, truncated saves, short network receives or invalid creature state remain
outside the contract. Runtime acceptance remains open.
