# Compiler

The compiler owns the execution model between an ISA and a selected machine
target. An ISA supplies typed program facts. A target chooses an ABI, assigns
physical storage, encodes instructions, and publishes object records. Neither
side reaches through this boundary to borrow the other's representation.

```text
ISA-owned meaning
-> Compiler::Execution
-> Compiler::Backend
-> Compiler::Target
-> Linker
```

## Directory ownership

- [`execution`](execution/) owns the target-independent function, body, block,
  operation, operand, and SSA-binding data. Each public concept has a matching
  file. The mutable builder freezes those dense tables into an immutable body,
  while the program owner collects completed functions.
- [`allocation`](allocation/) contains target-independent physical-location
  planning. It deals in abstract colors and spill slots, not machine registers.
- [`target`](target/) contains ABI and target lowering. The current host target
  is x86-64 System V.
- [`assembler`](assembler/) contains instruction encoders. Assemblers receive
  physical operands after target policy has been decided.
- [`symbol.hpp`](symbol.hpp) describes a named byte range published by a
  compiler product.
- [`linkage.hpp`](linkage.hpp) joins canonical TTX functions to the symbols
  published by their producers.
- [`backend.hpp`](backend.hpp) is the toolchain-selected lowering contract.
- [`engine.hpp`](engine.hpp) is the front door shared by terminal lowerers.

The root contains coordination interfaces only. Adding another execution
instruction belongs in `Execution`. Adding an x86-64 rule belongs in `Target`,
and adding an encoding belongs in `Assembler`.

`Compiler::Linkage` retains the owning type because `Ttx::Function` has no
owner back pointer and packages encode functions by owner-local index. It does
not contain calling-convention policy. That remains target-owned.

## Execution program

`Execution::Program` is a collection of functions identified by linkage
symbol. Each `Execution::Function` joins four facts:

- its diagnostic source
- its linkage symbol
- the canonical `Ttx::Function` signature
- its immutable `Execution::Body`

The signature remains the owner of parameter and result layouts. The compiler
does not copy those layouts into a parallel type system.

`Execution::Builder` is the mutable construction phase for one body. It
assigns monotonically increasing SSA identities, validates operand types, and
copies the completed tables into the caller's arena. `finish()` returns an
immutable body or `nullptr`; a partial body is never published.

A body is four dense tables:

| Table       | Meaning                                                      |
| ----------- | ------------------------------------------------------------ |
| `Block`     | A range into the operation table.                            |
| `Operation` | An ordered `Binary`, `Call`, or `Return`.                    |
| `Operand`   | An inline `Constant` or an `Addressable`.                    |
| `Binding`   | The canonical TTX type and first live position of an SSA id. |

Operations store ranges when they consume a variable number of operands. A
call therefore stores its symbol, an argument range, and a result-binding
range. The actual operands and bindings remain dense and are shared by every
pass over the body.

An `Addressable` is an SSA id, not a disguised register or pointer. A
`Constant` is a tagged union of `Bits_64`, `Signed_64`, `Real_64`, byte view, or
`Bool`. Its tag is sufficient to materialize the inline data. Semantic types
stay on function signatures, calls, binary operations, and SSA bindings rather
than being copied onto constants. Strings, byte arrays, and embedded files
share the byte-view alternative after their owning frontend has decoded or
loaded them. The null state of `Operand` represents a failed construction step
and is rejected before a body is published.

The current execution model is straight-line. Builder emits one block and
supports integer arithmetic, remainder, equality, calls, and returns. Branches,
block parameters or phi values, ordered comparisons, memory operations, and
address formation require real execution operations before an ISA can lower
them. They must not be simulated with ISA-specific payload bytes.

Builder folds binary operations when both operands are constants. The folded
value remains an inline `Constant`, so `return 2 + 3` publishes only the return
and the value `5`; it never creates an SSA binding or reaches register
allocation. Operations that cannot be evaluated safely, such as integer divide
by zero or signed overflow, remain explicit operations for later validation or
lowering.

## Allocation

`Allocation::Registers` is a conservative first-fit interval-coloring pass for
the current straight-line execution model. It operates in three phases.

First, every SSA binding begins with a one-position half-open interval at its
definition. The allocator walks the dense operation table in source order and
extends the referenced binding through each binary operand, call argument, or
return value. The interval is conservative at operation granularity. It does
not pretend that target instruction scheduling or sub-instruction kills are
already known.

Second, bindings are visited in their stable definition order. For each value,
the allocator scans abstract colors from zero upward and chooses the first
consecutive range that does not overlap both the lifetime and color range of an
earlier value. A value can require more than one color. This is how a target
describes a compound ABI value such as a byte view represented by pointer and
length without teaching the allocator about that semantic type.

Finally, a value that cannot fit receives consecutive spill slots. Spill slots
are monotonic and are not reused, even when spilled lifetimes do not overlap.
The result remains target-independent. `Target::SystemV` maps colors to its
preserved registers and maps spill slots to stack offsets.

This algorithm is intentionally simple. It is deterministic, easy to inspect,
and well matched to the short, single-block functions the execution model can
currently express. Its worst-case work grows with the square of the binding
count multiplied by the available color count because every candidate may be
checked against every earlier binding. Definition order can also spill a value
that a pressure-aware ordering would keep in registers, and consecutive
multi-color values can spill when free colors are fragmented.

Control flow changes the tradeoff. Once Execution gains branches and loops,
the allocator needs block-level use and definition sets followed by backwards
liveness dataflow. A linear-scan allocator with an active interval set is the
natural next step if compilation speed and compact implementation remain the
priority. Interval splitting, spill-slot reuse, rematerialization, move
affinity, register classes, call-clobber constraints, and a target cost model
should be added only when their owning execution and ABI facts exist. Graph
coloring is an option if generated code quality eventually justifies its
greater implementation and compile-time cost. None of those policies should be
hidden in `Target::SystemV` or inferred from TTX types by the assembler.

## Backend and engine

`Backend` is a pair of continuations selected when a `Toolchain` is built:

- lower an `Execution::Program` into linker records
- produce the generated host-language interface header

`Engine` owns one program and one linker transaction. Lowerers add execution
functions or named read-only byte products. `build_archive()` and
`build_header()` return their products directly. Engine does not retain a
second cached copy of either result.

Read-only byte publication is the narrow path used by non-host execution
products. The producer supplies the bytes and named ranges, while Engine owns
the conversion to linker sections and symbols. A producer never sees linker
section ids.

## x86-64 System V

`Target::SystemV` is the current host ABI and machine-code backend. The
standard toolchain pairs it with `Target::Cpp`, which independently emits the
C++ declarations used to call published symbols. System V does not own C++
namespace, spelling, or formatting policy.

The System V backend:

- maps integer, signed integer, boolean, and real values to one component and
  `View::Bytes` to two components
- uses the six integer and eight SSE argument registers before placing complete
  arguments on the stack
- allocates preserved registers for SSA values and stack slots for spills
- lowers integer add, subtract, multiply, signed or unsigned division,
  remainder, equality, calls, and returns
- returns up to two integer or SSE components in the System V result registers
- writes code, constants, symbols, and relocations through `Linker`

`Target::Cpp` emits declarations for functions carrying the supported C++
interface attributes. This is a host-language artifact and remains separate
from System V ABI lowering.

The target owns these choices. `Execution::Call` retains the canonical function
signature and linkage symbol. It does not know the System V argument registers.
The x86-64 assembler knows how to encode a move or call; it does not decide
which SSA value belongs in a register.

Results larger than two eight-byte components require the System V hidden
result pointer convention. That memory-class path is not implemented yet and
is rejected before object publication rather than being assigned an invented
register representation.

## Invariants

- TTX objects retain semantic identity throughout lowering.
- Compiler tables are dense, immutable views after construction.
- ISA implementations do not name registers, stack slots, opcodes, sections,
  or relocations.
- Targets do not reinterpret ISA-specific byte payloads.
- Assemblers encode decisions without making semantic or ABI decisions.
- Derived archives and headers are return values, not cached builder state.
