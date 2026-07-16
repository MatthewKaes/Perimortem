# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

The top-level [`ttx`](../ttx/README.md) package supplies the language model. It
owns source text, token bytecode, the Abstract class hierarchy, Layouts, and
documentation.
Tetrodotoxin provides the VM that evaluates the bytecode, along with the ISA
registry, Puffer source resolution, tooling, and backend context.

Use these documents for the language itself:

- [`ttx/README.md`](../ttx/README.md) for the current TTX architecture
- [`ttx/ttx_design.md`](../ttx/ttx_design.md) for the author-facing language
- [`ttx/ttx_semantics.md`](../ttx/ttx_semantics.md) for evaluator and semantic
  contracts

Use [`tetrodotoxin_design.md`](tetrodotoxin_design.md) for the VM, ISA,
Puffer resolution, package, and output model owned by this toolchain.

## Toolchain shape

Tetrodotoxin is not a second language model beside TTX. It is the toolchain that
hosts TTX and gives it the context a single source file cannot know.

```text
source text
-> TTX token bytecode
-> Puffer Boot preamble evaluation
-> Puffer resolution and package loading
-> declared ISA evaluation
-> Compiler-owned Abstract DAG
-> terminal Type and Layout lowering
-> requested terminal products
```

Lexical lowers source text into TTX token bytecode. Puffer starts full source
execution by calling its `Boot` ISA directly. Boot evaluates the source
preamble, which contains documentation, the `dialect : Name;` instruction, and
imports. The source keyword remains `dialect`, but semantically it selects the
next ISA from
the active `Isa::Registry`. Puffer Boot is not a selectable body ISA in that
registry.

Puffer Resolution loads package files, resolves package names such as
`Perimortem.Graphics`, checks that imported files declare the requested ISA,
binds local import aliases, and calls the evaluator installed under the declared
name. Resolution may retain the resolved dialect value on each source record.
Later stages do not repeat the registry lookup from the authored name. Package
names are `Type("." Type)*`, so a parsed package name can be used directly as
the package cache key. Puffer creates a root resolver from the body ISA registry
selected for that session. File imports stay within at most two coalesced
project source roots owned once by that resolver. Package imports use the
separate registered-buffer repository and become explicit dependency edges.

Tetrodotoxin's standard TTX packages live under [`standard`](standard/). They
describe the ABI surfaces that Puffer can resolve today, including
`Perimortem.Graphics`, `Perimortem.Math`, and `Perimortem.Runtime`. The same
subsystem provides the built-in prelude objects and contexts used during ISA
evaluation. Perimortem's
C++ subsystems still implement parts of these APIs, so generated C++ headers
currently connect the standard packages to that runtime.

Packages are Tetrodotoxin's module boundary. Source imports a package and uses
exports such as `Graphics::Shaders::Default2D`. It cannot import the package's
private shader file directly. If the package resolver cannot resolve its
private graph, the package source is dropped from the outer cache
with its consumers.

That split keeps filesystem identity, package records, cache invalidation, and
cross-file lookup in Tetrodotoxin while keeping the TTX language package small.
TTX can ask Abstract, Type, Callable, and Layout questions once Tetrodotoxin has
supplied the resolved imports, but it does not manage the source tree itself.

One Compiler owns the arena, Abstract DAG, source-owned resolver contexts,
Generic instantiations, Layouts, Callable bodies and Addressables, diagnostics,
linker state, and terminal products for one build. There is no ClassDB or
allocated Route model. A future foreign boundary must use explicit versioned
operations and opaque non-null handles. C++ RTTI and vtables do not cross it.

## Repository map

The language core lives in [`../ttx`](../ttx/README.md):

- [`../ttx/lexical`](../ttx/lexical/) lowers source text into token bytecode
- [`../ttx/abstraction/abstract.hpp`](../ttx/abstraction/abstract.hpp) is the root semantic query
  contract. Alias, Invalid, Type, Generic, Callable, Static, Self, Addressable,
  and ISA-specific objects extend it through ordinary inheritance
- [`../ttx/model/layout.hpp`](../ttx/model/layout.hpp) defines ordered fitting;
  [`../ttx/model/layouts`](../ttx/model/layouts/) contains Fluid, Named, and
  Structured
- [`../ttx/model/documentation.hpp`](../ttx/model/documentation.hpp) preserves
  source-authored documentation for editor, package, and export surfaces

Tetrodotoxin layers toolchain context around that language core:

- [`puffer`](puffer/README.md) owns the command-line host and source
  orchestration
- [`puffer/main.cpp`](puffer/main.cpp) is the `puffer` command-line entry point
- [`puffer/compiler.hpp`](puffer/compiler.hpp) coordinates one resolved compile
  against a Compiler boundary without owning command syntax or target behavior
- [`puffer/isa/boot`](puffer/isa/boot/) owns Puffer's source preamble ISA
- [`puffer/lsp`](puffer/lsp/) owns Puffer's native language server mode
- [`puffer/resolution`](puffer/resolution/) owns source loading, package
  loading, import binding, the source cache, and cache validity
- [`puffer/package`](puffer/package/) presents resolved package closures to the
  Archiver without teaching the CLI about serialization tables
- [`puffer/toolchain.hpp`](puffer/toolchain.hpp) defines Puffer's standard ISA
  and host-target composition policy
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
- [`compiler`](compiler/README.md) owns the per-build Abstract DAG and memory
  boundary, terminal planning, execution facts, allocation, target backends,
  and their private assemblers
- [`linker`](linker/) owns object records, archive packaging, and target formats

## TTX in the toolchain

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

Every layer works with the same TTX Abstract graph. Tetrodotoxin does not create
a replacement semantic representation for each tool. A layer may enrich real
objects and contexts, but it cannot replace the graph with Type projections,
synthesized export paths, or pointer-keyed implementation tables. The cache
owner defines how those objects remain valid when its source graph changes.

Terminal outputs include SPIR-V, machine code, ELF files, JSON, and generated
headers. A terminal leaves the semantic pipeline when its owner produces it. If
a later layer needs the same information, the earlier layer must expose the
required TTX facts instead of feeding a generated terminal back into the
pipeline.

## ISAs and virtual machines

TTX defines token bytecode and an abstract machine. Its canonical lexer lowers
the textual TTX format, but another frontend can produce the same bytecode.
Tetrodotoxin executes that bytecode through registered instruction set
architectures, or ISAs. Each ISA owns the instructions and facts for one source
region. The active registry therefore defines the program that a Tetrodotoxin
toolchain can execute.

The canonical lexer makes deliberate choices about TTX source syntax. A custom
lexer and a suitable set of ISAs could lower another language into the same
bytecode, but that is an integration path rather than a promise that TTX models
every source language directly.

Puffer provides `Boot` for source preambles. Built-in Tetrodotoxin body ISAs
include `App`, `Library`, `Package`, `Render`, `Scene`, and `Shader`.

## Backend outputs

ISAs own source meaning and lower it into the execution interface appropriate
to that domain. Library attaches target-independent execution facts to Callable
objects in the Compiler-owned DAG. Foreign attaches external Addressable contracts.
The selected terminal planner resolves Abstract identity, proves Type,
recursively deconstructs non-empty Structured Layouts through their actual
Addressables, and lowers terminal leaves through target contracts. The backend
then chooses registers, instruction encoding, reversible name-derived symbols,
and relocations. A `Linker` converts terminal facts into durable binary records.

Shader currently emits SPIR-V through its assembler-facing state machine. A
future graphics execution interface should own that compiler boundary. The
assembler remains an implementation choice and not a public ISA API.

## Layer interaction

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Named lookup and contract discovery are
Abstract queries. Represented identity comes from `Abstract::resolve()`.
Fitting and recursive storage shape are
[`Layout`](../ttx/model/layout.hpp) queries. Invocation is a Static or Self
Callable query. ISA legality belongs to the active registry. Package reachability
belongs to the package graph. Backends consume these facts only when the
toolchain crosses into a terminal artifact.

## Puffer

Puffer is Tetrodotoxin's command-line compiler for Perimortem. Bazel integrates
it through [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl).
Puffer can also compile selected `.ttx` roots directly.

Puffer stays thin around Tetrodotoxin's body ISAs and compiler backend. `Main`
owns command syntax, diagnostics, and file output.
`Puffer::Toolchain` owns the ISA registry and backend composition.
`Puffer::Compiler` owns the objects and outputs of one compilation. Its Resolver
and selected ISAs enrich that boundary. The compiler loads dependency package
manifests first, then executes ISAs on the resolved closure.
Puffer's Boot ISA reads each source preamble, then dispatches to Tetrodotoxin's
built-in body ISAs to evaluate the rest of the source.

Library, Package, and the other ISAs construct Abstract-derived objects, Layouts,
and ISA contracts produced by the TTX abstract machine. Puffer coordinates those ISAs
for one-shot compiles and long-running tooling sessions.

In `-library` mode, Puffer invokes the lowerer installed with each resolved ISA.
Library's continuation emits host linker records and a generated C++ FFI
header. Puffer never dispatches on Library or Foreign itself.

In `-package` mode, Puffer walks the resolved root closure and passes each input
to its installed lowerer through `Isa::Lowering`. Registry entries explicitly
state whether their lowerer is complete enough for package output. Partial
backends remain usable without becoming durable package producers. The package
builder then freezes the resolved TTX facts together
with the archive and generated header terminals as Tetrodotoxin's
precompiled-library equivalent.

Package dependencies are resolved by package name, not by leaking source files
from one package into another. Bazel passes dependency `.puffer` outputs to
Puffer with `-dep=...`. The resolver registers each buffer and restores its
package once. The restored `Package` owns its manifest identity, TTX Abstract
graph, terminal byte payloads, and callable Addressables. Physical paths remain
resolver diagnostic context and are not durable package identity. Direct source
loading only reads files in the current package workspace. Package imports do
not fall back to guessed `.ttx` paths. A durable package must reconstruct its
resolver from named facts and owner-defined data. It cannot depend on ClassDB
schema references, allocated Routes, or process Addresses. The complete package
model is documented in
[`archiver/README.md`](archiver/README.md).

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

Puffer writes each package into a module directory named from its authored
identity. `Perimortem.Math` therefore contains `binary_archive.puffer`,
`x86_64.a`, and `cpp_abi.hpp` regardless of the Bazel target or repository that
built it. The manifest carries the same package identity so compiler and tooling
transactions can restore the typed package directly. Standalone library targets
produce the archive and C++ ABI header without a Puffer Buffer.
