# Puffer

Puffer is Tetrodotoxin's command-line and language-server host. It loads complete
TTX sources, assembles container Environments, resolves exact packages, selects
body Dialects, and requests terminal products from the compiler.

Puffer owns orchestration. It does not own the TTX semantic contracts, backend
ABI rules, linker formats, or a parallel Type model.

## Build transaction

A build begins from an explicit ordered Source member set, package inputs, and
the Dialects exposed by the active toolchain. The resolver completes one
Environment, Boot reads each source envelope, and the selected Dialect
evaluates the remaining token bytecode inside the build boundary.

Puffer does not supply a ClassDB, allocated Route model, or global semantic
registry. Dialect selection is ordinary Abstract resolution through the host's
`Dialects` context. Semantic lookup continues through the Abstract graph and
borrowed `View::Bytes` routes.

The build returns completed artifacts. Puffer does not retain Type, Callable,
Layout, or other local object identities after their owning Compiler boundary
ends.

## Resolution ownership

The resolver owns the source and package dependency graph for its workspace.
It decides whether a source system can be enriched, whether a dependency closure
must be replaced, and which readers may retain borrowed references into that
closure.

The same starting Abstract and ordered query chain are deterministic while the
graph is unchanged. When Puffer changes the graph, it also owns invalidation and
reference lifetime. Process addresses are valid local identities only for that
stable lifetime.

Failed source or package evaluation resolves to Invalid and retains diagnostics
on the source-owning record. A partially connected semantic graph and a null root
are not valid published results.

## Packages and outputs

Package identity comes from authored names and explicit versions. Filesystem
paths and local cache indices are host data rather than semantic identity.
The repository owns Manifest keys, hash-indexed exact-version lookup, buffers,
restored arenas, and recursive dependency lifecycles. It publishes selected
Packages into one `Model::Environment` before source evaluation:

```ttx
resolve Math : Perimortem.Math = "1.2";
source Types : Library = "library/types.ttx";
```

That container resolution binds the Alias `Math` to exactly
`Perimortem.Math` 1.2. The quoted version is parsed as two unsigned components,
never as a Float or Real. Every member Source borrows the same context and owns
no dependency edges. A standalone interpreter or REPL constructs the identical
Environment dynamically without fabricating a package manifest or a Source
around a restored Package.

Puffer's package Descriptor parses the real `package.ttx` envelope, resolves
the selected and member Dialect names to borrowed `Model::Dialect` objects,
retains descriptor Documentation, and records the exact body token. Tests and
callers therefore cannot substitute token classification or raw Dialect strings
for package evaluation.

Public names come from explicitly selected named chains in that package graph.
Puffer does not manufacture `.Type` or `.Addressable` suffixes, allocate route
history, hash signatures, or choose one alias by lexicographic order.

The package builder passes complete semantic and terminal facts to
[`../archiver`](../archiver/). The archive format owns serialization. Puffer
owns when a package is loaded, cached, invalidated, and exposed to another build.

Terminal materialization is a separate Puffer filesystem transaction. It
publishes a completed collection beneath
`<packages-root>/<package-name>/<major>.<minor>/` only after every relative path
and write has succeeded.

The current implementation still contains objects from the earlier Type-centric
model. They are migration inputs, not contracts that the new TTX model must
preserve.
