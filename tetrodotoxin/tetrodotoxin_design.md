# Tetrodotoxin Design

Tetrodotoxin is the VM and toolchain host for TTX source IR. TTX supplies the
human-authored source format, token bytecode, and shared Type and Layout model.
Tetrodotoxin decides which instruction sets execute that bytecode, how source
files are loaded, how packages are resolved, and which terminal artifacts are
emitted.

TTX does not have a canonical ISA or canonical ISA set. A host toolchain can
install whatever ISAs it understands. Tetrodotoxin's standard toolchain provides
Boot as its source-file entry point and installs the body ISAs used by
Perimortem: Package, Library, Shader, Render, and future authoring spaces as
they become real.

## Boot

Boot is Tetrodotoxin's base ISA for complete TTX source files. It is called
directly by systems that know they are starting from a full source file. It is
not a selectable body ISA in the regular `Isa::Registry`.

Boot has an intentionally small instruction set:

- read source documentation
- execute the `dialect : Name;` instruction
- collect imports
- validate that requested ISA names exist in the active toolchain

That is the minimum envelope Tetrodotoxin needs to attach source bytes to the VM
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
lookup in the active toolchain.

## Resolution

The resolver is not an ISA. It is the source loading and cache-validity layer
between Boot and the selected body ISA.

After Boot evaluates the envelope, resolution loads the requested import
closure, resolves packages, checks imported source files declare the expected
ISA, and binds each import to the local name written in source. Once imports are
available, the resolver dispatches the remaining bytecode to the selected body
ISA with those imports in context.

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

Package-local resolution can use its own resolver graph. That keeps private
package files, package imports, and cache invalidation local to the package
while still allowing package dependencies to become explicit edges in the outer
source graph.

## Body ISAs

After Boot and resolution, the selected body ISA executes the rest of the token
bytecode. A Package ISA can publish package exports. A Library ISA can publish
types, functions, and host-code facts. Shader and Render ISAs can publish stage,
layout, binding, and lowering facts.

Those ISAs are sometimes dialect-like authoring spaces, but their job is more
specific than parsing. They are executable state machines over TTX token
bytecode that enrich the shared TTX model with facts owned by that authoring
space.

## Outputs

Backends are terminal targets, not ISAs. A Shader ISA may eventually supply facts
that a SPIR-V backend consumes. A Library ISA may supply facts for host object
generation. Package evaluation may supply exports for editor navigation and
compilation. The ISA owns source meaning. Compilation, linking, and generation
own the final artifact.
