# Tetrodotoxin Design

Tetrodotoxin is the VM and toolchain host for TTX source IR. TTX supplies the
human-authored source format, token bytecode, and shared Type and Layout model.
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
attach that semantic dialect, imports, and root Type before entering the cache.
The cache rejects incomplete records. File imports are limited by the
resolver's compact project-root table; compiled packages use the separate
package repository rather than carrying filesystem roots on every record.

The resolver also owns cache safety. TTX facts use address identity. If a
producer source is removed or republished, every consumer that may point into
the producer's arena must leave the cache or execute again. That dependency
graph is a Tetrodotoxin concern, not a TTX language feature.

## Packages

Packages are Tetrodotoxin's module boundary. A package source is evaluated by
the Package ISA and can expose package exports as TTX facts. Private files under
the package subtree are not imported directly by outside source. Outside source
imports the package by name, then queries exported types through the package
surface.

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
publish types, functions, and host-code facts. Shader and Render ISAs can publish
stage, layout, binding, and lowering facts.

Those ISAs are sometimes dialect-like authoring spaces, but their job is more
specific than parsing. They are executable state machines over TTX token
bytecode that enrich the shared TTX model with facts owned by that authoring
space.

`Isa::Base` is the composable instruction layer used by those body ISAs. It
owns shared authored forms such as documentation, attributes, declarations,
layouts, expressions, and the typed publication context. Base is not a
registry-selectable body dialect and does not define a generic block or
statement. Library, Shader, Scene, App, and future ISAs own those bodies and
publish their concrete data against stable TTX identities.

## Outputs

An ISA owns the meaning of its bodies and lowers that meaning into a compiler
execution interface. Library emits typed operands, calls, returns, and ordered
operations into `Compiler::Execution::Program`; it never names a register or
assembler. The backend supplied to the Puffer compilation transaction assigns
physical locations, implements the ABI, encodes instructions, and publishes
linker facts. Puffer currently selects x86-64 System V for host compilation.

Foreign is a child dialect invoked by Library at the foreign declaration
boundary. It publishes concrete `Compiler::Linkage` values for the functions it
produces. Library consumes the same linkage shape for Foreign functions,
another Library module, or a restored package without branching on the
producer dialect.

Shader still owns stage and render-contract meaning. Its SPIR-V state machine
is the next backend boundary to project through a typed graphics execution
interface. The produced modules already cross into `Compiler::Engine` as named
read-only data ranges, so Shader no longer sees linker sections or object
symbols; direct SPIR-V assembly inside Shader remains the temporary boundary.

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
`Isa::Lowering::Input` borrows the selected source facts, while
`Isa::Lowering::Context` exposes the compiler and terminal-product sinks for
that transaction. `Compiler::Engine` owns the Program, selected backend, and
linker transaction. Archive and header builds return their products to the
caller; Engine does not cache derived outputs. A lowerer can also publish an
arbitrary group/path terminal whose bytes the Puffer transaction retains, so
adding a backend output does not add another concrete compiler or orchestration
layer.
Package lowering is an explicit registry capability so an incomplete backend
is not mistaken for a durable package producer.

Puffer Buffers use a fixed header and table directory so manifest, reference,
package, and linkage reads can seek independently. The reader retains no
decoded object or continuation state. A restored Package owns its Manifest as
identity and dependency surface; the filesystem path used to register the
buffer remains resolver diagnostic context. The complete durable object model,
table grammar, and validation rules are documented in
[`puffer/README.md`](puffer/README.md).
