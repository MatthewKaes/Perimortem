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
The package/source graph layer will then load package files, resolve package
paths such as `TTX::Graphics`, check that imported files declare the requested
dialect, bind local import aliases, and call the dialect object registered under
the parsed dialect name. The old resolution code was removed so this layer can
be rebuilt from the current TTX source envelope and dialect dispatch model.

That split keeps filesystem identity, package records, cache invalidation, and
cross-file lookup in Tetrodotoxin while keeping the TTX language package small.
TTX can ask type and layout questions once Tetrodotoxin has supplied the
resolved imports, but it does not manage the source tree itself.

## Ownership

The main owners are intentionally narrow:

- `ttx/lexical` owns token classification and fixed source spellings
- `ttx/parse` owns the common source envelope and reusable parse facts
- `ttx/type.hpp` owns type identity, aliases, nested types, functions, and
  documentation attached to authored names
- `ttx/layout.hpp` owns shape questions such as exact equivalence, construction
  fit, name mapping, and default suffix rules
- The future Tetrodotoxin package/source graph owns package records, import
  graphs, package path lookup, local import aliases, and source lifetime
- TTX dialects own body parsing, dialect-specific declarations, exported facts,
  and legality for their authoring space
- Compilation and generation own terminal lowering into backend artifacts

Checks live with the owner that knows the fact. Layout fitting belongs to
`Layout`. Dialect legality belongs to the dialect. Package reachability belongs
to the package/source graph. Host boundary shape is deliberately deferred until dialect
parsing and lowering have enough context to earn that model. Tetrodotoxin should
avoid rebuilding those facts in a broad intermediate layer that becomes a second
source of truth.

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
primary representations that earlier TTX layers must imitate.

## Dialects And Outputs

Dialect names such as `Library`, `Package`, `Shader`, `Render`, and `Entity`
are authoring spaces registered with TTX. A dialect is not a backend target and
not a closed enum in the lexer. Adding a project-specific dialect should mean
registering an object with a name and behavior, then letting the source envelope
dispatch to that object after the package/source graph has prepared imports.

Backends are terminal targets selected later. A `Shader` dialect package may
emit SPIR-V. A `Library` package may eventually emit host code and boundary
metadata after dialect lowering defines those facts. A `Package` body may export
types and other package members. The dialect owns the source meaning, while
compilation and generation own the final artifact.

## Practical Rule

If a Tetrodotoxin layer starts cloning TTX into private "bound", "checked",
"shader", or "compiler" copies of the same program, the layer is probably
fighting the architecture. Prefer queryable facts owned by the package/source
graph, type, layout, dialect, providers, compilation, or generation. Keep the
source shape alive until the toolchain intentionally crosses a terminal output
boundary.
