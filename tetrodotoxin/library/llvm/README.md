# Library LLVM compiler

Library compiles one completed graph into LLVM IR, an ELF object, and a matching
C header. The compiler borrows the real Monograph, exact authored source facts,
and caller owned source error sink for one request. LLVM objects and target maps
never escape that request. Puffer is one host of this interface and owns none of
its lowering behavior.

The public compilation boundary is [`compiler.hpp`](compiler.hpp). Internal
lowering headers expose only the opaque handles from `llvm-c/Types.h`. LLVM C
operations, C++ headers, libraries, and target details remain private to the
Library compiler implementation.

## Target and SDK

The contained SDK comes from [`//toolchain/llvm:sdk`](../../toolchain/llvm). The
repository rule verifies matching LLVM packages, public headers, and runtime
version before compilation. Each request also compares the linked runtime with
the header version.

The current target contract is 64 bit x86 Linux with the ELF System V ABI,
baseline x86 instructions, position independent code, the small code model, and the
DataLayout supplied by LLVM. Library does not infer these facts from the build
host.

## Compilation transaction

Compilation proceeds through one sequence local to the request:

* Configure the target
* Plan semantic identities and physical carriers
* Emit functions and Static lifetime operations
* Verify the LLVM module
* Render review IR
* Emit the ELF object
* Render the matching C header
* Publish all returned bytes in the caller Arena

Planning completes before instruction emission. Exact Library Types, Fields,
Callables, Packs, and control owners remain the keys for physical maps. These
maps record LLVM Types, values, addresses, functions, blocks, and debug scopes.
They are not a second semantic graph.

Builder is concrete. Carrier, Callable, and Static facts are composed state
owners, and no Target inheritance hierarchy mediates their operations. The
compatibility `//tetrodotoxin:library_llvm` target depends on the complete
`//tetrodotoxin:library` owner rather than creating a second implementation
archive.

Unknown completed Library owners indicate missing backend coverage. They do not
become generic target nodes or trigger a fallback compiler.

## Values and control flow

Values are lowered using the TTX Layout system. Value flow follows each Pack's
real Produced edges and performs physical transformations at the receiving
seams. Constant representations are used when available while dynamic
expressions continue through their concrete Library owners.

* Index establishes a scalar or complete ranged write target before storing
  any value.
* Slice performs safe reads and supplies the element Type default for each
  missing position.
* Assignment uses the receiving Expression write contract and does not inspect
  concrete target kinds.

Target layout comes from LLVM Types and the configured DataLayout. A semantic
Library Type does not imply one synonymous LLVM Type.

Blocks retain source order. Branch, loop, Match, Return, and LoopControl lower
from their exact Statement roots. Target cleanup is emitted before control leaves
a storage scope.

## Calls and native publication

Function planning fixes one matching declaration and call signature. Published C
interfaces apply the SysV `sret` and `byval` carriers required for large
aggregates. The generated C declarations use that same classification, while
separately compiled native consumers and runtime assertions prove the actual
boundary behavior.

Calls between TTX Functions require no ABI Attribute. They retain direct target
aggregate carriers and may use a private backend convention. `@abi("C")`
carries real boundary information even though both paths target the same
machine.

`@abi("C")` requests an externally published C interface. The compiler
generates a readable name unless `@symbol` requests an exact external spelling. Generated
Callable names use `TTX_FUNC_`, `__` between nested hosts, and `_self` or
`_static` for the receiver role. Static Addressables use `TTX_ADDR_` and the same
host path. Filesystem paths never enter these names.

Examples include:

```text
TTX_FUNC_Pair__sum_self
TTX_FUNC_small_static
TTX_ADDR_dynamic
```

Source bytes outside the alphanumeric set are escaped so the separator remains
unambiguous. Duplicate external names reject the source before object emission.

## Owned target values

Option values store one inline payload slot followed by their selected state,
matching `Perimortem::Core::Option`. The payload is live only while the state is
selected. Copies and destruction therefore apply to that payload conditionally,
while Option itself adds no allocation, reference count, or helper interface.

Object values use one nonnull payload pointer. Perimortem stores the payload
locally to one worker and retains it with a reference count. Parameters borrow
that handle and results transfer one reservation. Copies retain it, replacement
and scope exit release it, and the final release runs the generated payload
destruction in reverse Field order.
The C header keeps the payload opaque and publishes the generic retain and
release entries for hosts that preserve a returned handle. Objects and writable
Access values never cross workers. A read only View may borrow through a call
whose owner guarantees its lifetime, and retained data is copied.

Inline carriers must have finite storage. An Option can contain an Object
reference recursively, but it cannot make a Structure contain itself by value.
The backend rejects that recursive inline carrier with a source diagnostic.

Dynamic Static initialization follows source order in one generated constructor.
Destruction releases owned values in reverse order.

## Debug information

Debug modes are `none`, `line`, and `full`. Debug selection changes correlation
only and never changes runtime behavior.

Line and full modes use the retained diagnostic path as the compilation unit
path, an empty compilation directory, and a checksum of the retained source
bytes. Full mode describes scalar, array, pointer, and structure carriers with
the stock C11 DWARF language so LLDB can use its supported physical type system.
Readable semantic names and generated linkage names remain separate.

## Diagnostics

Source failures are reported through the request's exact source facts and error
sink, preserving authored ranges without retaining the operation Cursor.
Request selection errors go to stderr. LLVM verifier and target failures use
the configured process Log.

A failed request returns no product bytes. A command line host stages the three
products before renaming them to their requested paths.

## Validation

The ordinary validation binary owns two complete native cases:

* [`runtime.ttx`](../../../validation/data/ttx/llvm/runtime.ttx) and
  [`runtime_harness.c`](../../../validation/data/ttx/llvm/runtime_harness.c) cover
  value flow, calls, control flow, safe access, constant aggregates, Static
  lifetime, generated names, debug data, and logging owned by TTX.
* [`foreign.ttx`](../../../validation/data/ttx/library/foreign.ttx) and
  [`foreign_harness.c`](../../../validation/data/ttx/llvm/foreign_harness.c) cover
  imported State, Foreign calls, generated C carriers, and an exact symbol
  override.

[`llvm.cpp`](../../../validation/unit_tests/puffer/llvm.cpp) executes both cases,
checks reproducible products and source path debug correlation, and verifies
rejection diagnostics owned by the backend. Shell scripts and a separate file
for every error are not part of this acceptance surface.

The direct request still owns one standalone Library Monograph. Ordinary native
calls between Package members require a Package compilation request that selects
the completed member set, assigns linkage qualified by Package, and propagates the
dependency objects. That is a Package publication gap rather than an unresolved
value representation or reason to weaken the standalone backend.
