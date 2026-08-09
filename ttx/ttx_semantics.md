# TTX Semantics

This document defines the normative version 1 TTX lexical and semantic
contracts. TTX is host neutral. Concrete languages define their grammar and
type systems; package, compiler, target, and runtime policy belong to the
systems that consume the resulting graph.

## Formal model

TTX has two layers:

1. A Lexer maps authored bytes to an ordered Token stream.
2. A consumer interprets those Tokens and may construct shared semantic
   identities and supporting values.

The consumer may construct durable facts directly while reading Tokens. TTX
requires no universal syntax tree between the lexical stream and the semantic
graph.

## Lexical contract

Each Token carries an eight bit `Lexical::Code`, source offset, line, column,
and span size. `Terminal` is `0x00`; `Unknown` is `0xFF`. ***Every other Code belongs***
***to the exact Lexer and Lexicon contract that emitted it.***

The Tokenizer appends one zero length Terminal at the end of the source. Its
offset equals the source byte count. `Cursor::peek(relative)` observes a signed
relative position without moving the Cursor and returns an empty Terminal when
the requested position lies outside the stream.

`Cursor::branch()` creates a speculative position over the same immutable Token
stream. `Cursor::join(branch)` publishes that position. A rejected branch leaves
the parent Cursor unchanged.

`Lexical::Span` identifies a complete authored range. `Lexical::Anchor` pairs
that range with the independent Token a diagnostic should emphasize. An Anchor
created from only a Span focuses its opening Token. A synthetic semantic fact
has no Anchor.

A Code stream is meaningful only with the Lexer contract and source bytes that
produced it. Tokens are decoded source spans, not an independent serialized
program.

### Common categories

TTX provides a common Lexicon optimized for fast lexing alongside a rich,
generic semantic interface suitable for a wide range of interpreters.

An Addressable-shaped source name uses `snake_case`; a Type-shaped name uses
`PascalCase`. Fixed grammar words, modifiers, delimiters, and operators have
dedicated Codes. Attributes use `@name`, byte literals use `0x[...]`, and
embedded resources use `$[...]`.

The lexer preserves common operator spellings as distinct Codes:

| Code            | Spelling |
| --------------- | -------- |
| `AddressOp`     | `.`      |
| `TypeAccessOp`  | `::`     |
| `CallOp`        | `->`     |
| `SwizzleOp`     | `.[`     |
| `ValueAccessOp` | `:[`     |

`BracketStart` and `BracketEnd` retain the shared delimiter Codes. A concrete
language may assign different grammar roles to the same delimiter according to
parser context; TTX does not add another lexical spelling for each use.

The Code names do not prescribe a shared expression grammar or result category.
A concrete language assigns grammar and result contracts and proves every TTX
category it consumes.

## Semantic graph

The shared semantic model is a directed graph of `Concept::Abstract` objects.
One object may be reachable through several local bindings or Alias edges and
therefore has no required parent path.

The closed version 1 identity categories are:

- `Abstract` provides identity, a local name, Documentation, category proof,
  and contextual resolution.
- `Invalid` is the absorbing semantic failure identity.
- `Alias` provides a local identity that redirects to one borrowed target.
- `Type` provides a semantic domain and one total Layout.
- `Value` provides a leaf Type with no contextual subdomains.
- `Flag`, `Real`, `Signed`, and `Unsigned` refine Value domains.
- `Addressable` provides a named address whose edge reaches one Type.
- `Callable` provides complete parameter and result Layouts.

`Documentation`, `Layout`, `Reference`, and `Attribute` are identity-free
supporting contracts and values.

These categories are an interchange vocabulary rather than a complete type
system. Concrete languages define their Type inventory, access policy,
mutation, construction, and invocation roles.

## Resolution and category proof

Every Abstract provides these total queries:

```text
get_name()                 -> borrowed Bytes
get_documentation()        -> const Documentation&
resolve()                  -> const Abstract&
resolve_context(route)     -> const Abstract&
```

`resolve()` returns the represented identity. `resolve_context(route)` gives a
borrowed route to the receiving identity, which interprets it according to its
own domain. For an unchanged valid graph, resolution is idempotent and every
chain terminates.

An unanswered semantic query returns `Invalid`, never a null graph edge. Later
construction may answer a query that formerly returned Invalid, but it does not
replace an identity that was already returned successfully.

Category proof establishes a public semantic interface on the real object.
`is<Category>()` answers the Boolean proof and `visit<Category>(match,
mismatch)` dispatches to the proven category or the original Abstract.

## Invalid

`Invalid : Abstract` is stateless and absorbing. Its name is `Invalid`; both
resolution operations return the same Invalid identity; its Documentation is
empty.

Invalid stores no failed route, source range, diagnostic, or recovery choice.
Parser rejection and failed Layout fitting use the result contract of the
operation that failed. Other nonsemantic outcomes remain values of their
concrete owners rather than Invalid identities.

## Alias

Alias retains a local name, local Documentation, and one borrowed target.
`get_target()` returns that exact immediate edge without following another
Alias or asking whether the target is complete.
`resolve()` returns the target's represented identity.
`resolve_context(route)` forwards the complete route through that represented
identity.

The graph owner preserves target lifetime and prevents Alias cycles from
becoming queryable.

## Type and Value

`Type : Abstract` represents a semantic domain. Once resolution succeeds,
`get_layout()` returns one complete `const Layout&`. An empty Layout is a valid
shape and does not alone establish scalar identity.

`Value : Type` is a leaf domain used for terminal bit interpretations. Its
contextual resolution always returns Invalid. Value provides width, size, and
alignment queries; concrete domains and representations belong to the language
that constructs them.

TTX defines the `Flag`, `Real`, `Signed`, and `Unsigned` Value interfaces.

## Addressable

`Addressable : Abstract` is a named semantic address to typed data:

```text
get_type()                 -> const Type&
```

A concrete graph object may implement Addressable while adding capabilities
owned by its language. TTX defines only the named edge to one Type.

Contextual resolution, Layout selection, or another consumer operation may
return an Addressable. TTX does not prescribe its physical address or target
representation, and later realization does not change the selected identity.

Assignment, writability, storage duration, and visibility are concrete language
policy rather than part of the shared Addressable contract.

## Callable

`Callable : Abstract` supplies one complete signature:

```text
get_parameters()           -> const Layout&
get_results()              -> const Layout&
```

TTX does not prescribe how a Callable is selected or invoked. A concrete
language may fit its parameter and result Layouts while adding receiver,
executable body, calling convention, machine address, or target ABI policy.

## Layout

Layout is the identity-free directional fitting contract over an ordered group
of real Abstracts:

```text
get_size()                         -> Count
get_abstract(index)                -> Option<const Abstract&>
fits(target)                       -> Bool
fits_at(target, offset)            -> Bool
get_fitted(target, index)          -> Abstract or Layout::Errors
get_fitted_at(target, offset, index)
                                   -> Abstract or Layout::Errors
is_empty()                         -> Bool
```

The closed fitting errors are `IndexOutOfBounds`, `SizeMismatch`, and
`IncompatibleFit`. A failed fit does not add Invalid to the semantic graph.

TTX supplies four common implementations:

- `Fluid` fits ordered entries by represented identity. An Addressable target
  participates through its Type.
- `Named` fits nonempty unique names by name and represented identity.
- `Ranged` repeats one real entry over a fixed interval and applies Fluid
  fitting.
- `Composite` combines two complete Layouts without flattening them.

Fitting is directional: the source supplies the target. Complete fitting
requires equal sizes; segmented fitting places a source in one target interval.
Successful fitted queries return the original source edge that supplies the
target position.

A consumer may select one real Layout entry and prove the category required by
its own operation. `Named` provides name based fitting without defining a
universal lookup or member interface.

Layout retains no copied semantic record, target offset, storage class, ABI
rule, or anonymous Type identity. Structural coincidence does not create Type
identity.

## Documentation

Documentation is an identity-free ordered view of presentation lines. Every
Abstract returns one stable Documentation reference; missing documentation is
the shared empty Documentation value.

`Documentations::Comment` presents one generated line, `Block` presents an
ordered authored sequence, and `Merged` composes two complete Documentation
values. Documentation does not participate in identity, resolution, or Layout
fitting.

## Reference

`Reference<Category>` is a nonnull borrowed semantic edge. It preserves the
exact object and its const qualification. The graph owner guarantees that the
borrowed identity outlives the Reference.

## Attribute

Attribute is an identity-free key with an optional scalar value. Version 1
values are borrowed `Bytes`, `Unsigned_64`, `Signed_64`, `Real_64`, and `Bool`.
The owner that exposes an Attribute defines its meaning.

## Consumer boundary

Concrete languages may define expressions, constants, generic formulas,
mutation capabilities, receiver roles, executable bodies, and concrete scalar
Types. They retain real TTX Type, Layout, Addressable, and Callable edges.

Targets may derive sizes, offsets, pointer forms, address spaces, registers,
ABI carriers, and executable addresses. Runtimes may add managed storage,
frames, collectors, and scheduling state. These are consumers of the semantic
graph rather than additional TTX categories.

## Version 1 invariants

1. Every emitted Token has one Code and one source span.
2. A Token stream is interpreted with the Lexer contract and source bytes that
   produced it.
3. Unrecognized authored bytes emit `Unknown` rather than disappearing.
4. Every semantic identity implements Abstract.
5. Semantic query failure returns Invalid rather than a null edge.
6. Later construction never changes an identity already returned successfully.
7. Alias preserves local identity while redirecting represented identity.
8. Value has no contextual subdomains.
9. Addressable reaches one Type; Callable supplies complete parameter and
   result Layouts.
10. Type, Addressable, Callable, and Layout remain independent contracts; TTX
    defines no universal member model over them.
11. Layout owns order and directional fitting, not copied semantic or physical
    records.
12. Concrete language, package, target, runtime, and diagnostic policy remain
    outside TTX.
