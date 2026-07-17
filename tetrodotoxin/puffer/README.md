# Puffer

Puffer is Tetrodotoxin's command-line and language-server host. It loads complete
TTX sources, evaluates the Boot preamble, resolves source and package imports,
selects body ISAs, and requests terminal products from the compiler.

Puffer owns orchestration. It does not own the TTX semantic contracts, backend
ABI rules, linker formats, or a parallel Type model.

## Build transaction

A build begins from explicit source roots, package inputs, and the ISAs installed
by the active toolchain. Boot reads the source envelope. The resolver binds the
import closure. The selected body ISA evaluates the remaining token bytecode and
constructs Abstract-derived objects inside the build boundary.

Puffer does not supply a ClassDB, allocated Route model, or global semantic
registry. ISA installation selects evaluators. Semantic lookup still occurs
through the Abstract graph and borrowed `View::Bytes` routes.

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
Package imports bind a local name to the resolved Source root. That root is a
named Abstract context, not a Type with an empty Layout.

Public names come from explicitly selected named chains in that package graph.
Puffer does not manufacture `.Type` or `.Addressable` suffixes, allocate route
history, hash signatures, or choose one alias by lexicographic order.

The package builder passes complete semantic and terminal facts to
[`../archiver`](../archiver/). The archive format owns serialization. Puffer
owns when a package is loaded, cached, invalidated, and exposed to another build.

The current implementation still contains objects from the earlier Type-centric
model. They are migration inputs, not contracts that the new TTX model must
preserve.
