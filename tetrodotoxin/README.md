# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

TTX is the default language that lives in this toolchain. In the repository, the
language core is the top-level [`ttx`](../ttx/README.md) package. Tetrodotoxin
uses that package for source text, token bytecode, type and layout facts, and
documentation. The `tetrodotoxin` directory then hosts the VM that evaluates
that bytecode with a caller-owned ISA registry, Puffer source resolution,
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
the active `Isa::Registry`. Puffer Boot is not a selectable body ISA in that
registry.

Puffer Resolution loads package files, resolves package names such as
`Perimortem.Graphics`, checks that imported files declare the requested ISA,
binds local import aliases, and calls the evaluator installed under the declared
name. Resolution retains the resolved dialect value on each published record;
it does not pass the authored name forward for another registry lookup. Package
names are `Type("." Type)*`, so a parsed package name can be used directly as
the package cache key. Puffer creates a root resolver from the body ISA registry
selected for that session. File imports stay within at most two coalesced
project source roots owned once by that resolver. Package imports use the
separate registered-buffer repository and become explicit dependency edges.

Tetrodotoxin's standard TTX packages live under [`standard`](standard/).
These packages describe the current standard ABI surface that Puffer can resolve
today, such as `Perimortem.Graphics`, `Perimortem.Math`, and `Perimortem.Runtime`.
The same layer owns the built-in standard type table used by ISA resolution.
The matching C++ engine subsystems in Perimortem aren't fully self-hosted yet so the
standard TTX package layer that the toolchain uses bridges the C++ gap for now, but
will continue to provide C++/C ABI headers in the future.

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

- [`puffer`](puffer/README.md) owns the command-line host, source orchestration,
  and the Puffer Buffer format reference
- [`puffer/main.cpp`](puffer/main.cpp) is the `puffer` command-line entry point
- [`puffer/compiler.hpp`](puffer/compiler.hpp) owns one resolved compile,
  lowering, and package-serialization transaction without owning command syntax
  or backend-specific behavior
- [`puffer/isa/boot`](puffer/isa/boot/) owns Puffer's source preamble ISA
- [`puffer/lsp`](puffer/lsp/) owns Puffer's native language server mode
- [`puffer/resolution`](puffer/resolution/) owns source loading, package
  loading, import binding, the source cache, and cache validity
- [`puffer/package`](puffer/package/) assembles resolved package closures into
  Puffer Buffers without teaching the CLI about archive reference tables
- [`puffer/toolchain.hpp`](puffer/toolchain.hpp) defines Puffer's stateless
  standard ISA and host-backend composition policy
- [`archiver`](archiver/) owns durable package identities, dependency records,
  restored package data, and Puffer Buffer serialization
- [`lsp`](lsp/) contains the VSCode extension client and package assets
- [`isa`](isa/) owns `Isa::Registry`, the composable `Isa::Base` instruction
  forms, and body ISAs such as Package, Library, Shader, and Render
- [`isa/lowering`](isa/lowering/) owns the borrowed input and output services
  presented to the lowerer selected for one resolved record
- [`standard`](standard/) owns the standard TTX packages Puffer can resolve,
  including the Perimortem graphics, math, and runtime ABI surfaces
- [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl) integrates
  TTX source with Bazel targets
- [`compiler`](compiler/README.md) owns target-independent execution programs,
  allocation, target backends, and their private assemblers
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
include `App`, `Library`, `Package`, `Render`, `Scene`, and `Shader`.

## Backend outputs

ISAs own source meaning and lower it into the execution interface appropriate
to that domain. Library emits a target-independent `Compiler::Execution::Program` with
typed operands and ordered calls. The backend supplied to the
`Compiler::Engine` owned by `Puffer::Compiler` chooses ABI, registers,
instruction encoding, symbols, and relocations. Foreign is a Library child
dialect that publishes typed `Compiler::Linkage` linkage rather than an opaque
provider object. A `Linker` converts backend facts into durable binary records.

Shader currently still emits SPIR-V through its assembler-facing state machine.
Moving that path behind a graphics execution interface is the next compiler
boundary; the assembler is an implementation choice, not a public ISA API.

## Layer Interaction / TTX Abstract Machine

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Layout fitting is a
[`Layout`](../ttx/layout.hpp) question. Type identity and member lookup are
[`Type`](../ttx/type.hpp) questions. ISA legality belongs to the active
`Isa::Registry`. Package reachability belongs to the package graph.
Backends
consume the resolved facts they need when the toolchain crosses into a terminal
artifact.

## Puffer

Puffer is the canonical Tetrodotoxin command-line compiler for the Perimortem Engine.
Perimortem integrates it into its Bazel toolchain through [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl). It can be run independently when a caller wants to turn selected
`.ttx` roots into toolchain outputs that Perimortem can manage.

Puffer is intentionally thin around Tetrodotoxin's body ISAs and compiler
backend. `Main` owns command syntax, diagnostics, and file output.
`Puffer::Compiler` owns a registry created by
`Puffer::Toolchain::standard_registry()`. Its Resolver borrows that registry,
while its Compiler Engine receives the independently selected host backend. The
compiler loads dependency package manifests first, then executes ISAs on the
resolved closure.
Puffer's Boot ISA reads each source preamble, then dispatches to Tetrodotoxin's
built-in body ISAs to evaluate the rest of the source.

Library, Package, and other ISAs populate to create the full Virtual Machine runtime
that executes TTX Token Bytecode to produce a full TTX Abstract Machine (types, layouts,
members, functions, etc). In this sense as a layer `Puffer` acts as the front end
orchestrator for one-shot compiles and long-running tooling sessions.

In `-library` mode, Puffer invokes the lowerer installed with each resolved ISA.
Library's continuation emits host linker records and a generated C++ FFI
header; Puffer never dispatches on Library or Foreign itself.

In `-package` mode, Puffer walks the resolved root closure and passes each input
to its installed lowerer through `Isa::Lowering`. Registry entries explicitly
state whether their lowerer is complete enough for package output; partial
backends remain usable without being silently treated as durable package
producers. The package builder then freezes the resolved TTX facts together
with the archive and generated header terminals as Tetrodotoxin's
precompiled-library equivalent.

Package dependencies are resolved by package name, not by leaking source files
from one package into another. Bazel passes dependency `.puffer` outputs to
Puffer with `-dep=...`; the resolver registers each buffer and restores its
package once. The restored `Package` owns its manifest identity, TTX type tree,
terminal byte payloads, and function linkage. Physical paths remain resolver
diagnostic context and are not durable package identity. Direct source loading
is only for files in the current package workspace; package imports do not fall
back to guessed `.ttx` paths. The complete package model and byte layout are
documented in [`puffer/README.md`](puffer/README.md).

Dependency restore is transitive, but name visibility is not. Importing
`Perimortem.Graphics` can make its Math-backed member types valid because the
Graphics Puffer Buffer references `Perimortem.Math`, but it does not bind a
local `Math` name for the consumer. Public forwarding is explicit package
surface:

```ttx
import Math : Package = Perimortem.Math;

expose Size2D : alias = Math::Geometry::Size2D;
```

That keeps package APIs friendly while still making canonical type identity
clear. Two packages that spell `Size2D` independently are different types unless
they intentionally alias the same canonical source.

In `--pipe=<socket>` mode, Puffer starts the native LSP server over the socket
provided by an editor client. The VSCode extension packages and launches the
same `puffer` binary rather than a separate language-server executable.

Puffer writes package snapshots as `.puffer` files. Bazel exposes the snapshot
separately from its generated archive and header so later compiler and tooling
transactions can restore the typed package directly.
