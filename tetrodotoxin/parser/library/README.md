# Library Parser

The Library parser owns the concrete grammar for reusable CPU-side Types,
Addressables, constants, Callables, and executable Bodies. It consumes TTX
Tokens into the real semantic owners supplied by the Library model.

It does not own source lifetime, package membership, resource files, native
providers, runtime cells, target lowering, or archives.

## Document and member order

A complete Library document starts with Documentation and
`dialect : Library;`.

Library members are presented in this order:

1. published Type-like and addressable definitions;
2. public functions;
3. private Type-like and addressable definitions; and
4. private functions.

`public` and `expose` addressables retain authored order. Unpublished
Library-owned `state` belongs to the private-addressable group. Duplicate names
and shadowing are rejected against the active semantic context.

This order is not permission to retain later token ranges. Every definition,
initializer, and function body is consumed once into its semantic owner. A
later completion phase may walk those owners to resolve and seal cross-
references. Unsupported forward visibility is rejected until a durable
semantic representation can carry it.

## Definition forms

Library admits the common publication and evaluation prefixes plus its concrete
continuations:

```ttx
public Packet : struct {
  public width : Unsigned_64 = 0;
}

public Session : object {
  expose state progress : Unsigned_64 = 0;
}

public PacketAlias : alias = Packet;
public const signature : Fixed[Unsigned_8, 4] = 0x[54 54 58 31];
```

`struct` constructs an inline Type. `object` constructs a Type that proves
Managed. `alias` constructs the shared Alias model. `state` constructs writable
storage on the Library owner selected by context. `const` must complete
compile-time evaluation rather than creating write-once runtime storage.

The parser passes each accepted form directly to its real Type, Addressable,
Alias, Constant, or Callable owner. It does not construct a generic Member
record or store a visibility Kind on the result.

## Functions and Body

Library functions construct Static or Self Callables with complete parameter
and result Layouts. Bare `self` is accepted only at parameter zero and expands
to the ordinary receiver entry in the Callable's parameter Layout.

```ttx
public func identity[.value : Unsigned_64] -> Unsigned_64 {
  return value;
}

public func identity[self] -> Unsigned_64 {
  return self.width;
}
```

The body is consumed directly into the common TTX Body representation. It is
not retained as source Tokens, a Cursor range, a statement AST, or a second
Library-specific executable tree.

A bodyless ordinary Library Callable is invalid. A body may be omitted only
when a concrete continuation supplies the complete implementation contract.

## Embedded resources and Foreign

`$[...]` is a TTX embedded-resource Token. Library literal parsing asks the
Source's Environment for the corresponding byte snapshot and constructs the
Bytes Constant or reports a diagnostic. It never opens a path itself.

The [Foreign parser](../foreign/) is an explicitly admitted embedded Dialect.
Foreign declarations do not become Library exports merely because Library
accepts their syntax.

## Status

The current tree contains authored Library fixtures and shared Type parsing,
but no complete Library parser or evaluator target. The fixtures define design
pressure; they do not establish semantic execution.
