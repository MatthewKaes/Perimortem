# Compiler

The Compiler is Tetrodotoxin's memory and terminal-product boundary for one
build. It owns the semantic objects created during that build and the target
products derived from them. Only completed terminal artifacts escape the
boundary.

The compiler consumes the TTX Abstract graph. It does not rebuild that graph as
an ABI Type tree, publication tree, class database, or pointer-keyed
implementation table.

```text
TTX Abstract graph
        |
        v
Type, Layout, Callable, and ISA queries
        |
        v
target planning and execution
        |
        v
linker and terminal products
```

The canonical language contracts live in
[`../../ttx/ttx_semantics.md`](../../ttx/ttx_semantics.md). Implementation in
this directory that disagrees with those contracts is migration work rather
than design authority.

## Ownership

One Compiler owns:

- the arena and per-build Abstract objects
- source and import edges supplied by its host
- concrete Types produced by Generic instructions
- Callable bodies and linkage facts
- target-independent execution facts
- target plans, linker state, and terminal products
- diagnostics retained by the source-owning system.

The active toolchain owns installed ISA evaluators and targets. Puffer owns
source loading, dependency resolution, and cache invalidation. TTX owns the
shared Abstract, Type, Layout, and Callable contracts.

## Semantic input

Compilation starts from an Abstract reference. The consumer resolves that
object and proves the narrow contract required by the operation. Failure is an
Invalid Abstract. It is never a null Type, Callable, or linkage pointer.

Aliases redirect through `Abstract::resolve()`. Generic objects consume their
accepted arguments and return a concrete Abstract that must resolve to Type.
Neither Alias nor Generic is itself a Type.

A Type supplies its Structured Layout. The Layout contains the real
Addressables owned by that Type. It does not copy child Types, offsets,
documentation, attributes, defaults, or target storage into a compiler record.

## Lowering

Lowering selects behavior by proven contracts:

```text
resolve Abstract
-> prove Type
-> Terminal: read its domain, size, and alignment
-> otherwise: recursively lower every Addressable in its Structured Layout
```

Terminal proof precedes Layout inspection. This preserves the distinction
between a Terminal and a valid zero-entry aggregate. A non-empty composite is
never collapsed into one scalar merely because a backend recognizes its outer
name.

Aggregate offsets, storage size, and aggregate alignment are derived by the
target planner from recursively resolved terminal facts. Register carriers,
calling conventions, stack placement, and wire policy remain target decisions.
They are not source attributes or Type fields.

## Callables

Callable publishes complete parameter and result Layouts plus an address query.
Static invocation has no receiver. Self invocation includes its receiver as
parameter zero. Reflection, fitting, call lowering, and generated interfaces
all consume that same complete signature.

An unresolved endpoint is represented by an unresolved Addressable or Invalid.
The compiler does not infer invocation mode from a function name, prepend a
receiver later, or use a null pointer as linkage state.

Target-independent body tables may be attached to a Callable through an
ISA-owned contract. Those tables describe execution. They do not own Type
identity, name resolution, or package publication.

## Target planning

ABI planning consumes the recursively lowered terminal projection. Caller,
callee, generated interfaces, register allocation, and object emission must use
one shared call plan rather than reclassifying the semantic Type independently.

Assemblers encode decisions already made by the semantic and ABI owners. They
do not decide Type identity, aggregate shape, or calling convention.

If an output needs a durable symbol, its owner walks an explicitly selected
named chain in the Abstract graph and encodes those names reversibly. It does
not allocate a Route object, hash a signature, or choose an alias by sorting
reachable paths.

## Directory ownership

- [`execution`](execution/) owns target-independent operations and SSA facts
  attached to Callable bodies
- [`allocation`](allocation/) owns physical-location planning after terminal
  projection
- [`target`](target/) owns target ABI planning and target-specific interface
  projection
- [`assembler`](assembler/) owns instruction encoding
- [`engine.hpp`](engine.hpp) coordinates the selected target and linker inside
  the Compiler boundary
- [`../linker`](../linker/) owns durable object records.

The existing compiler predates the current Abstract model. Migration should
start at its semantic inputs and proceed toward terminal outputs. Compatibility
adapters must not become a second long-lived TTX model.
