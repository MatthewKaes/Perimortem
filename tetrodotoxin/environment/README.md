# Environment

`tetrodotoxin/environment` owns the semantic construction environment shared
by completed Tetrodotoxin Sources. The complete folder builds as
`//tetrodotoxin:environment`.

Environment is not a source parser or another source lifetime wrapper.
`Tetrodotoxin::Language::Source` remains the root of one authored stream, its
Tokenizer, its Arena, and its concrete Abstract root.

## Workspace

`Environment::Workspace` owns identities that must remain stable across one
graph construction transaction:

1. the common scalar Type identities;
2. the immutable Generic formulas;
3. the append only Generic materializations; and
4. local Alias bindings used by source resolution.

Binding returns either the real committed Alias or an exact Workspace error.
An invalid name, Invalid target, or duplicate binding never becomes a durable
graph edge.

Workspace does not own confined filesystem access. That remains
`Package::Workspace`. It also does not own source text, Tokens, parser
dispatch, or a Library specific semantic mirror.

## Namespace

`Environment::Namespace` is the durable named resolution context for retained
definitions. It keeps the complete root surface separate from the deliberately
published export surface.

Namespace construction and mutation return either the same Namespace or an
exact error. Publication is atomic. Private roots do not leak through
`resolve_context()`, exposed writable state publishes its real read only edge,
and Static Callables retain their invocation lookup surface.

`seal()` is the immutable consumer boundary. It validates every retained and
published edge before committing. A failed seal leaves the transaction mutable
so later bindings may complete and validation may be retried. Invalid or
unresolved edges cannot enter a sealed Namespace.

## Source ownership

The owner chain is:

```text
Environment::Workspace
  coordinates shared semantic identities and bindings

Language::Source
  owns one Arena, path, text, Tokenizer, and concrete root

concrete Dialect root
  owns declarations, definitions, and Dialect resolution policy

Environment::Namespace
  retains and publishes completed semantic edges
```

There is no separate Environment Container. Splitting the Arena from Source
would allow a live root to outlive the memory that backs it.

## Future Graph

The eventual `Environment::Graph` will finalize completed Source roots,
resolved Package inputs, and selected product roots into one immutable query
surface. It must reuse Namespace and Workspace identities rather than copy
them into a shadow model.

Graph publication will require every reachable owner to seal its mutable
surfaces, validate public signatures and cross owner edges, and reject Invalid
from committed data. Graph encoding and source free restoration remain
unimplemented.

## Dependency direction

```text
Environment -> TTX
Library ----> Environment
Package ----> Language -> TTX
```

Language never sees Environment. Package supplies source bytes and durable
entries without reconstructing the Environment graph. Concrete Dialects
consume Environment only when their implementation needs the shared
transaction.

Environment owns no parser map, Cursor bookmark, package repository search,
archive Manifest, target record, linker object, runtime cell, or compatibility
format.
