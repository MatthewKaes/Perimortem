# Tetrodotoxin Design

Tetrodotoxin is the Dialect and toolchain host for TTX source IR. TTX supplies the
human-authored source format, token bytecode, and shared Abstract query model.
Tetrodotoxin decides which instruction sets execute that bytecode and which
terminal artifacts are emitted. Puffer, as Tetrodotoxin's command-line host,
owns complete source-file preambles, source loading, package resolution, and the
source cache.

TTX does not have a canonical ISA or canonical ISA set. A host can install
whatever ISAs it understands. Puffer's standard composition installs the body
ISAs used by Perimortem: Package, Library, Shader, Render, and future authoring
spaces as they become real. Puffer owns Boot as its source-file preamble ISA.

## Boot

Boot is Puffer's preamble Dialect for complete TTX source files. It is selected
directly by systems that know they are starting from a full source file. Boot
can hand the remaining cursor to the authored body Dialect after resolution.

Boot has an intentionally small instruction set:

- read source documentation
- execute the `dialect : Name;` instruction
- collect imports
- validate that requested dialect names resolve to installed Dialects

That is the minimum preamble Puffer needs to attach source bytes to the Dialect
model. Boot does not own package loading, type binding, lowering, or backend
output. It leaves the cursor positioned at the body bytecode so resolution and
the selected Dialect can continue the execution.

## Dialects

An ISA is an executable semantic instruction set for a TTX token stream. The
lexer has already assigned each token a bytecode class, but the ISA decides how
many tokens to fetch, what instruction shape those tokens form, and which TTX
facts to make queryable.

An authored dialect names a `Dialect` Abstract. A CLI, LSP, test harness,
embedded runtime, or package-local resolver provides a `Ttx::Model::Group`
named `Dialects` containing the Dialects it intends to support. The name in the
source `dialect` instruction is therefore not a global enum or registry lookup.
It is an ordinary `resolve_context(View::Bytes)` query.

A Dialect is a reusable toolchain building block, not a compiler phase. Its
caller owns the Cursor, construction arena, diagnostics, and the TTX Abstract
context or contexts participating in one execution. A Dialect can construct a
new Abstract DAG, enrich an existing DAG, or hand the continuation to another
named Dialect. Alias and Group are ordinary Dialects selected by Defines.
Package is an ordinary Dialect selected by Boot. “Subdialect” describes one
handoff relationship and never denotes another contract hierarchy.

The inputs and outputs remain TTX Abstract graphs throughout the toolchain. A
host may compose multiple DAGs containing language facts, engine reflection,
target behavior, documentation, or other tool context. This is not a fixed
lexing-to-machine-IR pipeline. Compilation is one consumer of the unified query
model alongside editing, inspection, runtime reflection, and game-engine
execution.

Shader can expose Stage Types and terminal GPU facts. Library can attach
executable bodies and host-callable address contracts. Another language runtime
can provide its own Type or Callable implementations. These objects enrich one
or more Abstract graphs supplied to the composition. The Group only selects a
Dialect and never becomes a semantic class registry or a second dialect
container.

## Abstract Extension And Foreign Boundaries

Tetrodotoxin has no ClassDB. A central repository of classes, ancestry,
operations, or schema routes would duplicate the Abstract graph and force every
language or ISA through one authority.

Reflection, schema description, dynamic configuration, and language extension
are themselves Abstract concepts. The subsystem that needs one provides an
Abstract context whose ordinary `resolve_context(View::Bytes)` query exposes
its named children. It may use a table, generated index, language runtime, or
another optimized representation internally. No storage or lookup strategy is
part of the Abstract contract.

Native C++ implementations use the local semantic inheritance hierarchy.
C++ RTTI, vtables, and object layouts must not cross a language boundary. A
future foreign ABI therefore needs non-null opaque object handles and explicit
versioned operations corresponding to the narrow Abstract, Type, Layout, and
Callable contracts. Failed queries designate Invalid. The boundary may expose
its own schema Abstracts for configuration, but those schemas are participants
in resolution rather than entries in a mandatory global database.

The exact foreign contract-proof mechanism is not defined by this slice. It
must be designed from the required cross-language operations rather than by
serializing C++ inheritance or reintroducing a universal Type switch.

## Puffer Resolution

The resolver is not a Dialect. It is the source loading and cache-validity
layer between Puffer Boot and body evaluation.

After Puffer Boot evaluates the preamble, resolution loads the requested import
closure, resolves packages, checks imported source files declare the expected
ISA, and binds each import to the local name written in source. Once imports are
available, the resolver gives the remaining bytecode and the imported Abstract
context to the selected Dialect.

Boot's authored dialect name is resolved to an installed `Dialect`. A source
record may reserve stable objects while its imports and body facts are still
being assembled. Queries whose facts are not ready resolve to Invalid. There is
no publication bit or Incomplete Layout between construction and resolution.
File imports are limited by the resolver's compact project-root table. Compiled
packages use the separate package repository rather than carrying filesystem
roots on every record.

The resolver also owns cache safety. It decides whether to enrich a stable
source system, replace an invalidated closure, or rebuild a complete Compiler
boundary. Every consumer that may retain borrowed references participates in
that lifetime policy. Durable package identity remains its authored name and
explicit version, not a process address or content hash. That dependency graph
is a Tetrodotoxin concern, not a TTX language feature.

## Packages

Packages are Tetrodotoxin's module boundary. A package source is evaluated by
the Package Dialect and can expose package exports as TTX facts. Private files
under the package subtree are not imported directly by outside source. Outside source
imports the package by name, then queries exported Abstracts by contract through
the package surface. Type queries are one view of that graph. Tooling may
query Callable, Alias, or ISA-specific contracts through the same root.

Built-in standard package sources live under `tetrodotoxin/standard`. They are
resolved by public package name, not by asking user source to import their
private files. This keeps the standard TTX ABI layer distinct from the current
C++ engine implementation while the graphics/runtime stack is not fully
self-hosted.

The Package Dialect produces a `Tetrodotoxin::Model::Source`. Source is the
durable named root for one evaluated source unit and owns its definition
`Ttx::Model::Group` by value. It delegates ordinary name queries to that Group
but does not implement Type or fabricate an empty Layout. Its children retain
their real Alias, Type, Callable, or extended contracts. While evaluating a
body, `Ttx::Model::Scope` composes the current local Group with the supplied
outer context.

Package composes the reusable Defines Dialect with `expose` as its accepted
sigil and Alias and Group as its continuation Dialects. Defines consumes
documentation, the sigil, a Type or Addressable name, and `:`, then gives the
suffix and a transient Definition Abstract to the selected ordinary Dialect.
Alias resolves and constructs a real TTX Alias. Group hands its nested body back
to the same Defines Dialect and constructs a real TTX Group. There is no special
second child-Dialect type or package-specific Export syntax record.

A Source may replace Group with another lookup representation only when its
domain earns that policy through a concrete contract. Completed declarations
resolve before an unchanged name is delegated to the outer context. A
successful body publishes one durable Source root. That keeps private package
files, package imports, and cache invalidation local to the package while still
allowing package dependencies to become explicit edges in the outer source
graph.

Source does not own an untyped linkage vector. Relationships are the typed
edges of the Abstract graph. Alias retains its target, Structured Layout retains
its Addressables, Callable retains its address, and future executable or shader
contracts retain their own facts. This is the graph the Archiver must restore.
Adding a side table for relationships would make the serialized package more
complete than the live model and recreate the old split authority.

## Body Evaluation

After Puffer Boot and resolution, the selected Dialect executes the rest of the
token bytecode. The Package Dialect exposes package exports. Library can
construct Type and Callable objects plus executable-body contracts. Shader and
Render can expose stage, Layout, binding, and terminal facts.

The ISA is the semantic instruction set that a Dialect implements, not a
parallel software hierarchy. Evaluation is more specific than parsing. It
executes TTX token bytecode and enriches the shared TTX model with facts owned
by that authoring space.

Shared authored forms remain narrow evaluators owned by the concept they
construct. There is no universal graph-construction context. Imports are the
starting Abstract context, Generic owns parameterization, Callable and
Addressable objects own implementation facts, and the source or Compiler owns
memory. Library, Shader, Scene, App, and future Dialects may compose shared
evaluation code while retaining their own legality rules. A pointer-keyed
`Implementation` table must not become a second semantic authority beside the
graph.

## Outputs

The Compiler is the memory and terminal-product boundary for one build. It
owns:

- the arena and every instantiated Abstract object
- imported graph edges and source-owned resolution contexts
- Generic instantiations and Layouts
- Callable bodies, Addressables, and linkage objects
- diagnostics, while semantic failures reference the binary-wide Invalid
- target-independent execution facts
- linker state, generated interfaces, and other terminal products.

Dialects construct Abstract-derived objects inside that boundary and
enrich them with their own contracts. Library may attach a target-independent
execution body to a Callable. Foreign may attach an external Addressable. Shader
may attach stage and GPU terminal facts. The compiler consumes these contracts
without switching on the producer ISA and without reconstructing ownership from
pointer-keyed side tables.

Lower compiler layers operate on narrow interfaces. They resolve an Abstract,
prove Type, and obtain its Structured Layout. Non-empty Structured Layouts are
recursively deconstructed through their actual Addressables in declaration
order. Empty Structured Layouts are lowered through a terminal contract
contributed by the selected target or ISA. The same recursive projection drives
parameters, results, registers, stack placement, generated host declarations,
and durable archive descriptions.

```text
Abstract::resolve()
-> prove Type
-> Type::get_layout()
-> aggregate: resolve each Addressable and recursively lower its Type
-> terminal: invoke selected terminal Type contract
```

An authored `@abi` scalar, a C++ type switch, or an outer-Type shortcut is not a
terminal type fact. In particular, a composite with a non-empty Layout cannot
be laundered into one scalar because a backend recognizes its name. A view,
struct, vector, render contract, or foreign carrier remains one semantic value
whose terminal representation is the ordered projection of its entries.

Static and Self Callable objects expose their complete parameter and result
Layouts. Self includes its receiver at parameter zero. The compiler never
prepends that receiver a second time. A resolved implementation supplies an
Addressable object. Unresolved linkage supplies an explicit unresolved Addressable or
Invalid, not `nullptr`.

If an output needs symbols, its owner walks the selected named resolution
contexts and encodes those names reversibly. It preserves the Static or Self
surface selected during lookup, does not invent `.Type` and `.Addressable`
publishing paths, does not choose a lexicographically preferred alias, and does
not hash a path or signature. ABI and package versions, when needed, are
explicit name components.

Only terminal artifacts escape the Compiler boundary: machine objects,
archives, generated language interfaces, SPIR-V modules, or another explicitly
owned output. Semantic objects and local handles do not outlive their owner.

## Application runtime boundary

App and Scene are host-execution ISAs. Puffer resolves and compiles their
sources, but the resulting application runtime is not a Puffer-owned graphics
model. The runtime owns scene storage and lifecycle, receives a Graphics
presentation target, and submits Render-typed scene values through the
language-neutral Graphics interface.

Render and Shader contribute different facts to that interface. Render owns the
value layout, constants, push constants, resources, and stage contracts. Shader
owns an implementation of those stages and may contribute GPU modules such as
SPIR-V. Terminal lowering must preserve their TTX identities and emit compiled
layout projections so runtime code can bind value storage by offsets and ranges
rather than rediscovering fields by name.

Scene lowering can then identify Addressables whose resolved Types are Render
contracts and expose where those values live in scene storage. A compiled C++
application, a Puffer-hosted application, or another language host can feed the
same frame transaction to Graphics because the transaction contains data,
resource handles, and compiled program identity rather than C++ scene or sprite
objects.

A Shader implementing a Render contract does not by itself select that Shader
for every value of the contract. That selection must be an explicit package or
application fact before executable App lowering is complete. Export names such
as `Default2D` are discoverable names, not runtime binding policy.

Graphics owns resource lifetime and frame scheduling. Vulkan consumes the
compiled Graphics transaction and owns only Vulkan devices, swapchains,
pipelines, images, commands, and synchronization. Vulkan must not depend on TTX
or introduce frontend concepts such as Scene, Render2D, or Sprite.

`Puffer::Compiler` asks the resolved Abstract closure for the contracts required
by one requested output. The selected Dialects enrich that graph and expose
their execution facts. The terminal planner and backend consume the same
objects. Archive and header builds return their products to the caller. A
terminal owner can also return an arbitrary group and path product whose bytes
the Compiler retains. Adding a backend output does not add another semantic
owner. Package production succeeds only after the compiler proves every
required contract, so an incomplete backend cannot masquerade as a durable
package producer.

Puffer Buffers are terminal archive products owned by Tetrodotoxin. A durable
format must preserve the named facts and owner-defined data required to
reconstruct a package resolver, plus any requested machine or generated
terminals. It must not serialize process pointers, a ClassDB, or allocated Route
objects. Unknown contracts, incompatible versions, and corrupt edges restore as
an Invalid package root. The exact v1 Abstract archive schema remains migration
work and must be earned from the contracts that actually need persistence. The
filesystem path used to register the buffer remains resolver diagnostic
context. Compatibility rules belong in
[`archiver/README.md`](archiver/README.md).
