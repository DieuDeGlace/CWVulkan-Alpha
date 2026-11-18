# Alpha generation worker floating-point controls

**FINAL / STABLE / HELD**

This contract records the original worker creation paths, visible floating-point
control operations and the limits on determining their runtime state. It uses
the following original files from the Alpha evidence tree.

| Image | SHA-256 |
| --- | --- |
| `binary/Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |
| `game-files/msvcr110.dll` | `b30160e759115e24425b9bcdf606ef6ebce4657487525ede7f1ac40b90ff7e49` |

C and S addresses below are client and dedicated RVAs. R addresses are RVAs
in this bundled runtime DLL, whose version resource reports 11.00.51106.1.
The EXE import tables name `MSVCR110.dll`. Selection of this exact DLL
during a live launch remains unverified.

The client PE entry point is C `0x28e1e0`. Its process initialization
invokes C `0x28e87e` through C `0x28e17b`, before the application
entry call at C `0x28e307`. The dedicated PE entry point is
S `0x14b44b`. Its initializer invokes S `0x14b8cb` at
S `0x14b281`, before application entry at S `0x14b3de`.
Both initializer helpers call the imported `_controlfp_s` with a null
result pointer, new-control value `0x00010000` and mask `0x00030000`
(C `0x28e87e` through `0x28e891`, S `0x14b8cb` through
`0x14b8de`).

In the checked runtime, `_controlfp_s` at R `0x027f57` delegates this
request to the control routine at R `0x027df9`. It retains the current
x87 controls outside the requested mask. The native-to-x87 mapping at
R `0x0a6fbb` through `0x0a6fd3` maps the requested value to x87
precision bits `0x0200`, selecting a 53-bit significand. The change is
loaded at R `0x05dd39` when needed. The request preserves the x87
rounding bits, exception masks and other control-word fields.

The same runtime masks its SSE control update with `0x0308031f` at
R `0x027e9b`. Its intersection with this precision-only request is zero.
Thus this initializer request preserves MXCSR. It establishes no requested
MXCSR rounding mode, flush-to-zero setting or denormals-are-zero setting.
The relevant hardware bits are MXCSR `0x6000` for rounding,
`0x8000` for flush-to-zero and `0x0040` for denormals-are-zero
(R `0x027e8e` through `0x027f20`).

The client creates its preparation worker at C `0x0635ec`. Its callable
uses the table at C `0x3029c4`, whose invocation entry is
C `0x06dba0`. That entry adjusts the callable pointer and transfers to
C `0x06a8a0`. The loop calls the root zone generator at
C `0x1e4850` through C `0x06acd1`, and repeats through
C `0x06b70f` while its running flag remains set
(callable construction C `0x051100`, worker construction
C `0x0635bf` through `0x063600`).

The dedicated creates its generation worker through S `0x149f9d`.
Its callable table at S `0x173d44` invokes S `0x149b90`, which
adjusts the callable pointer and transfers to S `0x149550`. That loop
calls the root zone generator at S `0x118630` through S `0x1497d4`
and repeats through S `0x149af8` while its running flag remains set.

Both paths use an EXE wrapper around the imported `_beginthread`.
The client wrapper at C `0x050e70` passes application start
C `0x050e00`, stack-size argument zero and its copied callable payload.
The dedicated wrapper at S `0x013640` passes application start
S `0x0135d0` with the same argument arrangement
(C `0x050eda` through `0x050ee8`, S `0x0136aa` through
`0x0136b8`). Each application start invokes the callable through its
table, then releases the payload and ends the thread.

In the checked runtime, `_beginthread` at R `0x0775c8` allocates and
initializes thread data. It stores the application callback and argument,
then calls imported `CreateThread` with its own entry R `0x0776ef`
and creation flag 4. It records the returned handle and calls
`ResumeThread` before returning to the creator
(R `0x0775eb` through `0x077665`).

The runtime's thread-attach path recognizes reason 2 at R `0x0100b5`.
It obtains or creates the per-thread data slot and initializes that data
through R `0x010476`. Its final hook at R `0x010090` returns
directly. This checked thread-attach route contains no explicit x87
control-word load or MXCSR load
(R `0x0100b5` through `0x01011f`, R `0x010476` through
`0x010510`).

The runtime thread entry R `0x0776ef` obtains the thread data slot. When
one already exists, it copies the callback, argument and handle from the
creation data into it, then releases the creation data. It proceeds to
R `0x077667`, which calls the stored application callback with the stored
argument. The checked callback route contains no explicit x87 precision,
rounding or MXCSR initialization
(R `0x0776ef` through `0x077745`, R `0x077667` through
`0x077687`). These operations carry application and CRT thread data.
The traced payload copying carries no saved x87 control word or MXCSR.

The checked client and dedicated worker bodies and their root zone-generator
bodies contain no explicit floating-point baseline control write. The root
generator bounds are C `0x1e4850` through `0x1ee0b8` and
S `0x118630` through `0x121e98`. Entry into a generation iteration
therefore supplies no native directive in these bodies to select precision,
rounding, flush-to-zero or denormals-are-zero.

Some other original EXE routines temporarily select x87 truncation for
integer conversion. For example, C `0x1fc0da` saves the current control
word, C `0x1fc11b` loads that word with `0x0c00` ORed into it,
and C `0x1fc128` restores the saved word. The dedicated counterpart is
S `0x012d0a`, `0x012d4b` and `0x012d58`. These sequences
preserve the entry precision bits and restore the entry rounding controls
on their ordinary return paths. Their existence establishes a local
conversion operation.

The bundled runtime also exports `_fpreset` at R `0x0a6f17`. Its body
performs x87 initialization, calls the runtime's 53-bit precision request
at R `0x077e12`, and requests MXCSR `0x00001f80` through the
runtime SSE setter. In the applicable SSE branch that MXCSR value selects
round-to-nearest and clears flush-to-zero and denormals-are-zero
(R `0x0a6f17` through `0x0a6f4c`, R `0x0bd19b` through
`0x0bd210`). Invocation of this export for the generation workers
remains unverified.

The original-byte evidence closes the process precision request and the
creation, CRT entry and generation-loop paths above. It leaves the operating
system's new-thread floating-point context and the effects of other loaded
DLL thread-attach callbacks unresolved. A live observation is required to
establish the worker's actual x87 control word and MXCSR at application
callback entry and immediately before its first root generation call.

Those observations need the x87 precision, rounding and exception-mask fields,
and MXCSR rounding, exception masks, flush-to-zero and denormals-are-zero.
The original EXE process initializer alone does not establish those worker
values. Persistence across nested generation callees and repeated iterations
also remains unverified. The complete transitive effects of CRT math calls,
other imported calls and exception paths remain open.
