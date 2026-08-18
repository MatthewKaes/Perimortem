# TTX Semantics

This document is the normative contract for implementers of TTX hosts
and semantic objects. Readers evaluating TTX should begin with the
[overview](README.md). The [design document](ttx_design.md) explains the
rationale and tradeoffs behind these rules.

TTX is independent of its host. Concrete languages define their grammar and
type systems. Package, compiler, target, and runtime policy belong to the
systems that consume the resulting graph.

A host conforms by preserving the categories, identity relations, total
queries, and lifetime rules specified here. Conformance does not require one
internal object layout or graph shape.

## Formal model

TTX has two layers:

1. A Lexer maps authored bytes to an ordered Token stream.
2. A consumer interprets those Tokens and may construct shared semantic
   identities and supporting values.

The consumer may construct retained semantic facts directly while reading
Tokens. TTX requires no universal syntax tree between the lexical stream and
the semantic graph.

The semantic graph is live owner state. When a consumer emits an independent
representation, that output is a Terminal product and lies outside the graph.
A later consumer cannot query the output as TTX semantics. It must validate the
format and reconstruct new semantic identities through a graph owner when the
format carries enough facts to support reconstruction.

## Lexical contract

Each Token carries an eight bit `Lexical::Code`, source offset, line, column,
and span size. `Terminal` is `0x00` and `Unknown` is `0xFF`.
Every other Code is interpreted by the exact Lexer and Lexicon contract that
emitted it.

The Tokenizer appends one zero length Terminal at the end of the source. Its
offset equals the source byte count. A Cursor can observe a signed relative
position without moving. Observation outside the stream returns an empty
Terminal.

A Cursor is the one mutable position over the immutable Token stream. Grammar
dispatch proves the selected production before its parser consumes that Cursor.
a rejected production retains its diagnostics and the source transaction owns
discarding any candidate semantic state.

A source transaction constructs one `Associations` index and supplies it to the
Cursor. A consumer records an authored Anchor with the exact semantic identity
it constructs there. The index retains those borrowed associations after the
mutable Cursor completes, while sharing the source transaction lifetime of the
graph. It creates no semantic edge and cannot be serialized or used after that
graph owner releases the transaction.
When several Anchors contain one source byte, an exact focus Token is more
precise than a containing Span and the narrower range is more precise within
the same class.

`Lexical::Span` identifies a complete authored range. `Lexical::Anchor` pairs
that range with the independent Token a diagnostic should emphasize. An Anchor
created from only a Span focuses its opening Token. A synthetic semantic fact
has no Anchor.

A Code stream is meaningful only with the Lexer contract and source bytes that
produced it. Tokens are decoded source spans, not an independent serialized
program.

### Common categories

TTX defines a common Lexicon for spellings shared by concrete languages. The
Lexicon and semantic interfaces remain separate, so a Token Code never chooses
one universal grammar or semantic result.

A source name for an Addressable uses `snake_case`. A source name for a Type
uses `PascalCase`. Fixed grammar words, modifiers, delimiters, and operators
have dedicated Codes. Attributes use `@name`, byte literals use `0x[...]`, and
embedded resources use `$[...]`.

The lexer preserves common operator spellings as distinct Codes:

| Code            | Spelling |
| --------------- | -------- |
| `AddressOp`     | `.`      |
| `TypeAccessOp`  | `::`     |
| `CallOp`        | `->`     |
| `SwizzleOp`     | `.[`     |
| `ValueAccessOp` | `:[`     |
| `QuestionOp`    | `?`      |

`BracketStart` and `BracketEnd` retain the shared delimiter Codes. A concrete
language may assign different grammar roles to the same delimiter according to
parser context. TTX does not add another lexical spelling for each use.

The Code names do not prescribe a shared expression grammar or result category.
A concrete language assigns grammar and result contracts and proves every TTX
category it consumes.

## Semantic graph

The shared semantic model is a directed graph of Abstract identities. One
identity may be reachable through several local bindings or Alias edges and
therefore has no required parent path.

The closed identity categories are:

* `Abstract` provides identity, a local name, Documentation, category proof,
  and contextual resolution.
* `Invalid` is the absorbing semantic failure identity.
* `Alias` provides a local identity that redirects to one borrowed target.
* `Type` provides a semantic domain and one total Layout.
* `Addressable` provides a named address whose edge reaches one Type.
* `Pack` provides produced value flow and one total output Layout.
* `Callable` provides complete parameter and result Layouts.

`Documentation`, `Layout`, and `Reference` are supporting contracts and values
that carry no semantic identity.

These categories are an interchange vocabulary rather than a complete type
system. Concrete languages define their Type inventory, access policy,
mutation, construction, and invocation roles.

A Terminal product is not an identity category. `Lexical::Code::Terminal` is
the end marker for a Token stream. Neither term introduces another semantic
identity.

## Resolution and category proof

Every Abstract exposes its local name, its Documentation, its represented
identity through `resolve()`, and owner directed contextual identity through
`resolve_context(name)`. Each observation is total.

`resolve()` returns the represented identity. `resolve_context(name)` gives one
borrowed name to the receiving identity, which interprets it according to its
own domain. Concrete language operators split qualified syntax and query the
identity selected by each preceding name. They never flatten a qualified route
into one lookup key. For an unchanged completed graph, resolution is idempotent
and every chain terminates.

Context lookup, receiver access, and invocation are distinct semantic queries.
An explicit receiver asks `resolve_access(host, name)` or
`resolve_call(host, name)` with the original caller authority and one borrowed
name. These Abstract hooks let an arbitrary graph context route a concrete
language question without acquiring that language's Type system. TTX assigns
them no receiver role, visibility rule, or forwarding policy. A concrete
language may refine its Type and Addressable contracts to interpret the query.
A Type-qualified context route still splits into names and uses
`resolve_context(name)` on each selected identity. TTX defines no separate Type
lookup or Static and Self distinction. Every query returns the original
selected identity or Invalid and never searches another query domain as a
fallback.

An unanswered semantic query returns `Invalid`, never a null graph edge. Later
construction may answer a query that formerly returned Invalid, but it does not
replace an identity that was already returned successfully.

Category proof establishes the semantic contract of the original object. A
proof never creates a wrapper, clone, registry entry, or substitute identity.
The consumer states the category it needs and either receives that same object
under the proven contract or retains the original Abstract.

Source provenance, declaration structure, visibility, and publication remain
facts of the concrete language owner. Abstract exposes no generic declaration
projection. A consumer that needs those facts proves the concrete owner and
inspects its complete declaration value.

## Invalid

Invalid is a stateless and absorbing Abstract. Its name is `Invalid`. Every
resolution, access, and invocation query returns the same Invalid identity. Its
Documentation is empty.

Invalid stores no failed route, source range, diagnostic, or recovery choice.
Parser rejection and failed Layout fitting use the result contract of the
operation that failed. Other nonsemantic outcomes remain values of their
concrete owners rather than Invalid identities.

## Alias

Alias retains a local name, local Documentation, and one borrowed target. The
immediate target is opaque: consumers cannot inspect or bypass an Alias edge.
`resolve()` is the sole traversal operation. It follows only Alias edges and
returns the first non-Alias target identity without invoking that target's own
`resolve()` operation. Every other operation, including
`resolve_context(name)`, returns Invalid. A consumer that needs context first
resolves the Alias, proves the returned owner, and invokes that owner's
operation explicitly.

A concrete graph owner may reserve an Alias identity before its target is
known. An unbound Alias resolves to Invalid, and its target may be bound only
once. Repeating the same binding is harmless while changing it fails. The
owner preserves target lifetime and prevents Alias cycles before publishing
the completed graph.

## Type

Type is an Abstract that represents a semantic domain. Once resolution
succeeds, each Type exposes one complete Layout. A Type admitted to ordinary
value flow has at least one Layout entry. A Type with an empty Layout may own
contextual facts, but it cannot be instantiated, produced, or named by an
Addressable. Empty Layouts fit without creating a shared Type identity.

A value Layout terminates when recursively following its real Type entries and
the Types named by its Addressable entries reaches terminal `Value` leaves. An
atomic Type's exact self entry is one such leaf. Returning to an active
structural Type or reaching an empty child Layout is not a complete value
shape. Completion validates this graph property before a concrete language
constructs, lowers, or stores the value.

Every completed concrete Type admitted to ordinary value flow has one total
semantic default. The concrete language owns the
default value and the operation that materializes it. TTX does not infer that
value from an all-zero target representation, add a default query to Type, or
require different Types with equivalent defaults to share identity. A semantic
Type selected for contextual traversal remains outside value flow unless a
concrete language operation produces an instance of it.

An empty Layout has no value to default. An empty View is different because it
is one value of the exact View Type and still contributes that Type to its
Pack's Layout.

The identity-free terminal `Value` Layout has one entry containing its exact
atomic Type. Atomic identity therefore participates in ordinary Layout fitting
instead of being inferred from an otherwise empty shape. Scalar families,
logical and numeric refinements, bit width, abstract machine storage, and
alignment belong to the concrete language that constructs those Types. They
are not additional host-neutral TTX categories.

## Addressable

Addressable is an Abstract that names typed data. It reaches one exact Type
whose Layout contains at least one value. A zero-value Type remains a valid
semantic domain, but there is no value whose stable address an Addressable
could name.

A concrete graph object may be Addressable while adding capabilities
owned by its language. TTX defines only the named edge to one Type.

TTX does not make an Addressable forward context, receiver access, or
invocation queries to that Type. A concrete language may add that behavior when
its own receiver model requires it.

Contextual resolution, Layout selection, or another consumer operation may
return an Addressable. TTX does not prescribe its physical address or target
representation, and later realization does not change the selected identity.

Assignment, writability, storage duration, and visibility are concrete language
policy rather than part of the shared Addressable contract.

## Pack

Pack is an Abstract that carries one produced value flow. It is not a Type,
Addressable, or Layout. The Pack preserves the exact producer identity while
its output Layout describes the values that producer supplies. A consumer can
therefore retain, link, inspect, fit, or lower a value flow without first
materializing an aggregate Type.

A Pack may supply zero, one, or several values. It may expose positional, named,
ranged, or composed output shape. One ordinary value-producing expression is
already a one-value Pack. Grouping that expression does not create a second
semantic identity. An empty Pack exposes an empty Layout. A concrete language's
empty result and an explicit empty grouping agree through that Layout without
requiring a Type identity. A multi-value Pack remains value flow until a
receiving contract fits it and an owning language deliberately materializes a
Type.

The common delimiter shapes keep value flow and required shape visually
distinct: parentheses group produced Packs while brackets describe Layouts.
Concrete languages decide which productions may omit those delimiters, but
omission does not change the resulting Pack or Layout contract. Named Pack
slots use `.name = expression` and retain those names independently from the
produced semantic objects. Named descriptor slots use `.name : Type`. The
different operator keeps promised shape distinct from supplied value flow.

A graph owner may reserve a stable Pack before its output is complete. Its
Layout query remains safe during that interval and may expose an empty shape,
but the Pack resolves to Invalid. Only a Pack that resolves to itself supplies
an empty Layout as completed zero-value flow. Once Pack resolution succeeds,
its output Layout is stable and never replaced.

## Callable

Callable is an Abstract that supplies one complete signature as a parameter
Layout and a result Layout. TTX assigns no receiver role to any parameter
position or spelling.

TTX does not prescribe how a Callable is selected or invoked. A concrete
language may fit an argument Pack to the parameter Layout and expose the
invocation's result Pack through the result Layout while adding executable body,
calling convention, machine address, or target ABI policy.

## Layout

Layout carries no semantic identity. It describes one promised value shape and
provides ordered observation and directional fitting over exact Abstract
identities. Types and Callables expose required Layouts. Packs expose the output
Layout of the values they supply. A consumer may test a complete fit, test a fit
at an offset, and recover the original source edge that supplies a target
position.

The closed fitting errors are `IndexOutOfBounds`, `SizeMismatch`, and
`IncompatibleFit`. A failed fit does not add Invalid to the semantic graph.

TTX defines five common Layout forms:

* `Value` contains one exact atomic Type as the terminal Layout leaf.
* `Fluid` describes ordered positional entries and fits them by represented
  identity. An Addressable target participates through its Type.
* `Named` describes nonempty unique slot names, matches them by name, then
  preserves the fitting rule of the source Layout for each matched entry. A
  slot may borrow its name independently from the source Abstract without
  renaming or wrapping that Abstract.
* `Ranged` describes one exact entry repeated over a fixed interval and applies
  Fluid fitting.
* `Composite` describes two complete Layouts as one shape without flattening
  them.

Fitting is directional: the source supplies the target. Complete fitting
requires equal sizes. Segmented fitting places a source in one target interval.
Successful fitted queries return the original source edge that supplies the
target position. A Layout that decorates or combines another Layout delegates
entry fitting to the source owner rather than replacing its fitting rules.

A consumer may select one exact Layout entry and prove the category required by
its own operation. `Named` provides name based fitting without defining a
universal lookup or member interface.

Layout retains no copied semantic record, target offset, storage class, ABI
rule, or anonymous Type identity. Structural coincidence does not create Type
identity.

An empty Layout has size zero. It fits another empty Layout and describes no
stable value or address, regardless of which concrete Type or Pack exposes it.

## Documentation

Documentation carries no semantic identity. It presents an ordered view of
presentation lines. Every Abstract returns one stable Documentation reference.
Missing documentation is the shared empty Documentation value.

Documentation may present one generated line, an ordered authored block, or a
composition of two complete Documentation values. It does not participate in
identity, resolution, or Layout fitting.

## Reference

Reference is a nonnull borrowed semantic edge. It preserves the exact object
and exposes no absent state.

Reference does not call `resolve()`, follow Alias targets, prove another
category, or canonicalize structurally equal Types. The consumer performs the
operation required by its own contract. This lets a Layout retain the exact
Addressable selected by its owner while a Generic materialization retains the
exact Type selected for one argument.

The graph owner guarantees that the borrowed identity outlives the Reference.
A process address may identify that object while the owner keeps it stable, but
the address is not a durable semantic name. A Reference cannot be serialized or
carried across a Terminal boundary.

## Consumers and Terminal products

Concrete languages may define expressions, constants, generic formulas,
mutation capabilities, receiver roles, executable bodies, and concrete scalar
Types. Their value-producing expressions participate as Packs and retain exact
TTX Type, Layout, Addressable, and Callable edges.

Targets may derive sizes, offsets, pointer forms, address spaces, registers,
ABI carriers, and executable addresses. Runtimes may add managed storage,
frames, collectors, and scheduling state. These are consumers of the semantic
graph rather than additional TTX categories.

A consumer crosses the Terminal boundary when it emits an output whose
consumption is independent of the live semantic graph and its identities.
Formatted text, editor data, LLVM IR, SPIR-V modules, debug data, object
modules, executables, and semantic archives are examples. Terminal is a
boundary role. TTX defines no universal Terminal category, product registry,
or common byte container.

Each Terminal format belongs to its concrete producer. The format may retain
source presentation, target representation, or reconstruction facts defined by
semantic owners according to that producer's purpose. The Terminal itself is
never an Abstract, Type, Pack, Layout, Addressable, Callable, or Reference.

A target Terminal such as LLVM IR or an object module does not become a
semantic source of truth. Target Types, offsets, registers, address spaces,
calling convention records, and pointer representations remain derived facts
owned by that compilation. They are not copied back into the graph.

A semantic Terminal may support reconstruction without source. Its reader first
validates the complete bounded format. A graph owner then creates new stable
identities, reconnects edges as defined by their owners, and applies its own
validation, completion, and publication contract.

Reconstruction is equivalent when the fresh graph reproduces every public
observation promised by the format. These observations may include names,
categories, represented identity relations, semantic edges, order, Layout
behavior, completion, and concrete owner facts. Equivalence does not require
the same internal graph shape, process addresses, or References.

Structural coincidence is never enough for reconstruction. A reader cannot
infer Type identity from matching Layouts, recover owner relations from target
offsets, or treat a backend Type as the original semantic Type.

## Semantic invariants

1. Every emitted Token has one Code and one source span.
2. A Token stream is interpreted with the Lexer contract and source bytes that
   produced it.
3. Unrecognized authored bytes emit `Unknown` rather than disappearing.
4. Every semantic identity is an Abstract.
5. Semantic query failure returns Invalid rather than a null edge.
6. Later construction never changes an identity already returned successfully.
7. Alias preserves local identity while redirecting represented identity.
8. TTX Type and Addressable impose no Static or Self receiver-routing policy.
9. An atomic Type exposes itself as one terminal `Value` Layout entry.
   Every Type admitted to value flow has a nonempty Layout, Addressable reaches
   one such Type, and Callable supplies complete parameter and result Layouts.
10. Pack preserves produced value-flow identity and exposes one complete output
    Layout without acquiring Type identity.
11. Type, Pack, Addressable, Callable, and Layout remain independent contracts.
    TTX defines no universal member model over them.
12. Layout owns promised shape, order, and directional fitting, not produced
    value identity or copied semantic or physical
    records.
13. Every completed concrete Type admitted to ordinary value flow has one
    language-owned semantic default. TTX neither derives it from target bits
    nor makes it a shared identity or Type query.
14. Concrete language, package, target, runtime, and diagnostic policy remain
    outside TTX.
15. Reference preserves one exact borrowed object and never resolves or
    canonicalizes it implicitly.
16. A Reference is valid only within the lifetime guaranteed by its graph
    owner and never crosses a Terminal boundary.
17. A Terminal product is outside the semantic graph and belongs to no TTX
    identity category.
18. Target facts specific to a Terminal never flow backward into the graph as
    semantic authority.
19. Reconstruction without source creates a new live graph through its graph
    owner and never restores process addresses.
20. A reconstructed graph is published only after the graph owner's complete
    validation, completion, and publication contract succeeds.
