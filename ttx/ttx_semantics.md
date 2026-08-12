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

A Cursor branch creates a speculative position over the same immutable Token
stream. Joining publishes that position. Rejecting the branch leaves the parent
Cursor unchanged.

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
* `Value` provides a leaf Type with no contextual subdomains.
* `Flag`, `Real`, `Signed`, and `Unsigned` refine Value domains.
* `Addressable` provides a named address whose edge reaches one Type.
* `Callable` provides complete parameter and result Layouts.

`Authorship`, `Documentation`, `Layout`, and `Reference` are supporting
contracts and values that carry no semantic identity.

These categories are an interchange vocabulary rather than a complete type
system. Concrete languages define their Type inventory, access policy,
mutation, construction, and invocation roles.

A Terminal product is not an identity category. `Lexical::Code::Terminal` is
the end marker for a Token stream. `Value` is the leaf semantic Type
category. Neither one represents an emitted Terminal product.

## Resolution and category proof

Every Abstract exposes its local name, its Documentation, its represented
identity through `resolve()`, and owner directed contextual identity through
`resolve_context(route)`. Each observation is total.

`resolve()` returns the represented identity. `resolve_context(route)` gives a
borrowed route to the receiving identity, which interprets it according to its
own domain. For an unchanged completed graph, resolution is idempotent and
every chain terminates.

An unanswered semantic query returns `Invalid`, never a null graph edge. Later
construction may answer a query that formerly returned Invalid, but it does not
replace an identity that was already returned successfully.

Category proof establishes the semantic contract of the original object. A proof
never creates a wrapper, clone, or substitute identity. The consumer states the
category it needs and either receives that same object under the proven
contract or retains the original Abstract. Category proof follows one public
C++ inheritance chain.

An Abstract may separately borrow one identity-free Authorship source fact.
Authorship preserves the exact authored Documentation and Anchor while its
concrete language decides whether the authored object is published. It does
not expose or invent a shared visibility model. Absence means that the semantic
identity has no authored source fact; it does not manufacture a second
category, wrapper, or declaration identity.

## Invalid

Invalid is a stateless and absorbing Abstract. Its name is `Invalid`. Both
resolution operations return the same Invalid identity. Its Documentation is
empty.

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
`resolve_context(route)`, returns Invalid. A consumer that needs context first
resolves the Alias, proves the returned owner, and invokes that owner's
operation explicitly.

A concrete graph owner may reserve an Alias identity before its target is
known. An unbound Alias resolves to Invalid, and its target may be bound only
once; repeating the same binding is harmless while changing it fails. The
owner preserves target lifetime and prevents Alias cycles before publishing
the completed graph.

## Type and Value

Type is an Abstract that represents a semantic domain. Once resolution
succeeds, each Type exposes one complete Layout. An empty Layout is a valid
shape and does not alone establish scalar identity.

Value is a leaf Type used for scalar bit interpretations. Its contextual
resolution always returns Invalid. Value provides bit width and abstract
machine storage size and alignment. These are language level scalar facts, not
target object layout, ABI alignment, or register policy. Concrete domains and
representations belong to the language that constructs them.

TTX defines the `Flag`, `Real`, `Signed`, and `Unsigned` Value interfaces.

## Addressable

Addressable is an Abstract that names typed data. It reaches
one exact Type.

A concrete graph object may be Addressable while adding capabilities
owned by its language. TTX defines only the named edge to one Type.

Contextual resolution, Layout selection, or another consumer operation may
return an Addressable. TTX does not prescribe its physical address or target
representation, and later realization does not change the selected identity.

Assignment, writability, storage duration, and visibility are concrete language
policy rather than part of the shared Addressable contract.

## Callable

Callable is an Abstract that supplies one complete signature as a parameter
Layout and a result Layout. A Callable is type-bound when parameter entry zero
is an Addressable named `self`; that Addressable supplies the exact receiver
Type. This role is derived from the Layout and creates no second Callable
category or retained marker.

TTX does not prescribe how a Callable is selected or invoked. A concrete
language may fit its parameter and result Layouts while adding executable body,
calling convention, machine address, or target ABI policy.

## Layout

Layout carries no semantic identity. It provides ordered observation and
directional fitting over a group of exact Abstract identities. A consumer may
test a complete fit, test a fit at an offset, and recover the original source
edge that supplies a target position.

The closed fitting errors are `IndexOutOfBounds`, `SizeMismatch`, and
`IncompatibleFit`. A failed fit does not add Invalid to the semantic graph.

TTX defines four common Layout forms:

* `Fluid` fits ordered entries by represented identity. An Addressable target
  participates through its Type.
* `Named` fits nonempty unique names by name and represented identity.
* `Ranged` repeats one exact entry over a fixed interval and applies Fluid
  fitting.
* `Composite` combines two complete Layouts without flattening them.

Fitting is directional: the source supplies the target. Complete fitting
requires equal sizes. Segmented fitting places a source in one target interval.
Successful fitted queries return the original source edge that supplies the
target position.

A consumer may select one exact Layout entry and prove the category required by
its own operation. `Named` provides name based fitting without defining a
universal lookup or member interface.

Layout retains no copied semantic record, target offset, storage class, ABI
rule, or anonymous Type identity. Structural coincidence does not create Type
identity.

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
Types. They retain exact TTX Type, Layout, Addressable, and Callable edges.

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
never an Abstract, Type, Layout, Addressable, Callable, or Reference.

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
8. Value has no contextual subdomains.
9. Addressable reaches one Type and Callable supplies complete parameter and
   result Layouts.
10. Type, Addressable, Callable, and Layout remain independent contracts. TTX
    defines no universal member model over them.
11. Layout owns order and directional fitting, not copied semantic or physical
    records.
12. Concrete language, package, target, runtime, and diagnostic policy remain
    outside TTX.
13. Reference preserves one exact borrowed object and never resolves or
    canonicalizes it implicitly.
14. A Reference is valid only within the lifetime guaranteed by its graph
    owner and never crosses a Terminal boundary.
15. A Terminal product is outside the semantic graph and belongs to no TTX
    identity category.
16. Target facts specific to a Terminal never flow backward into the graph as
    semantic authority.
17. Reconstruction without source creates a new live graph through its graph
    owner and never restores process addresses.
18. A reconstructed graph is published only after the graph owner's complete
    validation, completion, and publication contract succeeds.
