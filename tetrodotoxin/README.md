# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

TTX is the default language that lives in this toolchain. In the repository, the
language core is the top-level [`ttx`](../ttx/README.md) package. Tetrodotoxin
uses that package for source text, token bytecode, type and layout facts, and
documentation. The `tetrodotoxin` directory then hosts the VM that evaluates
that bytecode with a toolchain-owned ISA registry, Puffer source resolution,
tooling, and backend context.

Use these documents for the language itself:

- [`ttx/README.md`](../ttx/README.md) for the current TTX architecture
- [`ttx/ttx_design.md`](../ttx/ttx_design.md) for the author-facing language
- [`ttx/ttx_semantics.md`](../ttx/ttx_semantics.md) for evaluator and semantic
  contracts

Use [`tetrodotoxin_design.md`](tetrodotoxin_design.md) for the VM, ISA,
Puffer resolution, package, and output model owned by this toolchain.

## Toolchain Shape

Tetrodotoxin is not a second language model beside TTX. It is the toolchain that
hosts TTX and gives it the context a single source file cannot know.

```text
source text
-> TTX token bytecode
-> Puffer Boot preamble evaluation
-> Puffer resolution and package loading
-> declared ISA evaluation
-> owned query contexts
-> compilation
-> generation
```

Lexical lowers source text into TTX token bytecode. Puffer starts full source
execution by calling its `Boot` ISA directly. Boot evaluates the source
preamble: documentation, the `dialect : Name;` instruction, and imports. The
source keyword remains `dialect`, but semantically it selects the next ISA from
the active toolchain's `Isa::Registry`. Puffer Boot is not a selectable body ISA
in that registry.

Puffer Resolution loads package files, resolves package names such as
`Perimortem::Graphics`, checks that imported files declare the requested ISA,
binds local import aliases, and calls the evaluator installed under the declared
name. Puffer creates a root resolver from its active toolchain. Package
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

- [`puffer/main.cpp`](puffer/main.cpp) is the `puffer` command-line entry point
- [`puffer/isa/boot`](puffer/isa/boot/) owns Puffer's source preamble ISA
- [`puffer/lsp`](puffer/lsp/) owns Puffer's native language server mode
- [`puffer/resolution`](puffer/resolution/) owns source loading, package
  loading, import binding, the source cache, and cache validity
- [`lsp`](lsp/) contains the VSCode extension client and package assets
- [`toolchain.hpp`](toolchain.hpp) owns the VM capability table for one caller
- [`isa`](isa/) owns `Isa::Registry` and the VM instruction sets
  such as Package, Library, Shader, and Render
- [`../perimortem/graphics/package.ttx`](../perimortem/graphics/package.ttx)
  describes the Perimortem graphics ABI as a TTX package
- [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl) integrates
  TTX source with Bazel targets
- [`compiler/assembler`](compiler/assembler/) emits terminal instruction
  streams such as SPIR-V and x86-64
- [`linker`](linker/) owns object records, archive packaging, and target formats

## TTX in the Toolchain

TTX is source shaped by design. The same authored source supports multiple
tooling depths:

- Syntax highlighting can stop after lexical classification
- Formatting can stop after source shape
- Project navigation can stop after resolution builds the package graph
- Editor diagnostics can combine evaluator facts, package graph facts, and owned
  query answers
- Compilation can eventually consume resolved type, layout, ISA, backend,
  and provider facts once the owning lowering layer exists
- Generation emits terminal outputs such as shader binaries, objects, archives,
  generated headers, and editor data

To support this Tetrodotoxin uses a layering approach to the toolchain where
various toolchain layers act on the core TTX Abstract Machine as the compatibility
layer rather than independent intermediary representations.

Each layer may produce as many terminal outputs as it requires and may add data
to the TTX Abstract Machine but it may never mutate or delete data from "left"
layers. This additive only model serves as the core to Tetrodotoxin's cache model.

Terminal outputs may be lowered into SPIR-V, machine code, ELF files, JSON, or
other backend formats. Those outputs are products of the toolchain, not new
primary representations for earlier TTX layers. Once a terminal is produced it
exits the toolchain. Any layer can introduce terminals into the toolchain but
terminals that are reentrant (leave at a left layer but are used in a right layer)
are a major code smell and should be avoided. Generally if terminal data is required
to the right layers it should be explicitly expressed as TTX operations so the output
can be used as a proper continuation.

## ISAs and Virtual Machines

TTX itself is not a language with a defined Syntax but just a Bytecode spec and an
attached Abstract Machine model. While TTX has a textual format which can be lowered
to TTX Bytecode via its canonical lexer package, any source can produce valid TTX Bytecode.

To actually do something useful with that bytecode it must be interpreted as an
Abstract Machine that produces actual TTX data. To handle this Tetrodotoxin uses TTX as
a sort of RISC model that it executes using a set of ISAs (Instruction Set Architectures)
that define an implementation of a TTX Abstract Machine. These ISAs are implemented as
layers that can be added to Tetrodotoxin to execute chunks of TTX Token Bytecode. A TTX
program then can be defined as the set of registered ISAs the toolchain uses.

This leaves the actual TTX Source IR Syntax left up to the toolchain. Adding a language or
a new syntax is as simple as defining a new ISA to handle a chunk of stream. The default TTX
lexer does make a number of assumptions about the source syntax, but with a custom `Lexer`
layer and interop ISAs it is not unreasonable to lower languages like C into TTX Bytecode to
leverage Tetrodotoxin directly, but this is most likely more effort than it is worth.

Puffer provides `Boot` for source preambles. Built-in Tetrodotoxin body ISAs
include `Library`, `Package`, `Shader`, `Render`, and `Entity`.

## Backend outputs

Backends are the main terminal layers for generating durable output. The `Shader` ISA
generates the meaningful TTX data, but the `Shader` backend emits proper SPIR-V Bytecode.
A `Library` package may eventually emit host code and boundary metadata after ISAs evaluate
those facts. A `Linker` can convert a `Package` body with export types into durable binary
records. The ISA owns the source meaning, while backends such as compilation and generation
own the final artifact.

## Layer Interaction / TTX Abstract Machine

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Layout fitting is a
[`Layout`](../ttx/layout.hpp) question. Type identity and member lookup are
[`Type`](../ttx/type.hpp) questions. ISA legality belongs to the ISA installed
in the active toolchain. Package reachability belongs to the package graph.
Backends
consume the resolved facts they need when the toolchain crosses into a terminal
artifact.

## Puffer

Puffer is the canonical Tetrodotoxin command-line compiler for the Perimortem Engine.
Perimortem integrates it into its Bazel toolchain through [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl). It can be run independently when a caller wants to turn selected
`.ttx` roots into toolchain outputs that Perimortem can manage.

Puffer is intentionally thin around the Tetrodotoxin toolchain's default ISA set.
It creates `Toolchain::standard()` and owns VM startup with its own `Boot` ISA.
Puffer drives the top `Resolver` layer for source `.ttx` inputs, loading
dependency sources first, then executing ISAs on selected roots. Puffer's Boot
ISA reads each source preamble, then dispatches to Tetrodotoxin's built-in body
ISAs to evaluate the rest of the source.

Library, Package, and other ISAs populate to create the full Virtual Machine runtime
that executes TTX Token Bytecode to produce a full TTX Abstract Machine (types, layouts,
members, functions, etc). In this sense as a layer `Puffer` acts as the front end
orchestrator for one-shot compiles and long-running tooling sessions.

In `-library` mode, Puffer pulls in the `Library` compiler layer to lower TTX facts
emitted by the Library ISA family into linker records, which are then written to static
archives and an additional generated C++ header as an FFI (Foreign Function Interface).

In `-package` mode, it resolves package manifests and emits the package terminal data
that contains a frozen VM checkpoint which acts as Tetrodotoxin's "precompiled library"
equivalent.

In `--pipe=<socket>` mode, Puffer starts the native LSP server over the socket
provided by an editor client. The VSCode extension packages and launches the
same `puffer` binary rather than a separate language-server executable.

Puffer writes snapshots as `.puffer` output called a Puffer Buffer. The Puffer
Buffer is intended to store the TTX Abstract Machine in a Tetrodotoxin-owned
terminal binary format. It is not meant to be used with non-TTX toolchains and
does not reflect the public C++ ABI nor metadata hidden inside ELF or Portable
Executable sections. Today it records source, import, ISA, type, member, and
function facts for the resolved terminal. The format can evolve with Tetrodotoxin
because the final C++ link sees only the appropriate archive and generated header
terminals emitted by the `Linker` layer. Bazel exposes the Puffer Buffer separately
for tooling and future resolver/compiler use.
