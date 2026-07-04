# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

TTX is the default language that lives in this toolchain. In the repository, the
language core is the top-level [`ttx`](../ttx/README.md) package. Tetrodotoxin
uses that package for source text, token bytecode, type and layout facts, and
documentation. The `tetrodotoxin` directory then hosts the VM that evaluates
that bytecode with a toolchain-owned ISA registry, resolution, tooling, and
backend context.

Use these documents for the language itself:

- [`ttx/README.md`](../ttx/README.md) for the current TTX architecture
- [`ttx/ttx_design.md`](../ttx/ttx_design.md) for the author-facing language
- [`ttx/ttx_semantics.md`](../ttx/ttx_semantics.md) for evaluator and semantic
  contracts

Use [`tetrodotoxin_design.md`](tetrodotoxin_design.md) for the VM, ISA,
resolution, package, and output model owned by this toolchain.

## Toolchain Shape

Tetrodotoxin is not a second language model beside TTX. It is the toolchain that
hosts TTX and gives it the context a single source file cannot know.

```text
source text
-> TTX token bytecode
-> Boot envelope evaluation
-> resolution and package loading
-> declared ISA evaluation
-> owned query contexts
-> compilation
-> generation
```

Lexical lowers source text into TTX token bytecode. Tetrodotoxin starts full
source execution by calling `Boot` directly. Boot evaluates the universal
envelope: documentation, the `dialect : Name;` instruction, and imports. The
source keyword remains `dialect`, but semantically it selects the next ISA from
the active toolchain's `Isa::Registry`. Boot is not a selectable body ISA in
that registry.

Resolution loads package files, resolves package names such as
`Perimortem::Graphics`, checks that imported files declare the requested ISA,
binds local import aliases, and calls the evaluator installed under the declared
name. Tools create a root resolver from their active toolchain. Package
manifests can create package-local resolver graphs. File imports stay under
that package's source subtree, while package imports can name packages from
elsewhere and become explicit dependency edges.

Packages are Tetrodotoxin's module boundary. A source imports a package and then
uses package exports such as `Graphics::Shaders::Default2D`; it does not import
the package's private shader file directly. If the package resolver cannot
resolve its private graph, the package source is dropped from the outer cache
with its consumers.

That split keeps filesystem identity, package records, cache invalidation, and
cross-file lookup in Tetrodotoxin while keeping the TTX language package small.
TTX can ask type and layout questions once Tetrodotoxin has supplied the
resolved imports, but it does not manage the source tree itself.

## Repository Map

The language core lives in [`../ttx`](../ttx/README.md):

- [`../ttx/lexical`](../ttx/lexical/) lowers source text into token bytecode
- [`../ttx/type.hpp`](../ttx/type.hpp) models type identity, aliases, members,
  nested types, functions, and type-owned documentation
- [`../ttx/layout.hpp`](../ttx/layout.hpp) models shape, fitting, exact
  equivalence, named member access, and type-to-layout views
- [`../ttx/documentation.hpp`](../ttx/documentation.hpp) preserves
  source-authored documentation for editor, package, and export surfaces

Tetrodotoxin layers toolchain context around that language core:

- [`main.cpp`](main.cpp) is the `puffer` command-line entry point
- [`lsp`](lsp/) contains the language server and VSCode client
- [`toolchain.hpp`](toolchain.hpp) owns the VM capability table for one caller
- [`isa`](isa/) owns `Isa::Registry` and the VM instruction sets
  such as Boot, Package, Library, Shader, and Render
- [`resolution`](resolution/) owns source loading, package loading, import
  binding, the source cache, and cache validity
- [`../perimortem/graphics/package.ttx`](../perimortem/graphics/package.ttx)
  describes the Perimortem graphics ABI as a TTX package
- [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl) integrates
  TTX source with Bazel targets
- [`compiler/assembler`](compiler/assembler/) emits terminal instruction
  streams such as SPIR-V and x86-64
- [`linker`](linker/) owns object records, archive packaging, and target formats

## TTX In The Toolchain

TTX is source shaped by design. The same authored source supports multiple
tooling depths:

- Syntax highlighting can stop after lexical classification
- Formatting can stop after source shape
- Project navigation can stop after resolution builds the package graph
- Editor diagnostics can combine evaluator facts, package graph facts, and owned
  query answers
- Compilation can eventually consume resolved type, layout, ISA, backend,
  and provider facts once the owning lowering layer exists
- Generation emits terminal artifacts such as shader binaries, objects,
  archives, generated headers, and editor data

Terminal outputs may be lowered into SPIR-V, machine code, ELF files, JSON, or
other backend formats. Those outputs are products of the toolchain, not new
primary representations for earlier TTX layers.

## ISAs And Outputs

ISA names such as `Library`, `Package`, `Shader`, `Render`, and `Entity` are
authoring spaces installed into a Tetrodotoxin toolchain. An ISA is not a
backend target and not a closed enum in the lexer. Project-specific ISAs install
an evaluator with a name and behavior, then Boot and the resolver dispatch to
that evaluator after resolution has prepared imports.

Backends are terminal targets selected later. A `Shader` ISA package may
emit SPIR-V. A `Library` package may eventually emit host code and boundary
metadata after ISA lowering defines those facts. A `Package` body may export
types and other package members. The ISA owns the source meaning, while
compilation and generation own the final artifact.

## Layer Interaction

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Layout fitting is a
[`Layout`](../ttx/layout.hpp) question. Type identity and member lookup are
[`Type`](../ttx/type.hpp) questions. ISA legality belongs to the ISA installed
in the active toolchain. Package reachability belongs to the package graph.
Backends
consume the resolved facts they need when the toolchain crosses into a terminal
artifact.
