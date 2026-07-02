# Tetrodotoxin

Tetrodotoxin is the source-IR toolchain used by Perimortem. It owns the
programs that load source files, build package graphs, serve editor requests,
compile packages, link outputs, and integrate with Bazel.

TTX is the default language that lives in this toolchain. In the repository, the
language core is the top-level [`ttx`](../ttx/README.md) package. Tetrodotoxin
uses that package for tokenization, source-envelope parsing, type and layout
facts, documentation, and dialect dispatch. The `tetrodotoxin` directory then
wraps those facts with the tooling that needs source-tree and backend context.

Use these documents for the language itself:

- [`ttx/README.md`](../ttx/README.md) for the current TTX architecture
- [`ttx/ttx_design.md`](../ttx/ttx_design.md) for the author-facing language
- [`ttx/ttx_semantics.md`](../ttx/ttx_semantics.md) for parser and semantic
  contracts

## Toolchain Shape

Tetrodotoxin is not a second language model beside TTX. It is the toolchain that
hosts TTX and gives it the context a single source file cannot know.

```text
source text
-> TTX lexical classes
-> TTX source envelope
-> Tetrodotoxin package/source graph
-> registered TTX dialect parser
-> owned query contexts
-> compilation
-> generation
```

The TTX root parser reads only the universal envelope: documentation, dialect
header, and imports. It leaves the same parse cursor positioned at the first
dialect-owned token so Tetrodotoxin can resolve imports and then let the
registered dialect continue from that state.
The package/source graph layer loads package files, resolves package paths such
as `TTX::Graphics`, checks that imported files declare the requested dialect,
binds local import aliases, and calls the dialect object registered under the
parsed dialect name.

That split keeps filesystem identity, package records, cache invalidation, and
cross-file lookup in Tetrodotoxin while keeping the TTX language package small.
TTX can ask type and layout questions once Tetrodotoxin has supplied the
resolved imports, but it does not manage the source tree itself.

## Repository Map

The language core lives in [`../ttx`](../ttx/README.md):

- [`../ttx/lexical`](../ttx/lexical/) classifies source text into token classes
- [`../ttx/parse`](../ttx/parse/) reads the common source envelope and imports
- [`../ttx/dialect`](../ttx/dialect/) registers dialects and dispatches body
  parsing
- [`../ttx/type.hpp`](../ttx/type.hpp) models type identity, aliases, members,
  nested types, functions, and type-owned documentation
- [`../ttx/layout.hpp`](../ttx/layout.hpp) models shape, fitting, exact
  equivalence, named member access, and type-to-layout views
- [`../ttx/documentation.hpp`](../ttx/documentation.hpp) preserves
  source-authored documentation for editor, package, and export surfaces

Tetrodotoxin layers toolchain context around that language core:

- [`cli`](cli/) is the command-line entry point
- [`lsp`](lsp/) contains the language server and VSCode client
- [`packages`](packages/) contains toolchain packages such as `TTX::Graphics`
- [`ttx.bzl`](ttx.bzl) integrates TTX source with Bazel targets
- [`compiler/assembler`](compiler/assembler/) emits terminal instruction
  streams such as SPIR-V and x86-64
- [`linker`](linker/) owns object records, archive packaging, and target formats

## TTX In The Toolchain

TTX is source shaped by design. The same authored source supports multiple
tooling depths:

- Syntax highlighting can stop after lexical classification
- Formatting can stop after source shape
- Project navigation can stop after the package/source graph
- Editor diagnostics can combine parser facts, package graph facts, and owned
  query answers
- Compilation can eventually consume resolved type, layout, dialect, backend,
  and provider facts once the owning lowering layer exists
- Generation emits terminal artifacts such as shader binaries, objects,
  archives, generated headers, and editor data

Terminal outputs may be lowered into SPIR-V, machine code, ELF files, JSON, or
other backend formats. Those outputs are products of the toolchain, not new
primary representations for earlier TTX layers.

## Dialects And Outputs

Dialect names such as `Library`, `Package`, `Shader`, `Render`, and `Entity`
are authoring spaces registered with TTX. A dialect is not a backend target and
not a closed enum in the lexer. Project-specific dialects register an object
with a name and behavior, then the source envelope dispatches to that object
after the package/source graph has prepared imports.

Backends are terminal targets selected later. A `Shader` dialect package may
emit SPIR-V. A `Library` package may eventually emit host code and boundary
metadata after dialect lowering defines those facts. A `Package` body may export
types and other package members. The dialect owns the source meaning, while
compilation and generation own the final artifact.

## Layer Interaction

Tetrodotoxin layers ask the owner of a fact instead of copying the whole program
into a private replacement model. Layout fitting is a
[`Layout`](../ttx/layout.hpp) question. Type identity and member lookup are
[`Type`](../ttx/type.hpp) questions. Dialect legality belongs to the registered
dialect. Package reachability belongs to the package/source graph. Backends
consume the resolved facts they need when the toolchain crosses into a terminal
artifact.
