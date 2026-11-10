# Alpha spatial record association production

Status: FINAL / STABLE / HELD after complete author readback.

This bounded original-source audit establishes the four-byte member at `+0x178` of the resident `0x188`-byte spatial records. The closed fresh-construction routes give it raw bits `0xFFFFFFFF`; the closed copies preserve all four source bytes. A concrete native route that first creates a nonnegative association value has **not** been established. Copying an already nonnegative source is supported, but does not establish that source's origin. No descriptor ordinal is assigned to this member by the append and relocation mechanisms audited here.

The supported image is original client `binary/Cube.exe`, image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Evidence paths are relative to `disassembled alpha cube world`; bare function filenames are under `decompiled/client/functions`. Evidence consists of the original image, `decompiled/client/assembly.txt` and the individually identified original functions cited below. No earlier contract, reconstruction or runtime observation is evidence for this document.

## Record identity and the meaning of the width

R denotes a record, not an actor or an arbitrary object having the same displacement. The relevant copy constructor is original RVA `0x18EA10`; the default constructor is `0x18EAA0`. The vector append at `0x1A0E00`, relocation loop at `0x18E8E0` and capacity replacement at `0x1949D0` use an element extent of exactly `0x188`. Their record copies include the member at `+0x178` and the following members through `+0x184`. These relationships establish the storage identity independently of the displacement alone.

The resident consumer in hit application `0x195A60` supplies a further identity check. At `0x19619F` it looks up a spatial container using the actor's two full 32-bit members `+0x1B0/+0x1B4`; a nonnull result supplies a record range from container `+0x0C/+0x10`. The scan advances by `0x188`. It tests record full selector `+0` equal to 5, then compares all 32 bits of R `+0x178` with actor `+0x1B8`, then tests record byte `+0x30` nonzero. This is the scoped record family. [Evidence: native RVAs `0x196193–0x1961DA`; original lookup `00434d10.c`.]

The field's producer and copy width is 32 bits. The constructors use raw `0xFFFFFFFF`, which is −1 when interpreted as signed 32-bit. The local consumer above uses equality, not a signed ordering test or an array subscript from this value. In this document, “nonnegative” means the signed range `0..0x7FFFFFFF` requested by the audit; it is not a proved universal type interpretation. Copying does not narrow, sign-extend, clamp or validate this member.

The vector extent does not establish that every byte of a record is a raw transferable blob. The record copy explicitly handles its subobjects and selected fields. Padding and unlisted byte-copy behavior are not generalized from `0x188`, and a positive integer or float bit pattern at `+0x178` of a different receiver is not a producer of R's member.

## Fresh construction and replacement

| Original operation | Established association result | Relevant admission and ordering |
| --- | --- | --- |
| Default construction `0x18EAA0` | R `+0x178` receives full `0xFFFFFFFF`. | Unconditional on the ordinary valid-destination route. It follows initialization of the embedded object at R `+0x48`; then R `+0x17C` and `+0x180` each receive `0xFFFFFFFF`, and `+0x184` receives zero. |
| Construction `0x1BF5B0` | R `+0x178` receives full `0xFFFFFFFF` and is not replaced by its subsequent selection branches. | The explicit write follows the embedded `+0x48` initialization and precedes copying the supplied position, storing the supplied full value at `+0x20`, and random selection of prefix values. |
| Construction `0x1BFD70` | The same raw `0xFFFFFFFF` result. | The same association initialization precedes its different random prefix-selection alternatives. |
| Construction `0x1BFF10` | The same raw `0xFFFFFFFF` result. | The association initialization precedes the supplied position/value copies and its random selector result. |
| Construct-and-copy operation `0x1F6E20` | Its output record receives full `0xFFFFFFFF` at `+0x178`. | A fresh local record is built, then copied into the first stack-passed output pointer through `0x18EA10` at `0x1F7379`. The local association is explicitly `0xFFFFFFFF` before the random/position branches and is not rewritten by those branches. This operation can replace an incoming output value, rather than preserving it. |

For the first four rows, the association stores are original RVAs `0x18EACB`, `0x1BF5EB`, `0x1BFDAB` and `0x1BFF4A`. Their adjacent tail stores have the same four-byte widths. The embedded initializer `0x03C020` receives R `+0x48`; its ordinary writes end before R `+0x178`. None of these constructors chooses the association from an input identifier, record count, array position or random result. [Evidence: `0058eaa0.c`, `005bf5b0.c`, `005bfd70.c`, `005bff10.c`, `0043c020.c`; original native constructor bodies.]

At `0x1F6E20`, the fresh local association store is `0x1F6E92`. The copied local record begins at the source pointer passed to `0x18EA10` at `0x1F7379`; its `+0x178` is precisely that initialized four-byte slot. The ordinary function returns the output pointer after the copy and temporary cleanup. Its random branches change other record members and positions; no random result becomes the association. This closes this particular construction relation, not the whole generator that calls it. [Evidence: `005f6e20.c`; native RVAs `0x1F6E4E–0x1F6EAE`, `0x1F736A–0x1F73C4`.]

Concrete calls from the large routine `0x1CC510` reach `0x1F6E20` at `0x1D2555`, `0x1D28BA`, `0x1D2C1F` and `0x1D2F8B`, followed respectively by record appends at `0x1D2565`, `0x1D28CA`, `0x1D2C2F` and `0x1D2F9B`. In these reached local sequences, the record just constructed is the append source, so the append preserves its `0xFFFFFFFF` association. All earlier admission, other records built by `0x1CC510`, and complete generator output are not claimed closed. A call-site connection is not a proof over every branch of that large routine.

There are also verified inlined fresh-temporary append boundaries:

| Enclosing routine and local boundary | Explicit fresh association store | Record-vector copy |
| --- | --- | --- |
| `0x1AD320` local candidate route | `0x1AD60D`, full `0xFFFFFFFF` | `0x1AD9F0–0x1AD9F9` copies those four bytes to the destination record `+0x178`. |
| `0x1DC6E0` first inspected local candidate route | `0x1DDE83`, full `0xFFFFFFFF` | `0x1DE23C–0x1DE248` copies the local association. |
| `0x1DC6E0` second inspected local candidate route | `0x1DE3CB`, full `0xFFFFFFFF` | `0x1DE7DE–0x1DE7EA` copies the local association. |
| `0x1F0CE0` inspected local candidate route | `0x1F1803`, full `0xFFFFFFFF` | `0x1F1B0E–0x1F1B1A` copies the local association. |

The source and destination identities in this table were checked using the actual temporary record bases and the `0x188` append extent, not decompiler local-variable numbering. The inspected placement/filling helpers `0x1F4DA0` and `0x1CB3B0` write record prefix members. The former changes position members; the latter writes position and members `+0x20`, `+0x24..+0x2C` and byte `+0x30`. Their reached writes do not replace `+0x178`. They therefore do not supply a nonnegative association to these bounded temporary-copy sites. Their wider terrain-query behavior, all numerical placement outcomes and all transitive generation effects are outside this association contract. [Evidence: scoped regions of `005ad320.c`, `005dc6e0.c`, `005f0ce0.c`; `005f4da0.c`, `005cb3b0.c`; cited original instructions and image bytes.]

## Copying, append order and relocation lifetime

The record copy constructor `0x18EA10` reads the source's entire four-byte `+0x178` at `0x18EA58` and stores it to the destination at `0x18EA61`. It does so after the preceding subobject copies and `+0x170/+0x174` copies, and before copying `+0x17C`, `+0x180` and `+0x184`. The store has no sign, nonnegative, selector, state, count or ordinal gate. The function then returns the destination pointer. Ordinary distinct valid records are the bounded copy case; overlapping/malformed subobjects and allocation failures are not generalized. [Evidence: `0058ea10.c`; complete native routine `0x18EA10–0x18EA94`.]

Thus a valid source already holding 0, 1 or any other nonnegative 32-bit value can carry that value through the copy unchanged; negative bit patterns are equally preserved. This is a concrete propagation capability. None of the fresh-construction rows above supplies such a nonnegative source, and a hypothetical caller-provided value is not promoted into a verified native first producer.

Append `0x1A0E00` uses a three-pointer vector receiver: begin, logical end and capacity end at `+0/+4/+8`. A source in the half-open logical range from begin through just before end is treated as an internal source. If growth is needed, the source is recovered at the same element ordinal in the relocated storage before copying. A source outside that range is treated as external. With ordinary valid storage, the record is copied at the old logical end, including its unchanged association; only after the copy returns does the logical end advance by `0x188`. [Evidence: `005a0e00.c`; native RVAs `0x1A0E25–0x1A0EA1`.]

The source ordinal calculated during that internal-source repair is used to find the source after reallocation. It is not written into R `+0x178`. Likewise, advancing the vector's logical end changes the collection, not the copied association. A newly appended record can therefore retain an association unequal to its destination ordinal, and append does not force uniqueness or renumber existing associations.

Capacity replacement `0x1949D0` allocates storage in multiples of `0x188`, copies the old logical range through `0x18E8E0`, then destroys the old records' embedded storage and releases the old allocation. The ordinary successful tail publishes the new capacity end, logical end and begin, in that order. The relocation loop invokes `0x18EA10` for each record in source order and does not transform the association. Old record addresses cease to be live storage after release; the association's four-byte value survives in the replacement record. [Evidence: `005949d0.c`, `0058e8e0.c`, `0058d970.c`; native RVAs `0x194A31–0x194AA3`, `0x18E920–0x18E94A`.]

The growth admission helper `0x194B40` concerns available capacity and element counts; it does not initialize associations. The reached destruction loop `0x18D970` releases nested record allocations and does not directly reset R `+0x178` before release. These observations do not establish bucket-pointer repair, synchronization or safe continued use of an old record pointer across growth. The record's association value and the lifetime of its address are separate matters.

## State changes and the ordinal distinction at the resident consumer

State-changing helper `0x1A58E0` receives the record pointer. If its requested low-byte state equals record byte `+0x30`, it bypasses the record-change work. If different, the original changes record `+0x34` and byte `+0x30`, and can append presentation output. Its direct record writes do not clear, assign or renumber `+0x178`. In particular, changing the record state to zero through this helper does not itself restore the association to `0xFFFFFFFF`. A future fresh construction into the same storage would be a separate overwrite. [Evidence: `005a58e0.c`; native RVAs `0x1A58FA–0x1A591F` and the remaining bounded body through `0x1A5A05`.]

At the resident scan `0x1961B4–0x196219`, the equality involving R `+0x178` is independent of the scan's ordinal. The scan ordinal starts at zero and advances for every skipped `0x188`-byte record. After a match, a later output stores that ordinal separately; the matching record's association is not overwritten with it. The shown local comparison also has no independent nonnegative test of R `+0x178`; earlier actor/world admission is not replaced with a claim that every raw value is globally reachable.

The coordinate lookup reached by this scan uses two actor members and original helper `0x034D10`, whose ordinary admitted coordinate arguments lie in signed `0..65535`; missing lookup results stop the record scan. This supports a spatial-container relation. It does not prove that the association is a descriptor index, a collection ordinal, an actor identifier, a globally unique key or a derived coordinate. The exact equality test alone cannot supply those semantics.

## Loading and first-nonnegative producer limits

No file or socket decoding route has been closed in this audit that first supplies nonnegative R `+0x178`. No contract is made that a loaded record must have `0xFFFFFFFF`, that the field is absent from every persistence format, or that a loader cannot copy a previously stored nonnegative value. Those would require an identified original decoder, its actual destination-record identity, its complete source-field width and the producer of its input. A copy routine accepting arbitrary four-byte input does not establish those dependencies.

The large generators at `0x1AF170`, `0x1CC510` and `0x1E4850` contain many construction/call sites. This document closes the specific `0x1CC510` call sequences and local boundaries stated above; it does not certify every record or every alias-sensitive write in those generators. The same limitation applies to the portions of `0x1DC6E0/0x1F0CE0` outside the bounded sites. Equal member displacements in actor data, rendering objects, item records or temporary runtime structures were not accepted as association producers.

Accordingly, the first nondefault/nonnegative value, any descriptor-to-record association rule, global exhaustion of possible writers, complete reset/reconstruction policy, loaded-record origin and lifetime across all world transitions remain unresolved. Raw construction/copy/lifetime facts above are independently usable within their stated inputs. No random stream rule, gameplay label, array-ordinal assignment or positive producer may be inferred to fill the gap. Imported allocator callbacks, exceptional storage, concurrency and a server counterpart are also outside this ordinary successful-call boundary. Earlier HELD artifacts remain unopened and unchanged.
