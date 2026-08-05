# TTX Semantics

This document defines the normative version 1 TTX lexical and semantic
contracts. TTX is host neutral. Concrete language, source, package, filesystem,
compiler, linker, runtime, archive, and diagnostic policy belongs to the
systems that consume it.

## Formal model

TTX has two layers:

1. A Lexer maps authored bytes to an ordered Token stream whose Codes expose
   stable local categories.
2. A consumer interprets those Tokens and may construct shared semantic
   identities and identity free supporting values.

TTX requires no universal parse tree or semantic pass between those layers. A
consumer may construct durable facts directly while consuming Tokens. Cursor
positions and Token indexes never become unfinished semantic meaning.

## Token bytecode

A concrete Lexer emits one Token for every recognized source span. Each Token
carries an eight bit `Lexical::Code`, an offset, a line, a column, and a source
span size. `Terminal` reserves `0x00`. `Unknown` reserves `0xFF`. Every other
Code belongs to the exact Lexer and Lexicon contract that assigned it.

The Tokenizer appends one zero length `Terminal` at the source end boundary.
Its offset equals the source byte count. A default `Token()` is the same empty
terminal shape at zero coordinates. `Cursor::peek` accepts a signed relative
offset and returns that empty Token when either stream boundary would be
crossed. A consumer may construct a completed range from its retained opening
Token through `peek(-1)`, so it never fabricates a Token merely to recover the
current source boundary.

`Cursor::branch` creates a speculative position over the same immutable Token
stream. A branch may retain its own Errors owner so provisional diagnostics do
not enter the parent transaction. Only `Cursor::join` publishes a branch
position, and consumers join only after the complete parse transaction
succeeds.

A Code stream is not an independent stable serialization. It can be
interpreted only with the matching Lexer contract and the source bytes needed
to project payload text.

Tokens are decoded source spans, not fixed width language instructions. One
instruction may consume one Token or an ordered Token span according to the
selected consumer. Any authored byte stream can be tokenized. Source that is
neither ignored spacing nor recognized syntax emits `Unknown` rather than
silently disappearing.

### Lexical categories

The common source categories use `snake_case` for an Addressable shaped name
and `PascalCase` for a Type shaped name. Fixed grammar words have dedicated
keyword Codes, while publication and evaluation words have dedicated modifier
Codes. An Attribute uses `@name`. A byte literal uses `0x[...]`, and an
embedded resource operand uses `$[...]`. Punctuation and operators have
dedicated delimiter or operator Codes. Comment lines form their own category.

The Lexicon owns fixed spellings and the exact variable spelling of every
common lexical category. It validates one complete authored spelling against a
Code and can admit explicit separator Codes between repeated category values.
Payload bearing Tokens retain coordinates into the source view borrowed by the
Tokenizer rather than copying their text.

TTX defines the category of an embedded resource operand, but not how its bytes
are loaded. It defines the category of a Dialect marker, but not which names a
host accepts.

## Semantic graph

The shared semantic model is a directed graph of `Concept::Abstract` objects.
An object may be reachable through several bindings or Alias edges, so it does
not store one authoritative parent path.

The closed version 1 identity categories have distinct contracts:

* `Abstract` provides semantic identity, a local name, Documentation, category
  proof, and contextual resolution.
* `Invalid` provides the absorbing semantic failure identity.
* `Alias` provides a local identity that redirects resolution to one borrowed
  target.
* `Type` provides a domain whose successful resolution exposes one total
  Layout.
* `Value` provides a leaf Type with no contextual subdomains.
* `Flag` provides the Value contract for a binary logical domain.
* `Real` provides the Value contract for a floating point domain.
* `Signed` provides the Value contract for a signed integer domain.
* `Unsigned` provides the Value contract for an unsigned integer domain.
* `Addressable` provides a named address whose edge reaches one Type.
* `Callable` provides an invocable identity with complete parameter and result
  Layouts.

`Documentation`, `Layout`, `Reference`, and `Attribute` are identity free
supporting contracts and values. They do not inherit `Abstract` merely to gain
discovery.

There is no universal Kind, class database, mutable semantic registry, copied
Type tree, nullable member record, or mandatory reflection field.

## Category proof

Every declared semantic category owns a stable
`Perimortem::System::Uuid`. An implementation recognizes its declared category
and delegates unrecognized identifiers through its public base chain.

Category identifiers identify interfaces only. They are not object identities,
cache keys, path hashes, package versions, or serialized handles.

Native consumers use:

```text
abstract.is<Category>()                 → Bool
abstract.visit<Category>(match, mismatch)
```

`is()` proves the public category. `visit()` calls the match function with the
real category or the mismatch function with the exact `const Abstract&`.
Ordinary input mismatch never traps or exposes an unchecked reference.

An implementation may report only categories represented by its public C++
inheritance. Implementation reuse does not manufacture another semantic fact.

## Abstract resolution

Every Abstract provides:

```text
get_name()                 → borrowed Bytes
get_documentation()        → const Documentation&
resolve()                  → const Abstract&
resolve_context(route)     → const Abstract&
```

`resolve()` returns the represented identity. `resolve_context()` gives the
complete borrowed route to the receiving Abstract. That Abstract owns route
interpretation and may consume a prefix, pass a suffix, or redirect the
unchanged view. TTX prescribes no separator grammar, traversal object, or
resolution cache.

For an unchanged valid graph, `resolve()` is idempotent and every resolution
chain terminates. Failure returns `Invalid`, never a null pseudo Abstract.
Graph enrichment may refine a query that previously returned Invalid, but it
must not invalidate an earlier correct result.

Diagnostics retain authored input and failed boundaries on the consumer that
performed the query. Abstract does not retain route history for presentation.

## Invalid

`Invalid : Abstract` is the binary wide semantic failure object. It is closed,
stateless, and absorbing. Its name is `Invalid`. Both resolution operations
return the same Invalid identity. Its documentation is empty.

Invalid does not store a failed route, diagnostic, source range, package state,
or recovery choice. Parser failures, construction failures, invalid indexes,
and failed Layout fits use `Option`, `Union`, or another owner specific error
result instead.

## Alias

Alias retains a local name, local visible Documentation, and one borrowed
target. `resolve()` redirects to the target represented identity.
`resolve_context(route)` first resolves the target and then gives that identity
the complete route.

The graph owner preserves target lifetime and rejects cycles before an Alias
becomes queryable.

## Type and Value

`Type : Abstract` represents a semantic domain. Once Type resolution succeeds,
`get_layout()` returns one total `const Layout&`. An empty Layout is a complete
shape and does not by itself prove that the Type is a scalar.

A host may reserve a stable Type before all facts are ready. Until the Type can
answer its contract, its semantic resolution may return Invalid. No incomplete
Layout alternative is required.

`Value : Type` is a leaf domain used for terminal bit interpretations. It has
no contextual subdomains: every `resolve_context(route)` returns Invalid.
Ordinary `resolve()` remains the represented Value identity.

Value adds:

```text
get_width()       → Count
get_size()        → Count
get_alignment()   → Count
```

Width describes the value domain. Size and alignment describe storage. TTX
defines the `Flag`, `Real`, `Signed`, and `Unsigned` domain interfaces.
Concrete widths, names, representations, and installed instances belong to
the language or system that constructs them.

## Addressable

`Addressable : Abstract` is a named address to typed data:

```text
get_type()        → const Type&
```

A field, receiver, local, external symbol, interpreted endpoint, or runtime
object may implement Addressable while exposing richer owner specific
capabilities.

Contextual resolution delegates through the represented Type. Assignment,
mutation, storage duration, physical address, and access policy are not part of
the shared Addressable contract.

## Callable

`Callable : Abstract` supplies one complete signature:

```text
get_parameters()  → const Layout&
get_results()     → const Layout&
```

The parameter and result Layouts are consumed by fitting, reflection,
invocation, and lowering. Callable does not prescribe a receiver, executable
body, linkage, machine address, calling convention, or target ABI.

## Layout

`Layout` is the identity free directional fitting contract over an ordered
group of real Abstracts:

```text
get_size()                    → Count
get_abstract(index)           → Option<const Abstract&>
fits(target)                  → Bool
fits_at(target, offset)       → Bool
get_fitted(target, index)     → Union<const Abstract&, Layout::Errors>
get_fitted_at(target, offset, index)
                              → Union<const Abstract&, Layout::Errors>
is_empty()                    → Bool
```

The closed errors are `IndexOutOfBounds`, `SizeMismatch`, and
`IncompatibleFit`. An invalid index has no value. Layout failures do not inject
Invalid into the semantic graph.

TTX supplies five common implementations:

* `Fluid` fits ordered source values by represented identity. An Addressable
  target compares through its resolved Type.
* `Named` fits uniquely named source values by name and the same represented
  identity rule.
* `Structured` retains and fits the exact Addressable identities owned by a
  Type.
* `Ranged` repeats one real Abstract over a fixed interval and uses the Fluid
  identity rule.
* `Composite` combines two complete Layouts without flattening them.

Fitting is directional: the source fits the target. The complete operations
require equal sizes. The segmented operations fit a source into one target
interval so Composite can preserve each child contract. `get_fitted()` and
`get_fitted_at()` return the original source edge that supplies one target
slot.

Layout stores no copied field name, Type, Documentation, Attribute, default,
offset, storage class, or target representation. Those facts remain on their
real semantic or target owners. Structural coincidence does not create Type
identity.

## Documentation

`Documentation` is an identity free ordered view of presentation lines. Every
Abstract returns one stable Documentation reference. Missing documentation is
the shared empty Documentation object.

```text
get_line(index)       → borrowed Bytes
line_count()          → Count
is_empty()            → Bool
```

Implementations may borrow authored lines, generate stable prose, or compose
several Documentation objects. The object and every borrowed line remain valid
for as long as the exposing Abstract is queryable.

`Documentations::Comment` exposes one generated line. `Block` exposes an
ordered authored sequence and preserves empty lines. `Merged` stacks two
complete Documentation values without copying their lines.

Documentation does not participate in resolution, semantic identity, Type
identity, Layout identity, or fitting.

## Reference

`Reference<Category>` is a nonnull borrowed semantic edge suitable for
contiguous views and tagged unions. It preserves the exact object supplied by
its owner. Resolution remains an explicit consumer operation, and the graph
owner guarantees the borrowed lifetime.

## Attribute

`Attribute` is an identity free key and optional scalar value. Its closed value
alternatives are borrowed `Bytes`, `Unsigned_64`, `Signed_64`, `Real_64`, and
`Bool`.

```text
get_key()             → borrowed Bytes
get_value()           → const Attribute::Value&
has_value()           → Bool
is_empty()            → Bool
```

Attribute does not define interpretation, publication, semantic identity, or a
structured metadata model. Attribute equality compares the key and closed
scalar value. The owner that exposes an Attribute gives that key and value
meaning.

## Derived consumers

Concrete languages may define expressions, bindings, projections, constants,
generic formulas, mutation capabilities, receiver roles, executable bodies,
concrete scalar Types, and publication policy. Such objects retain real TTX
Type, Layout, Addressable, and Callable edges without entering the shared TTX
model.

Target representations may contain offsets, alignments, pointer forms, address
spaces, storage classes, register classes, ABI carriers, and executable
addresses. Runtime systems may contain managed cells, frames, collectors,
workers, and scheduling state. These derived records are not TTX semantic
identities.

Source caches, package manifests, filesystem roots, archives, diagnostics, and
durable formats remain outside TTX.

## Version 1 invariants

1. Every emitted Token has exactly one Code and one source span.
2. A Token stream is interpreted only with its exact Lexer contract.
3. Payload text is projected through the source view borrowed by the
   Tokenizer.
4. Unrecognized source emits `Unknown` rather than disappearing.
5. Cursor positions and Token indexes never stand in for unfinished semantic
   facts.
6. Every queryable semantic identity implements Abstract.
7. Documentation, Layout, Reference, and Attribute remain identity free.
8. Semantic query failure returns Invalid, never a null pseudo Abstract.
9. Later graph enrichment never invalidates an earlier correct query result.
10. Alias preserves local identity while redirecting represented identity.
11. Value has no contextual subdomains.
12. Addressable reaches one Type.
13. Callable supplies complete parameter and result Layouts.
14. Layout owns order and directional fitting, not copied semantic or physical
    records.
15. Concrete language, source, package, target, runtime, linker, archive, and
    diagnostic policy remain outside TTX.
16. Terminal Tokens have zero length, and Cursor provides bounded signed
    relative Token lookup without moving its parse position.
17. Cursor branches share their exact immutable Token stream, and only an
    explicit join after successful interpretation publishes branch position.
