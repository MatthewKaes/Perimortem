# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

The top-level [`ttx`](../ttx/README.md) package supplies the language model. It
owns source text, token bytecode, the Abstract class hierarchy, Layouts, and
documentation.
Tetrodotoxin provides Dialects that evaluate the bytecode, along with
Puffer source resolution, tooling, and backend context.

Use these documents for the language itself:

- [`ttx/README.md`](../ttx/README.md) for the current TTX architecture
- [`ttx/ttx_design.md`](../ttx/ttx_design.md) for the author-facing language
- [`ttx/ttx_semantics.md`](../ttx/ttx_semantics.md) for evaluator and semantic
  contracts

Use [`tetrodotoxin_design.md`](tetrodotoxin_design.md) for the Dialect,
Puffer resolution, package, and output model owned by this toolchain.

## Toolchain shape

Tetrodotoxin is not a second language model beside TTX. It is the toolchain that
hosts TTX and gives it the context a single source file cannot know.

```text
source text
-> TTX token bytecode
-> Puffer Boot preamble evaluation
-> Puffer resolution and package loading
-> declared dialect interpretation
-> Source-owned Abstract graph
-> Package reflection and execution surface
-> terminal Type and Layout lowering
-> requested terminal products
```

Lexical lowers source text into TTX token bytecode. Puffer starts full source
execution with its Boot envelope. Boot evaluates the source
preamble, which contains documentation, the `dialect : Name;` instruction, and
imports. The source keyword remains `dialect`, but semantically its name
resolves to a Dialect in the host's Abstract context named `Dialects`. A
Tetrodotoxin Namespace is one concrete representation. Puffer Boot hands the
body continuation to the selected Dialect after resolving imports.

Puffer Resolution loads package files, resolves package names such as
`Perimortem.Graphics`, checks that imported files declare the requested Dialect,
constructs Source or Package Dependency contracts with their local TTX Alias
bindings, and calls the Dialect resolved under the declared name. Source retains
each actual interpreted Dialect with the result it produced. Later stages do
not repeat name resolution from the authored spelling.
Package names use the token shape `Type("." Type)*`, so a parsed package name
can be used directly as the package cache key. The spelling alone does not make
a package identity a Type. The Package Dialect produces a source-backed Package
whose exports retain their actual contracts. Puffer creates a root
resolver from the Dialects context selected for that session. File imports stay
within at most two
coalesced project source roots owned once by that resolver. Package imports use the
separate registered-buffer repository and become explicit dependency edges.

Tetrodotoxin's standard TTX packages live under [`standard`](standard/). They
describe the ABI surfaces that Puffer can resolve today, including
`Perimortem.Graphics`, `Perimortem.Math`, and `Perimortem.Runtime`. Concrete
standard Types and Generic formulas enter evaluation through ordinary Abstract
contexts rather than a separate prelude model. Perimortem's C++ subsystems still
implement parts of these APIs, so generated C++ headers currently connect the
standard packages to that runtime.

Packages are Tetrodotoxin's module boundary. Source imports a package and uses
exports such as `Graphics::Shaders::Default2D`. It cannot import the package's
private shader file directly. If the package resolver cannot resolve its
private graph, the package source is dropped from the outer cache
with its consumers.

That split keeps filesystem identity, package records, cache invalidation, and
cross-file lookup in Tetrodotoxin while keeping the TTX language package small.
TTX can ask Abstract, Type, Callable, and Layout questions once Tetrodotoxin has
supplied the resolved imports, but it does not manage the source tree itself.

One Compiler owns the arena, Abstract DAG, source-owned Abstract contexts,
Generic instantiations, Layouts, Callable bodies and Addressables, diagnostics,
linker state, and terminal products for one build. There is no ClassDB or
allocated Route model. A future foreign boundary must use explicit versioned
operations and opaque non-null handles. C++ RTTI and vtables do not cross it.

## Repository map

The language core lives in [`../ttx`](../ttx/README.md):

- [`../ttx/lexical`](../ttx/lexical/) lowers source text into token bytecode
- [`../ttx/concept`](../ttx/concept/) owns the foundational Abstract,
  Documentation, and Layout contracts. Layout and Documentation are
  first-class concepts without Abstract identity
- [`../ttx/concept/abstract.hpp`](../ttx/concept/abstract.hpp) is the root
  semantic query contract. Alias, Invalid, and Model mechanisms extend it
  through ordinary inheritance
- [`../ttx/concept/layout.hpp`](../ttx/concept/layout.hpp) defines ordered
  identity-free fitting. [`../ttx/model/layouts`](../ttx/model/layouts/)
  contains Fluid, Named, Structured, Ranged, and Composite implementations
- [`../ttx/model/exports.hpp`](../ttx/model/exports.hpp) is the shared ordered
  public-definition contract consumed by dependency resolution, packages,
  reflection, and tooling. The remaining [`../ttx/model`](../ttx/model/) files
  own concrete Alias, documentation, Type, Expression, and Layout mechanisms

Tetrodotoxin layers toolchain context around that language core:

- [`model/namespace.hpp`](model/namespace.hpp) is Tetrodotoxin's concrete
  `Exports` owner for durable named source containment. It retains produced
  roots separately while direct lookup remains closed over its authored ordered
  exports, and
  [`model/source.hpp`](model/source.hpp) owns a complete
  formatter-capable source graph. [`model/dependency.hpp`](model/dependency.hpp)
  is the open import edge, with separate Source and Package locator contracts
  under [`model/dependencies`](model/dependencies/). None is a Type
- [`model/package.hpp`](model/package.hpp) extends `Exports` with the common
  package dependency surface;
  [`model/packages/interpreted.hpp`](model/packages/interpreted.hpp) adds optional
  Source access, [`model/packages/sources.hpp`](model/packages/sources.hpp) is
  Tetrodotoxin's concrete source-backed form, and
  [`model/packages/precompiled.hpp`](model/packages/precompiled.hpp) is the
  source-free restored form
- [`model/dialect.hpp`](model/dialect.hpp) is the named evaluation contract;
  [`model/definition.hpp`](model/definition.hpp) and
  [`model/definitions.hpp`](model/definitions.hpp) own compile-time definition
  composition, and [`model/dialects`](model/dialects/) begins with Package,
  Alias, and Group. Source invokes a Dialect and retains the exact returned
  result with that typed Dialect edge
- [`puffer`](puffer/README.md) owns the command-line host and source
  orchestration
- [`puffer/main.cpp`](puffer/main.cpp) is the `puffer` command-line entry point
- [`puffer/compiler.hpp`](puffer/compiler.hpp) coordinates one resolved compile
  against a Compiler boundary without owning command syntax or target behavior
- [`puffer/isa/boot`](puffer/isa/boot/) is the legacy implementation of
  Puffer's Boot envelope while that code migrates to the direct model
- [`puffer/lsp`](puffer/lsp/) owns Puffer's native language server mode
- [`puffer/resolution`](puffer/resolution/) owns source loading, package
  loading, import binding, the source cache, and cache validity
- [`puffer/package`](puffer/package/) presents resolved package closures to the
  Archiver without teaching the CLI about serialization tables
- [`puffer/toolchain.hpp`](puffer/toolchain.hpp) defines Puffer's standard
  Dialect and host-target composition policy
- [`archiver`](archiver/) owns durable package identities, dependency records,
  restored package data, and Puffer Buffer serialization
- [`lsp`](lsp/) contains the VSCode extension client and package assets
- [`isa`](isa/) retains the legacy body evaluators while their source domains
  migrate onto the Dialect and TTX Abstract model. It is reference code,
  not the canonical architecture
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
- Compilation can eventually consume resolved Type, Layout, Dialect, backend,
  and provider facts once the owning lowering layer exists
- Generation emits terminal outputs such as shader binaries, objects, archives,
  generated headers, and editor data

Every layer works with the same TTX Abstract graph. Tetrodotoxin does not create
a replacement semantic representation for each tool. A layer may enrich real
objects and contexts, but it cannot replace the graph with Type projections,
synthesized export paths, or pointer-keyed implementation tables. The cache
owner defines how those objects remain valid when its source graph changes.

Terminal outputs include SPIR-V, machine code, ELF files, JSON, and generated
headers. A terminal leaves the Abstract graph when its owner produces it. If
another tool needs the same information, the owning Abstract must expose the
required TTX facts instead of feeding a generated terminal back as semantic
input.

## Dialects

TTX defines token bytecode and an abstract machine. Its canonical lexer lowers
the textual TTX format, but another frontend can produce the same bytecode when
it implements that exact concrete Lexer contract.
Tetrodotoxin executes that bytecode through Dialects. Each Dialect implements
the semantic instruction set for one source region or continuation. The active
Abstract context of Dialects therefore defines the program a Tetrodotoxin host
can execute without creating a global registry or second semantic model.

Parent Dialects compose continuations with constexpr Definition mappings in one
`Definitions<...>` grammar. Each mapping carries its Dialect and the modifier
token Codes accepted by that parent grammar; it does not construct an
evaluator object, runtime function-pointer record, or polymorphic registry.
Definitions consumes the ordered publication/evaluation slots, owns collision,
rooting, and publication, and passes the authored tokens plus visible Abstract
contexts to the selected continuation. A richer Dialect supplies its own
distinct retained and exported owners; Namespace remains Tetrodotoxin's
concrete Package-group surface rather than a universal Scope.

The canonical lexer makes deliberate choices about TTX source syntax. A custom
lexer and a suitable set of Dialects could lower another language into the same
bytecode, but that is an integration path rather than a promise that TTX models
every source language directly.

Puffer provides `Boot` for source preambles. Tetrodotoxin's Package Dialect
is the first body migrated to the direct model. App, Library, Render, Scene, and
Shader remain source dialects and migrate as their real TTX facts become
available.

## Backend outputs

Dialects own source meaning and attach the resulting facts to the shared
Abstract DAG. Library attaches target-independent execution facts to Callable
objects in the Compiler-owned DAG. Foreign attaches external Addressable contracts.
The selected terminal planner resolves Abstract identity, proves Type,
recursively deconstructs non-empty Structured Layouts through their actual
Addressables, and lowers terminal leaves through target contracts. The backend
then chooses registers, instruction encoding, reversible name-derived symbols,
and relocations. A `Linker` converts terminal facts into durable binary records.

Shader currently emits SPIR-V through its assembler-facing state machine. A
future graphics execution interface should own that compiler boundary. The
assembler remains an implementation choice and not a public Dialect API.

## Layer interaction

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Named lookup and contract discovery are
Abstract queries. Represented identity comes from `Abstract::resolve()`.
Fitting and recursive storage shape are
[`Layout`](../ttx/concept/layout.hpp) queries. Invocation is a Static or Self
Callable query. Legality belongs to the selected Dialect. Package
reachability belongs to the package graph. Backends consume these facts only when the
toolchain crosses into a terminal artifact.

## Puffer

Puffer is Tetrodotoxin's command-line compiler for Perimortem. Bazel integrates
it through [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl).
Puffer can also compile selected `.ttx` roots directly.

Puffer stays thin around Tetrodotoxin's Dialects and compiler backend. `Main`
owns command syntax, diagnostics, and file output.
`Puffer::Toolchain` owns the Dialects context and backend composition.
`Puffer::Compiler` owns the objects and outputs of one compilation. Its Resolver
and selected Dialects enrich that boundary. The compiler loads dependency
package manifests first, then interprets the resolved closure. Puffer's Boot
evaluator reads each source preamble, then dispatches the body to the selected
Dialect.

Library, Package, and the other Dialects construct Abstract-derived objects,
Layouts, and extended contracts produced by the TTX abstract machine. The
Package Dialect returns a Model::Package over its export Namespace;
`Source::evaluate()` roots that exact Package with the typed Dialect edge.
Source is neither a Namespace, Type, Package, nor runtime aggregate. Puffer
coordinates those Dialects for one-shot compiles and long-running tooling
sessions.

Compiler modes query the resolved Abstract closure for the contracts required by
their output. A Dialect does not install a lowerer or claim package
readiness. The package builder freezes the resolved TTX facts together
with the archive and generated header terminals as Tetrodotoxin's
precompiled-library equivalent.

Package dependencies are resolved by package name, not by leaking source files
from one package into another. Bazel passes dependency `.puffer` outputs to
Puffer with `-dep=...`. The resolver registers each buffer and restores its
package once. Interpreted and restored packages implement the same Model::Package
contract. Only the interpreted form proves `Packages::Interpreted` and exposes
its Source collection. The restored package owns its TTX Abstract graph,
terminal byte payloads, and ABI execution facts. The resolver owns manifest
identity and projects its package key through Dependency bindings; the Package
Abstract itself remains anonymous. Physical paths remain resolver diagnostic
context and are not durable package identity. Direct source
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

public Size2D : alias = Math::Geometry::Size2D;
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
transactions can restore the source-free Package graph directly. Standalone
library targets produce the archive and C++ ABI header without a Puffer Buffer.
