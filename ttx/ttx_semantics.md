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

The TTX Tokenizer appends one zero length Terminal at the end of the source. Its
offset equals the source byte count. Another frontend may produce its own Code
stream, including facts established by preprocessing, before constructing the
same Cursor contract. A Cursor can observe a signed relative position without
moving. Observation outside the stream returns an empty Terminal.

A Cursor is the one mutable position over the immutable Token stream. It owns no
tokenization or macro policy. Grammar dispatch proves the selected production
before its parser consumes that Cursor. A rejected production retains its
diagnostics and the source transaction owns discarding any candidate semantic
state.

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

Tokens, Spans, and Anchors measure source in UTF 8 bytes. An editor protocol may
count the same text differently, so its host translates those offsets while it
has the source available. This keeps the authored lexical facts useful to every
host without making editor coordinates part of TTX.

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
  one total Domain answer, and concept queries.
* `Unknown` is a provisional answer that may settle differently as the graph
  completes.
* `Constant` proves one complete immutable axiomatic graph fact.
* `None` is the shared Constant proving completed absence.
* `Route` is a Constant that lends one complete immutable concept byte sequence.
* `Alias` transparently forwards every observation to one required referent.
* `Domain` relates one semantic identity to its current value shape.
* `Addressable` layers local answers over one required referent while retaining
  the visible candidate identity.
* `Callable` provides complete parameter and result Layouts.

`Documentation`, `Layout`, `Pack`, `Context`, and `Interface` are
supporting contracts and values that carry no semantic identity. Pack carries
named semantic flow over the exact Abstracts identified by its Layout.

These categories are an interchange vocabulary rather than a complete type
system. Concrete languages define their type inventory, access policy,
mutation, construction, and invocation roles.

A Terminal product is not an identity category. `Lexical::Code::Terminal` is
the end marker for a Token stream. Neither term introduces another semantic
identity.

## Resolution and category proof

Every Abstract exposes its local name, Documentation, represented identity
through `resolve()`, Domain relationship through `resolve_domain()`, and
owner-directed concept lookup through `resolve_concept(name)`. Each observation
is total.

`resolve()` returns the represented identity. `resolve_concept(name)` gives one
borrowed binary name to the receiving identity, which interprets the question
according to its own domain. Concrete operators split qualified syntax and ask
the identity selected by each preceding name. They never flatten a qualified
route into one lookup key or encode access and invocation as shared route modes.

`visit_concepts(callable)` synchronously invokes one caller-supplied Callable
for each name and exact Abstract that the receiver currently advertises. The
visit is factual, unordered, and unchained. It may omit an incomplete question
or visit an explicit Unknown answer. The receiver retains no visitor and
creates no Pack, Layout, or enumeration snapshot.

An Abstract returns Unknown for a concept it does not recognize. Returning None
means that the concrete owner recognized that exact question and proved
completed absence. Later graph observation may return another answer after a
live source authority changes.

Category proof negotiates a real category requirement Abstract against the
candidate through Interface. A successful proof returns an identity-free view
of the same candidate and that category's operations. It never creates a
wrapper identity, clone, registry entry, native type identifier, or substitute
semantic object.

Source provenance, declaration structure, visibility, and publication remain
facts of the concrete language owner. Abstract exposes no generic declaration
projection. A consumer that needs those facts proves the concrete owner and
inspects its complete declaration value.

## Unknown, Constant, and None

Unknown is the shared provisional Abstract. Its name is `Unknown`, its
Documentation is empty, and its Domain and concept answers remain Unknown. It
stores no failed route, source range, diagnostic, or recovery choice. A query
that answers Unknown may later answer a real identity or None.

Constant proves one complete immutable terminal graph fact. The fact itself
never changes. Constant says nothing about whether the route that selected it
will select the same identity during a later observation. A consumer re-queries
a live route and may reuse a derived result only after proving that its current
Constant inputs are the same exact identities.

None is the shared Constant for proven absence. Its name is `None`, its Domain
answer is None, and every concept query returns itself. None never means that a
future answer may materialize; Unknown carries that meaning.

## Route

Route is the narrow Constant contract for one complete concept byte sequence.
After positive negotiation its typed support view preserves the exact candidate
and lends those immutable bytes. The sequence may be empty or non textual and
always remains one atomic question regardless of how a receiver interprets it.

Route exists so Named Layout metadata and other host neutral routing machinery
do not depend on one language string type or copy the same bytes into a second
private table. It introduces no universal number, Boolean, string, collection,
or value hierarchy. A Route has no Domain unless its concrete owner separately
provides one.

## Finite extent

Extent is the narrow Constant contract Ranged uses to obtain one exact
nonnegative support cardinality. Positive Interface negotiation preserves the
candidate, and its typed observation lends the `uint64_t` count. This operation
does not make host integers, arithmetic, scalar storage, or one language's
numeric hierarchy part of TTX.

An extent answer is Unknown while the relationship remains indeterminate and
None after completed rejection. Only an exact Extent witness lets Ranged
satisfy Enumerable. An exact zero witness therefore proves completed empty
flow, while Unknown cannot be visited as though it were zero.

## Alias

Alias is transparent indirection to one required referent. It forwards its
name, Documentation, resolution, Domain, concepts, visitation, and Interface
negotiation to that referent without adding an observable contract of its own.
An unbound Alias forwards to Unknown, which records that the required referent
has not settled rather than making it nullable.

A concrete graph owner may reserve an Alias before its target is known. It can
then commit Unknown to one exact non Alias Abstract, after which a different
target fails. Repeating the same binding is harmless. Construction spot
resolves an Alias candidate before retaining it, so Alias chains cannot form.
None and self reference are rejected because neither can satisfy the promised
referent, and self reference is checked before resolution so Unknown cannot
conceal a cycle attempt.

Because Alias forwards every question, contract negotiation cannot reveal or
decorate the indirection. Policy belongs in an Addressable or another layered
owner that remains visible as the candidate it presents.

## Domain

Domain is the total graph relationship that answers which semantic value domain
currently describes an Abstract. `resolve_domain()` returns Unknown while that
relationship is indeterminate, None when the owner proves that no Domain
relationship exists, or one exact Domain identity beside its immutable Layout
for that observation.

A Domain answers its own Domain relationship with itself. That reflexive edge
does not create a type system, parent relation, or subtyping hierarchy. Domains
may participate in cycles and recursive value descriptions because every query
retains the factual uncertainty rules established above. Concrete languages
build their own type systems from Domain identities, concepts, Interfaces, and
Generics without making those policies host neutral.

A Domain admitted to ordinary value flow has a nonempty Layout. The identity
free `Value` Layout records one exact atomic producer relationship, while
structural Domains can project Ranged, Fluid, Named, Composite, or Reindexed
shape. A Domain with an empty Layout may retain contextual facts but cannot
itself enter value flow. An empty Pack remains complete zero value flow rather
than an instance of such a Domain.

Construction, initialization, fitting, folding, visibility, storage, scalar
families, bit width, alignment, and target representation are independent
concepts owned by the concrete systems that need them. Domain supplies none of
those policies merely because several operations ask about the same value
relationship.

## Addressable

Addressable is the first non axiomatic layered concept. Here “addressable” means
a semantic subject to which another system can direct a question, closer to
addressing a person than taking a machine address. Physical storage and pointer
representation remain language or Terminal facts.

An Addressable composes one required referent with an ordered set of local
layers. For each question the outer layer answers first. None means that layer
does not own the route and permits the next layer to continue. Unknown or an
exact answer stops the observation. Unknown is therefore absorbing: forwarding
past it could expose a referent answer that a later policy fact would have
hidden. A later observation asks the layer again rather than retaining that
decision.

The same left biased rule lets one layer enrich the referent with new answers or
attenuate it for a restricted holder. Visibility may return Unknown for private
routes while a writability layer supplies an exact write authority, for example.
Reversing those layers can expose different authority, so composition is
associative but not commutative.

Unlike Alias, Addressable remains the visible candidate. Its `resolve()` answer
is itself, and a successful Interface witness retains that Addressable identity
instead of lending the unrestricted referent. Layers may project a nonempty
Domain relationship, but assignment, storage duration, visibility, and
writability remain concrete policies rather than mandatory Addressable fields.

## Pack

Pack is an identity-free support value that carries one produced semantic flow.
Its Layout identifies the actual producer Abstracts directly. Pack is not an
Abstract, Domain, Addressable, or Layout, and it has no parallel Produced record.
A consumer can retain, inspect, fit, or lower the flow without materializing an
aggregate type or reconstructing a producer table.

A Pack may supply zero, one, or several values. It may expose positional, named,
ranged, or composed output shape. One ordinary value producing expression is
already a one value Pack. Grouping that expression does not create a second
semantic identity. An empty Pack exposes an empty Layout. A concrete language's
empty result and an explicit empty grouping agree through that Layout without
requiring a Domain identity. A multiple value Pack remains value flow until a
receiving contract fits it and an owning language deliberately materializes a
concrete type.

The common delimiter shapes keep value flow and required shape visually
distinct: parentheses group produced Packs while brackets describe Layouts.
Concrete languages decide which productions may omit those delimiters, but
omission does not change the resulting Pack or Layout contract. Named Pack
slots use `.name = expression` and retain those names independently from the
produced semantic objects. Named descriptor slots use `.name : Type`. The
different operator keeps promised shape distinct from supplied value flow.

A Pack is a snapshot value rather than a staged identity. Its owner publishes a
fresh Pack only from facts currently available. Ordered value flow remains
ordered because its owner explicitly supplies an ordered Layout; unordered
concept discovery does not acquire order merely because it is represented by a
Layout value.

## Callable

Callable is a closed TTX contract negotiated against one exact candidate. A
positive result returns an identity-free view retaining that candidate beside
its current parameter and result Layouts. Either Layout may remain partial while
the graph is useful to tools. Completed empty Layouts prove zero value input or
output. TTX assigns no receiver role to any parameter position or spelling.

The common Callable view contains no invocation operation. A concrete language
may fit an argument Pack to the parameter Layout and expose invocation through a
separate operational Interface while adding executable body, calling
convention, machine address, or target ABI policy. Equal Callable Layouts do not
prove behavioral equivalence.

## Layout

Layout carries no semantic identity. It describes one promised value shape and
provides synchronous support views plus directional
`fit(source Pack, caller Context)` over exact Abstract identities. Domains and
Callables expose Layouts. Packs carry the Layout of the values they supply.
Fitting returns Unknown while the relationship is unsettled, None for completed
rejection, or a Context retained witness Pack containing the admitted source
producers.

TTX defines these common Layout forms:

* `Empty` proves exact cardinality zero.
* `Value` retains one exact producer occurrence.
* `Ranged` retains one repeated producer relationship and one extent Abstract.
* `Fluid` retains one finite sequence of independently factual occurrences.
* `Named` combines one source Layout with a shape matching route Layout whose
  entries may be Unknown, None, or exact Route Constants.
* `Composite` combines two Layouts while preserving their coupled fitting
  boundaries.
* `Reindexed` retains one source Layout and one completed output shaped mapping
  whose leaves select exact source paths.

Fitting is directional: the source Pack supplies its produced-flow Layout, while
the receiving Layout owns the structure and relationship it asks that flow to
satisfy. A Layout that decorates or combines another Layout owns that
composition and delegates to its real children without exposing indices or
fitted maps to generic callers.
Enumerable proves an exact finite cardinality and visits that many structural
paths with their borrowed Abstract occurrences. Named provides route visitation
and selection without defining a universal lookup or member interface. Fluid
is a separate typed support observation, so a positional receiver cannot flatten
an arbitrary Enumerable or Composite Layout merely because their leaf counts
match.

Layout retains no copied semantic record, target offset, storage class, ABI
rule, or anonymous Domain identity. Structural coincidence does not create
Domain identity.

Every Layout can transfer one independently owned support snapshot. The
snapshot preserves the complete observable Layout, including fitting behavior,
support views, child boundaries, paths, partial route answers, and borrowed
Abstract occurrences. It carries no semantic identity. Its owner supplies the
release operation, while the receiving Context owns when that operation is
called.

Snapshotting is not leaf enumeration. Reconstructing a generic list from an
Enumerable view would discard stronger Named, Composite, or Reindexed facts.
The concrete Layout owner performs the copy so Context never selects a Layout
family or acquires the machinery that originally established the projection.

An empty Layout visits no entries. It fits another empty Layout and describes no
stable value or address, regardless of which concrete Domain or Pack exposes it.

## Interface

Pack and Layout describe value flow. A Pack borrows its exact producers, while
its Layout projects that flow into the shape a consumer may receive. This
projection deliberately omits behavior and richer domain meaning.

Interface negotiates the semantic relation that remains after that projection.
It receives two real Abstracts, treats the first as the requirement and the
second as the candidate, and returns `Unknown`, `Rejected`, `Satisfied`, or
`Equivalent`. Unknown keeps that ordered relationship unsettled. Rejected
completes it negatively. Satisfied proves the requested direction. Equivalent
records the stronger relation established by that exact requirement.

An Interface may use Layout fitting and shared category proof as evidence, but
matching Layouts alone never imply semantic equivalence. A Callable Interface
can compare parameter and result flow while a richer owner also checks behavior
or policy. Another Interface may negotiate resources, lifecycle roles, or a
domain that carries no value flow.

Interface carries no semantic identity and retains no copied inventory of the
Abstracts it compares. It creates no Alias, wrapper, common Domain, or dependency
between their Dialects. The concrete owner selects the negotiator appropriate
to its semantic question.

Interface negotiation creates no runtime representation. A concrete language
may define an explicit erased value that retains an accepted candidate, and a
Terminal may derive the Projection required by its target ABI. Without that value,
the candidate remains concrete and incurs no runtime dispatch merely because an
Interface accepted it.

An explicit erased value retains the candidate and exact positive Interface
witness owned by its concrete language. It never recovers category proof by
casting a native object or comparing operation table addresses.

## Documentation

Documentation carries no semantic identity. It presents an ordered view of
presentation lines. Every Abstract returns one stable Documentation reference.
Missing documentation is the shared empty Documentation value.

Documentation may present one generated line, an ordered authored block, or a
composition of two complete Documentation values. It does not participate in
identity, resolution, or Layout fitting.

An authored `//` line contributes one presentation line. When the comment
payload begins with `/`, the resulting `///` form is a raw comment instead.
Raw comments remain in the lexical source and editor token stream, but they do
not contribute Documentation. Formatting preserves their authored content.

## Host ABI

The TTX concept and model contracts are expressible as C handles and immutable
operation tables. One authority token plus one value token is the live semantic
identity. The operations pointer is borrowed dispatch machinery and may change
across a bridge without changing that identity. Category views, Layouts, Packs,
Contexts, Interfaces, and stateful Callable handles carry no second semantic
identity.

The ABI uses no C++ template proof, inheritance, RTTI, native type address,
`void*` callback context, or container contract. A concrete owner retains any
state behind its typed handle. Every borrowed identity remains valid only for
the operation and graph lifetime established by that owner.

Every public TTX header is a self contained C17 header. Abstract, Layout, Pack,
Context, and Callable handles begin with their immutable operation table.
Interface and category views instead borrow the exact candidate Abstract beside
the operations that prove the requested relation. A concrete owner composes the
operation tables for the categories it proves. It does not acquire category
meaning from a C++ base class or a registry.

Stateful visitation uses a typed Callable handle. The Callable operation
receives that same handle as its self value, so its concrete owner can recover
retained state without an erased context pointer. Concept and Layout visits are
synchronous and the receiver retains no Callable after returning.

Context is a caller owned result domain. Its sole authority is `pack(layout)`.
It asks the supplied Layout to transfer an owned support snapshot, verifies that
the snapshot remains Enumerable, and retains it behind the resulting Pack for
the Context lifetime. The snapshot continues to borrow the real Abstract
identities. The input Layout needs to remain valid only until `pack(layout)`
returns, while the graph owner must outlive every Context borrowing its
Abstracts. Concept visitation does not allocate a Context or a Pack.

`pack(layout)` publishes atomically. A snapshot, allocation, capacity, or
transport failure publishes no Pack and is a support failure rather than
semantic Unknown or None. Context never infers that an Enumerable relationship
Layout represents produced flow. The concrete operation calling `pack(layout)`
owns that precondition.

## Consumers and Terminal products

Concrete languages may define expressions, constants, generic formulas,
mutation capabilities, receiver roles, executable bodies, and concrete scalar
types. Their value producing expressions participate as Packs and retain exact
TTX Domain, Layout, Addressable, and Callable edges.

Targets may derive sizes, offsets, pointer forms, address spaces, registers,
ABI carriers, and executable addresses. Runtimes may add managed storage,
frames, collectors, and scheduling state. These are consumers of the semantic
graph rather than additional TTX categories.

The completed Workspace is the handoff from raising to Terminal production. A
target producer begins lowering, walks the concrete graphs it supports, and owns
every representation fact it derives. Other producers may project, serialize,
or compose completed facts for their own consumers. No Dialect calls into a
producer, and no product fact becomes a semantic edge.

Dialects and Terminal producers are complementary composition points for the
complete toolchain. Installed Dialects determine which meanings a Workspace can
construct. Selected Terminal producers determine which independent products can
be derived after completion. This parallel role does not give them one shared
interface or lifetime. Dialects participate in the semantic graph, while
Terminal producers consume it from outside.

A consumer crosses the Terminal boundary when it emits an output whose
consumption is independent of the live semantic graph and its identities.
Formatted text, editor data, LLVM IR, SPIR-V modules, debug data, object
modules, executables, and semantic archives are examples. Terminal is a
boundary role. TTX defines no universal Terminal category, product registry,
or common byte container.

Each Terminal format belongs to its concrete producer. The format may retain
source presentation, target representation, or reconstruction facts defined by
semantic owners according to that producer's purpose. The Terminal itself is
never an Abstract, Domain, Pack, Layout, Addressable, or Callable.

A target Terminal such as LLVM IR or an object module does not become a
semantic source of truth. Target types, offsets, registers, address spaces,
calling convention records, and pointer representations remain derived facts
owned by that compilation. They are not copied back into the graph.

Terminal is relative to the live Workspace. An LLVM module or emitted MLIR
module may be Terminal for Tetrodotoxin while remaining an intermediate input
to another tool's progressive lowering pipeline.

A semantic Terminal may support reconstruction without source. Its reader first
validates the complete bounded format. A graph owner then creates new stable
identities, reconnects edges as defined by their owners, and applies its own
validation, completion, and publication contract.

Reconstruction is equivalent when the fresh graph reproduces every public
observation promised by the format. These observations may include names,
categories, represented identity relations, semantic edges, order, Layout
behavior, completion, and concrete owner facts. Equivalence does not require
the same internal graph shape, process addresses, or old handle values.

The Terminal producer defines that observation set. It may retain only the
Domains, Addressables, Callables, constant Pack flow, and owner facts that later
consumers can query through its contract. Source declarations, executable
bodies, and intermediate expressions are not implied observations. A compiled
Package may therefore pair a semantic Terminal that reconstructs its query
surface with a native Terminal that supplies execution. Recompilation remains
a source or live Workspace operation.

Structural coincidence is never enough for reconstruction. A reader cannot
infer Domain identity from matching Layouts, recover owner relations from target
offsets, or treat a Terminal type as the original semantic Domain.

## Semantic invariants

1. Every emitted Token has one Code and one source span.
2. A Token stream is interpreted with the Lexer contract and source bytes that
   produced it.
3. Unrecognized authored bytes emit `Unknown` rather than disappearing.
4. Every semantic identity is an Abstract.
5. Incomplete semantic queries return Unknown, while completed absence returns
   None; neither answer is a null edge.
6. Later construction never changes an identity already returned successfully.
7. Alias transparently forwards every observation to one required spot resolved
   referent and exposes no separate contract.
8. Domain and Addressable impose no Static or Self receiver routing policy.
9. An atomic Domain exposes its exact producer relationship through one `Value`
   Layout entry. Every Domain admitted to value flow has a nonempty Layout, and
   Callable supplies parameter and result Layouts.
10. Pack is an identity-free support value whose Layout names the actual
    producers directly.
11. Domain, Addressable, Callable, Pack, Layout, and Interface remain independent
    contracts. TTX defines no universal member model over them.
12. Layout owns promised shape and directional fitting, not produced value
    identity or copied semantic or physical records. Visitation order is
    semantic only when that concrete Layout owner explicitly promises order.
13. Interface negotiates a semantic relation over real Abstracts without
    creating identity, copying either graph, or making equal Layouts imply
    equal meaning.
14. Construction, initialization, fitting, folding, visibility, storage, and
    target representation remain independently owned concepts rather than
    Domain methods.
15. Concrete language, package, target, runtime, and diagnostic policy remain
    outside TTX.
16. A borrowed handle preserves one exact object and never resolves or
    canonicalizes it implicitly.
17. A borrowed handle is valid only within the lifetime guaranteed by its graph
    owner and never crosses a Terminal boundary.
18. A Terminal product is outside the semantic graph and belongs to no TTX
    identity category.
19. A Layout owner transfers an independently owned support snapshot to Context.
    Context never reconstructs a stronger Layout from Enumerable leaves.
19. Target facts specific to a Terminal never flow backward into the graph as
    semantic authority.
20. Reconstruction without source creates a new live graph through its graph
    owner and never restores process addresses.
21. A reconstructed graph is published only after the graph owner's complete
    validation, completion, and publication contract succeeds.
