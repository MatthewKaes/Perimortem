# Library

Library owns CPU language semantics and the reusable native CPU compilation
path. The complete folder builds as `//tetrodotoxin:library`.

TTX provides the shared Abstract, Type, Value, Layout, Addressable, Callable,
Attribute, and Documentation contracts. Library adds only the capabilities and
value domains required by CPU executable languages.

## Library language

`Tetrodotoxin::Library::Language` currently declares the following contracts.

1. Expression, Binding, and Projection provide Library value identities.
2. Constant provides bytes, flag, real, signed, and unsigned value domains.
3. Generic provides Access, View, and Fixed materializations.
4. Concrete Bool, signed, unsigned, and real Types provide scalar identities.
5. Static and Self distinguish Callable invocation.

These classes retain and expose real TTX Type, Layout, Addressable, and Callable
edges. They do not copy those shared contracts into a Library model.

Generic is Library language semantics rather than a universal TTX category.
The formula and concrete Access, View, and Fixed Type shapes are present. The
shared Materializations implementation is not active, so materialization is not
yet an executable Library contract.

Static identifies a Callable selected without a receiver. Self identifies a
Callable selected through an addressable value and reserves parameter zero for
that receiver. Those invocation distinctions are not required by Package,
Render, or every other TTX host.

The concrete scalar Types live here because their names and native
representations are Library language policy. TTX retains only the common Value,
Flag, Real, Signed, and Unsigned domain contracts.

`Language::Function` is the concrete Library defined Static Callable. Its
Arena stable identity is reserved from the authored visibility, `func`, and
name prefix. Until its signature is complete it resolves to the shared TTX
Invalid object. One successful completion installs real parameter and result
Layouts on that same Function and consumes the required balanced definition
body without retaining a Cursor, Token range, source replay record, or
executable Body.

`Language::Parser::Layout` owns the reusable signature grammar. Empty, direct,
unnamed, and named shapes construct real TTX Layouts in authored order. Direct
and unnamed entries retain resolved Type identities. Named entries retain real
Arena owned Alias edges whose resolution reaches those same Types. Qualified
Type routes are resolved by the source local Abstract context supplied by the
Monograph transaction.

## Library Dialect and Monograph

The top level Library Dialect installs into `Environment::Workspace` through the
common `Language::Dialect` interface. Each Workspace owns a distinct stateful
Dialect while Bool, integer, real, and Void remain immutable binary wide
identities. Every Library Monograph resolves those same identities through the
Dialect without publishing them as authored declarations.

The concrete Library Monograph owns exact local Function lookup and an authored
order view of its public Functions. Local lookup also admits private Functions.
Each declaration is reserved and bound at its final Arena address before its
signature completes, so completion never replaces the identity already visible
through the Monograph. Duplicate declarations fail before either view changes.
Missing names resolve to the shared TTX Invalid identity.

Interpretation admits ordinary public and private Function definitions. It
constructs real TTX and Library Language identities in the Environment Arena
and retains no Cursor, Token range, declaration mirror, signature snapshot, or
source replay state. Unsupported declarations and malformed, incomplete,
bodyless, duplicate, or trailing syntax reject the complete transaction.

The exact executable body representation is not implemented. Function body
consumption proves only that one structurally balanced definition exists. The
current target has no authored imports, active executable Body parser, or
semantic compiler transaction.

The intended first application pressure target is
[`../../apps/ttx/echo`](../../apps/ttx/echo/). Its Library source requires
authored `using` expansion with duplicate detection, inferred byte Constants,
dynamic terminal byte input and output, byte comparison and concatenation,
loop and conditional Bodies, and one Static Callable selected by App. These
requirements belong to the future Library Dialect and compiler. The fixture's
remaining syntax contradictions must be settled before it becomes an
acceptance oracle.

## Shared CPU compilation

Library compilation may consume completed CPU executable facts retained by
Library, App, or Scene Monographs:

```text
Library Callables -> Library compiler
Scene lifecycle and helper Callables -> Library compiler
App entry and lifecycle driver facts -> Library compiler
Library compiler -> Linker input
```

App and Scene remain their own semantic owners. Library does not convert them
into Library source, reopen their Tokens, or clone their Type graph.

Compilation owns CPU target and ABI planning. It derives sizes, alignments,
offsets, and carriers, makes calling convention and register allocation
decisions, lowers completed executable facts, emits native instructions and
relocations, and diagnoses target decisions.

It does not own package acquisition, Environment lifetime, App or Scene policy,
Shader lowering, final object encoding, or archive assembly.

The accepted native output of compilation is one or more
`Linker::Object::Module` values. Each Module owns a coherent set of sections,
symbols, and relocations. The compiler does not place semantic facts or source
identity in that native terminal.

## Assembler and Linker boundary

The active `Library::Assembler::x86_64` encodes source independent instruction
decisions. Its tests cover instruction bytes and relocation slot shape.

The assembler does not decide Type identity, aggregate shape, calling
convention, lifecycle policy, or publication. A future compiler supplies those
decisions.

Linker owns source independent objects, symbols, relocations, ELF encoding, and
native archive construction. Library produces Linker input but does not absorb
that terminal artifact owner.

## Source free payload

Library owns the opaque payload needed to restore its durable declarations,
Types, values, Callables, Bodies, publication facts, and native symbol locators.
It implements the shared Language persistence dispatch without introducing a
second restored model.

Package owns Archive framing and never learns the Library schema. A fresh
Workspace asks the installed Library Dialect to restore real Library and TTX
identities into its Arena. Source bytes, Cursors, process addresses, compiler
caches, and machine code are excluded.

No Library payload encoder or restorer exists in the current target.

## Current boundary

The current Library target contains the installed Dialect, declaration
Monograph, binary wide scalar and Void Types, Function signature construction,
reusable Layout grammar, and the x86_64 assembler.

It does not yet contain authored imports, executable body ownership, CPU target
planning, semantic lowering, or App and Scene integration.
