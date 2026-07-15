# Tetrodotoxin Design

Tetrodotoxin is the VM and toolchain host for TTX source IR. TTX supplies the
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

Boot is Puffer's preamble ISA for complete TTX source files. It is called
directly by systems that know they are starting from a full source file. It is
not a selectable body ISA in the regular `Isa::Registry`.

Boot has an intentionally small instruction set:

- read source documentation
- execute the `dialect : Name;` instruction
- collect imports
- validate that requested ISA names exist in the active body ISA registry

That is the minimum preamble Puffer needs to attach source bytes to the VM
model. Boot does not own package loading, type binding, lowering, or backend
output. It leaves the cursor positioned at the body bytecode so resolution and
the selected body ISA can continue the execution.

## ISA Registry

An ISA is an executable semantic instruction set for a TTX token stream. The
lexer has already assigned each token a bytecode class, but the ISA decides how
many tokens to fetch, what instruction shape those tokens form, and which TTX
facts to publish.

The registry is contextual. A CLI, LSP, test harness, embedded runtime, or
package-local resolver can each install the ISAs it intends to support. The name
in the source `dialect` instruction is therefore not a global enum. It is a
lookup in the active `Isa::Registry`.

An installed ISA also registers the Abstract classes and operations it
contributes. Shader can register Stage Types and terminal GPU facts. Library can
register executable bodies and host-callable address contracts. Another
language runtime can register its own Type or Callable subtypes. These classes
enrich one graph; they do not require a new universal Type switch.

## ClassDB And Abstract ABI

The Toolchain owns one long-lived ClassDB containing the schemas installed for
that configuration. A Compiler borrows the immutable registry while it owns the
objects created for one compilation boundary.

```text
Toolchain-owned ClassDB
          |
          v
Compiler-owned Abstract DAG
          |
          +-- Type -> Alias / Generic / ISA Type
          +-- Callable -> Free / Self
          +-- Address -> native / external / runtime / unresolved
          +-- ISA and language-specific contracts
          `-- Invalid
```

ClassDB replaces C++ RTTI as the authority for dynamic semantic contracts. A
registered Class has a unique versioned schema Route, a parent Class, required
operations, construction and destruction callbacks, reflected methods and
properties, callable Layouts, language/plugin ownership, and documentation.
Native implementations and foreign-language implementations expose the same
Class descriptor. Native code may use ClassDB ancestry to justify a static C++
cast. Foreign objects use opaque handles and registered C ABI callbacks; C++
vtables and object layouts never cross the boundary.

A loaded registry may assign dense local Class indices for fast lookup. They
are process-local acceleration only. Durable identity is the schema Route, such
as `Ttx1.Abstract.Callable.Self`; indices are never serialized, hashed into
symbols, or assumed equal across languages.

The public ABI has three cooperating parts:

| Part | Responsibility |
| ---- | -------------- |
| ClassDB | schema registration, ancestry, reflection, and operation lookup |
| Abstract handles | non-null language-neutral references to compiler-owned objects |
| Layout-described calls | complete parameter/result shape and target terminal lowering |

A C-facing Abstract handle contains an opaque object identity and Class handle.
It always designates a real object. Failed resolution designates the registered
Invalid object. Versioned entry points expose operations equivalent to class
registration, class resolution, ancestry checks, Abstract resolution, class
inspection, and Callable invocation. API versioning is explicit in the function
and schema names rather than folded into a hash.

## Puffer Resolution

The resolver is not an ISA. It is the source loading and cache-validity layer
between Puffer Boot and the selected body ISA.

After Puffer Boot evaluates the preamble, resolution loads the requested import
closure, resolves packages, checks imported source files declare the expected
ISA, and binds each import to the local name written in source. Once imports are
available, the resolver dispatches the remaining bytecode to the selected body
ISA with those imports in context.

Boot's authored ISA name is resolved once to an installed `Isa::Dialect`.
Unpublished source records progressively build their arena-backed facts, then
attach that semantic dialect, imports, and root Abstract before entering the
cache. A failed record publishes an Invalid root for diagnostics and is not a
valid cache hit. File imports are limited by the
resolver's compact project-root table. Compiled packages use the separate
package repository rather than carrying filesystem roots on every record.

The resolver also owns cache safety. Local object handles remain stable only for
the Compiler boundary that owns their arena. If a producer source is removed or
republished, every consumer that may retain those handles must leave the cache
or execute again. Durable package identity remains its authored Route and
version, not a process address or content hash. That dependency graph is a
Tetrodotoxin concern, not a TTX language feature.

## Packages

Packages are Tetrodotoxin's module boundary. A package source is evaluated by
the Package ISA and can expose package exports as TTX facts. Private files under
the package subtree are not imported directly by outside source. Outside source
imports the package by name, then queries exported Abstracts by contract through
the package surface. Type queries are one view of that graph; tooling may
query Callable, Alias, or ISA-specific contracts through the same root.

Built-in standard package sources live under `tetrodotoxin/standard`. They are
resolved by public package name, not by asking user source to import their
private files. This keeps the standard TTX ABI layer distinct from the current
C++ engine implementation while the graphics/runtime stack is not fully
self-hosted.

Package-local resolution can use its own resolver graph. That keeps private
package files, package imports, and cache invalidation local to the package
while still allowing package dependencies to become explicit edges in the outer
source graph.

## Body ISAs

After Puffer Boot and resolution, the selected body ISA executes the rest of the
token bytecode. A Package ISA can publish package exports. A Library ISA can
publish Type and Callable objects plus executable-body contracts. Shader and
Render ISAs can publish stage, Layout, binding, and terminal facts.

Those ISAs are sometimes dialect-like authoring spaces, but their job is more
specific than parsing. They are executable state machines over TTX token
bytecode that enrich the shared TTX model with facts owned by that authoring
space.

`Isa::Base` is the composable instruction layer used by those body ISAs. It
owns shared authored forms such as documentation, attributes, declarations,
layouts, expressions, and graph construction context. Base is not a
registry-selectable body dialect and does not define a generic block or
statement. Library, Shader, Scene, App, and future ISAs own those bodies and
enrich the relevant Abstract objects through their registered contracts. A
pointer-keyed `Implementation` table must not become a second semantic
authority beside the graph.

## Outputs

The Compiler is the memory and terminal-product boundary for one build. It
owns:

- the arena and every instantiated Abstract object
- Resolution routes and imported graph edges
- concrete Generic instantiations and Layouts
- Callable bodies, Addresses, and linkage objects
- diagnostics and Invalid objects
- target-independent execution facts
- linker state, generated interfaces, and other terminal products.

ISAs construct registered Abstract-derived objects inside that boundary and
enrich them with their own contracts. Library may attach a target-independent
execution body to a Callable. Foreign may attach an external Address. Shader
may attach stage and GPU terminal facts. The compiler consumes these contracts
without switching on the producer ISA and without reconstructing ownership from
pointer-keyed side tables.

Lower compiler layers operate on narrow interfaces. For a Type they
canonicalize it and obtain its concrete Layout. Non-empty Layouts are recursively
deconstructed in declaration order. Empty Layouts are lowered through a
terminal contract registered by the selected target or ISA. The same recursive
projection drives parameters, results, registers, stack placement, generated
host declarations, and durable archive descriptions.

```text
Type::canonicalize()
-> Type::get_layout()
-> aggregate: recursively lower child Types
-> terminal: invoke selected terminal Type contract
```

An authored `@abi` scalar, a C++ type switch, or an outer-Type shortcut is not a
terminal type fact. In particular, a composite with a non-empty Layout cannot
be laundered into one scalar because a backend recognizes its name. A view,
struct, vector, render contract, or foreign carrier remains one semantic value
whose terminal representation is the ordered projection of its entries.

Free and Self Callable objects expose their complete parameter and result
Layouts. Self includes its receiver at parameter zero. The compiler never
prepends that receiver a second time. A resolved implementation supplies an
Address object; unresolved linkage supplies an explicit unresolved Address or
Invalid, not `nullptr`.

Public and internal symbols are reversible encodings of selected Routes. The
route already contains real Callable.Free or Callable.Self schema steps, so the
compiler does not invent `.Type` and `.Addressable` publishing paths. It does
not select a lexicographically preferred alias and does not hash a path or
signature. ABI and package versions, when needed, are explicit route segments.

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

Scene lowering can then identify members whose canonical types are Render
contracts and publish where those values live in scene storage. A compiled C++
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

`Puffer::Compiler` dispatches only lowerers installed by the active registry.
`Isa::Lowering::Input` borrows the selected source facts.
`Isa::Lowering::Context` exposes the Compiler-owned Abstract graph and terminal
transactions. The selected ISAs enrich that graph and publish their execution
facts; the selected terminal planner and backend consume the same objects.
Archive and header builds return their products to the caller. A lowerer can
also publish an arbitrary group and path terminal whose bytes the Compiler
retains. Adding a backend output does not add another semantic owner.
Package lowering is an explicit registry capability so an incomplete backend
is not mistaken for a durable package producer.

Puffer Buffers serialize ClassDB schema references, Abstract objects,
contract-qualified child edges, Routes, Layouts, Addresses, and terminal
products. Restore allocates a complete graph in the receiving Compiler boundary.
Unknown or incompatible schemas and corrupt edges produce an Invalid package
root; nullable type edges and partially published objects are not part of the
model. The filesystem path used to register the buffer remains resolver
diagnostic context. The durable object model and compatibility rules are
documented in [`archiver/README.md`](archiver/README.md).
