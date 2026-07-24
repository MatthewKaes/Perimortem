# Compiler

The Compiler is Tetrodotoxin's target product boundary for one build. It
borrows one finalized Package graph backed by Sources or restored from an
archive. It owns only target plans, linker state, and terminal products derived
from that graph. Only completed terminal artifacts escape the boundary.

The compiler consumes the TTX Abstract graph. It does not rebuild that graph as
an ABI Type tree, publication tree, class database, or pointer-keyed
implementation table.

```text
TTX Abstract graph
        |
        v
Type, Layout, Callable, and Dialect queries
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

- representation and ABI plans for the target
- allocation and lowering state
- linker state and terminal products
- diagnostics for target derivation

The finalized Package graph owns semantic Types, Generic results, Callables,
Bodies, linkage promises, and dependency edges. For a Package backed by
Sources, its Environment and every retained Source remain alive until Compiler
finishes. A restored Precompiled Package owns the equivalent graph
independently.

The active toolchain owns its `Dialects` context and targets.
`Parser::Package` owns the root transaction and confined loading; the package
construction walk owns dependency resolution and cache invalidation. TTX owns
the shared Abstract, Type, Layout, and Callable contracts.

## Semantic input

Compilation starts from a finalized Package definition or export reference.
The consumer resolves that object and proves the narrow contract required by
the operation. Failure is an Invalid Abstract. It is never a null Type,
Callable, or linkage pointer. Compiler never observes a graph while bindings,
materializations, signatures, lookup surfaces, or Bodies can still change.

Aliases redirect through `Abstract::resolve()`. Generic objects consume their
accepted arguments and return a concrete Abstract that must resolve to Type.
Neither Alias nor Generic is itself a Type.

A Type supplies its Layout. Structured contains the real Addressables owned by
that Type; Ranged compactly repeats one queried Type. Layout does not copy child
Types, offsets, documentation, attributes, defaults, or target storage into a
compiler record.

## Lowering

Lowering selects behavior by proven contracts:

```text
resolve Abstract
-> prove Type
-> Terminal: read its domain, size, and alignment
-> Structured: recursively lower every Addressable
-> Ranged: recursively lower the repeated Type across its fixed count
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

Callable publishes complete parameter and result Layouts. Static invocation has
no receiver. Self invocation includes its receiver as parameter zero.
Reflection, fitting, call lowering, and generated interfaces all consume that
same complete signature. Machine linkage belongs to a narrower ABI or execution
contract rather than core Callable or Addressable.

An unresolved endpoint is represented by its ABI owner or Invalid. The compiler
does not infer invocation mode from a function name, prepend a receiver later,
or use a null pointer as linkage state.

A target-independent executable body may be attached to a Callable through a
Dialect-owned contract. That real body contract describes execution without a
pointer-keyed implementation table and does not own Type identity, name
resolution, or package publication.

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
