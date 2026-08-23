# TTX Design

TTX is the shared semantic boundary between concrete languages and the systems
that consume their work. It keeps exact identity, resolution, Type, Pack,
Addressable, Callable, and Layout facts available before a package format,
compiler target, runtime, or tool has been chosen.

The design is intentionally smaller than a complete language model. A concrete
language owns the meaning of its declarations, expressions, members, and
source rules. TTX owns the contracts that another domain can use without first
translating those facts into a second model.

That division gives a host one live multi domain semantic IR without making one
frontend or backend representation the language of every subsystem. The shared
part is the TTX contracts rather than a common node schema. This also places
responsibility on each concrete owner. TTX cannot provide generic declaration
visitors, C language behavior, target layout, optimization, or durable storage
on the owner's behalf.

Readers deciding whether that tradeoff fits their project should begin with the
[TTX overview](README.md). The exact contract is specified in
[TTX semantics](ttx_semantics.md).

## A small shared boundary

A representation works best when it has a clear job. TTX carries the facts
that packages, concrete languages, editors, compilers, and runtimes can exchange
directly. This includes produced Pack flow as well as the Layout contracts it
must satisfy. Facts meaningful to only one system stay with that owner.

The governing design test is simple: pull upward every fact that is target
neutral and genuinely shared while leaving richer meaning with its concrete
owner. A proposed contract belongs in TTX when independent domains need the same
question answered with the same meaning. A fact that depends on one grammar,
runtime policy, target representation, or consumer stays with that system.

This test also makes architectural drift visible. Copying an owner into a shared
record, introducing a parallel semantic graph, or moving target facts into TTX
changes the common layer from meaning into representation. The resulting model
would no longer give every consumer the original semantic fact.

The shared vocabulary contains relationships that are genuinely cross domain
rather than source features owned by one language.
A Library Object, a Package Dependency, a Scene signal, and a Shader resource
can all expose exact TTX identities while retaining their richer rules in their
own domains.

The benefit is that a consumer can ask the original semantic object for the
contract it needs. The cost is that there is no universal declaration record to
inspect when the concrete owner has not exposed that contract.

## One meaning, several consumers

An extensible toolchain works best when every useful fact has one clear owner.
A concrete language creates the semantic identity because it knows what that
identity means. TTX gives other domains a small set of stable questions they can
ask of the same object.

That arrangement lets one completed Workspace serve several kinds of consumer:

* Package follows names, dependencies, and reconstructable language facts
* Editors use source Anchors, Documentation, and semantic categories
* Compilers use Type, Pack, Layout, Addressable, and Callable contracts
* Runtimes receive finished products and the values defined by their language
* Language aware tools can still inspect the richer concrete object

The shared graph remains live while those domains need to cooperate. Later
stages can connect an identity, complete it, and answer richer queries, but the
identity itself stays stable. This gives every consumer the same answer without
making one consumer's representation the toolchain's universal model.

Physical representation begins only when a product needs it. A CPU compiler
chooses registers and calling conventions. A GPU compiler chooses storage
classes and bindings. An Archive writer chooses durable reconstruction facts.
TTX Layout continues to describe semantic shape and fitting across all three
uses.

Finished compiler formats remain valuable downstream tools within the platform.
Their role is to carry the completed facts needed by a target, while TTX keeps
the language meaning shared by the wider Workspace.

## Source, Tokens, and meaning

Authored bytes own spelling, order, and location. The Lexer turns those bytes
into an ordered Token stream. Each Token records one decoded source span and the
Code assigned by the exact Lexer contract.

A concrete language can construct retained semantic objects as it recognizes
the Token stream. TTX does not require a retained syntax tree between Tokens
and the semantic graph. Another host may own a source model when source
transformation or tooling requires one. Tetrodotoxin instead constructs its
Monographs directly and retains no second source graph.

Direct construction removes a translation layer and lets each language preserve
the distinctions its consumers actually use. The corresponding cost is that
TTX cannot provide generic AST visitors or source transformations across
unrelated languages.

TTX stores source locations as byte offsets because its source is UTF 8. Editors
sometimes count characters another way, so the editor host translates
positions from the retained text when it prepares a response. Keeping that
translation at the edge lets every editor use the same Anchors.

The ownership progression is:

```text
authored bytes
-> Tokens interpreted with their Lexer contract
-> semantic identities constructed by their owner
-> richer owner queries over those same identities
-> a Terminal product whose use is independent of the live semantic graph
```

Construction may connect identities and answer unresolved queries, but it
preserves every identity already returned successfully. This monotonic growth
keeps the live graph as the only semantic authority.

## One live graph

Every semantic identity begins with `Abstract`. Narrower categories add only
the queries needed to exchange that identity with another domain:

```text
Abstract
├── Invalid
├── Alias
├── Type
├── Addressable
├── Pack
└── Callable
```

`Documentation`, `Layout`, and `Reference` are supporting values. They describe
or connect identities without acquiring another semantic identity. Complete
declaration structure, source provenance, visibility, and publication remain
facts of each concrete language owner rather than a lossy Abstract projection.

The graph is not a tree of declarations. One Type may be reached through a
Package Alias, a source Alias, a Generic materialization, and a direct local
route. Those paths retain one Type and do not give it a required parent.

Structural similarity therefore says nothing about identity. Two Types may
have equal Layouts and still mean different things. Two Alias objects may have
different local names and Documentation while representing the same target.
One Addressable keeps its own identity while reaching a Type. A Pack keeps its
producer identity while exposing an identity free output Layout. A Callable
keeps its own identity while its parameters and results expose required
Layouts.

This gives tools and lowerers the original semantic fact. It also means they
must use the contracts exposed by its owner. TTX offers no synchronized member
record or cloned Type graph for a consumer that wants a different shape.

Category selection proves a contract on the original identity without creating
a wrapper, clone, registry entry, or second identity. A consumer that needs
source or declaration facts proves the concrete owner and inspects the complete
value retained there rather than asking every Abstract for a shared fragment.

## Reference and graph lifetime

A Reference is a nonnull borrowed edge to one exact semantic object. It remains
valid for the lifetime established by the graph owner and carries no absent
state.

A Reference preserves the exact object it receives. Alias hides its immediate
Reference and `resolve()` is the only operation that follows Alias edges.
Proving a Type or reaching the Type of an Addressable remains an explicit
operation performed by the consumer whose contract requires it.

The graph owner guarantees the lifetime of every borrowed identity. A process
address may serve as local identity while that owner keeps the object stable.
Its meaning ends with that graph lifetime.

This avoids copying a semantic object into every context that retains it and
avoids treating a pointer, spelling, or structural hash as a durable identity.
The tradeoff is that callers must respect one clear graph lifetime. A Reference
cannot serve as a persistent handle after its owner is gone.

When a Terminal supports later reconstruction, its format records the owner
facts needed to build a new graph. Reconstruction creates new live objects
under a new owner. It does not revive old References or promise equal process
addresses.

## Owner directed contextual resolution

`resolve()` follows represented identity. `resolve_context(name)` asks the
receiving Abstract to interpret one borrowed name in its own domain. The
concrete operator splits qualified syntax and asks each selected result about
the next name, so a route can cross Package, Monograph, source, and Type
contexts without flattening those contexts into one key or converting them
into one common category. Alias remains opaque to contextual lookup: the
caller resolves it before asking the selected identity to interpret the next
name.

Context lookup does not stand in for explicit receiver access. Address and call
operators issue `resolve_access(host, name)` and `resolve_call(host, name)` with
the original caller authority. These hooks are shared because a concrete query
may cross an arbitrary Abstract context. Their interpretation is not shared:
TTX Type and Addressable impose no receiver role, forwarding behavior,
visibility policy, or member inventory. The concrete language refines those
contracts when it needs such behavior. Type qualification remains ordinary
one name contextual resolution rather than a second host neutral Type lookup.
Each query returns the original selected identity or Invalid without searching
a different category domain.

The caller owns the expected contract and proves the returned identity against
the semantic category it needs. Route spelling does not infer a Type,
Addressable, Callable, or another category on the caller's behalf.

This lets one qualified spelling cross several concrete contexts without
requiring a universal member table, overload set, scope object, or collision
domain. The receiving owner interprets the one name it recognizes, and the
concrete language assigns meaning to the operator that initiated the query.

The cost is deliberate locality. Contextual resolution is not a global search
service, so a caller cannot rely on unrelated domains or a registry to repair
an ambiguous ownership path.

## Progressive construction

A graph owner may reserve a stable identity before all of its edges are ready.
An Alias reserved this way binds its borrowed target once after the defining
pass. The immediate edge remains opaque and only Alias resolution traverses
it. An incomplete total query returns the shared `Invalid` object. Completion
may make that unanswered query valid, while every successful identity remains
stable.

A staged Pack follows the same rule. Its Layout query is total and may expose an
empty shape while the Pack resolves to Invalid. Only a Pack that resolves to
itself supplies that empty Layout as valid zero value flow. Once resolution
succeeds, the Layout is stable.

This supports recursive declarations and source groups without adding an
Incomplete Layout or a universal publication bit to every Abstract. The
concrete owner decides how it stages construction and when completed roots can
be published.

Negative answers require care while construction is active. A consumer cannot
treat `Invalid` as permanent across a transition that may complete more facts.
An immutable compiler, writer, or Terminal emitter begins after its owning
completion barrier. A tool that intentionally observes partial state must treat
the graph as changing.

Parser rejection, failed Layout fitting, archive corruption, and backend
failure remain results of their owning operations. They do not create substitute
semantic identities.

## Pack is value flow and Layout is semantic shape

Pack and Layout deliberately answer different questions. A Pack identifies one
producer and the values it supplies. A Layout has no semantic identity and
describes a promised shape. This lets a call, swizzle, return, or other
multiple value operation remain live value flow without materializing an anonymous
aggregate Type merely so another owner can fit it.

A Pack may be empty, contain one ordinary value producing expression, or carry
several positional, named, ranged, or composed values. One expression is
already a one value Pack. Grouping it does not create another semantic object.
A multiple value Pack remains untyped as a group until a receiving declaration or
operation deliberately materializes one Type. Its individual produced values
retain their exact semantic identities throughout fitting. An explicit empty
grouping exposes an empty Layout, so cross language empty flow needs no Type
identity.

The common source convention reinforces the distinction: parentheses group
supplied Pack values and brackets describe required Layout entries. A named
descriptor uses `.name : Type`. A named value uses `.name = expression`. A
concrete language may omit a delimiter where its grammar remains unambiguous,
but the semantic direction does not change.

A Layout retains an ordered view of exact Abstract identities and answers
whether one shape fits another. Fitting is directional because a Pack's source
Layout supplies the values required by a target Layout.

`Value` is the terminal one entry descriptor for one exact atomic Type. `Fluid`
describes positional entries and compares them in order. `Named` describes
uniquely named slots, matches them by name, then preserves the source Layout's
fitting rule for each match. A slot may borrow a name independently while
retaining the exact source Abstract. `Ranged` describes one entry across a fixed
interval. `Composite` preserves two complete child Layouts.

Successful fitting returns the original source edge that supplies a target
position. The Pack remains the value flow owner. Its Layout retains order,
borrowed slot names, and applicability without copying Types, Documentation,
defaults, or storage facts into a generic member record. A decorator or
composition delegates entry fitting to the source Layout that owns each edge.

Keeping Layout semantic lets the same graph feed a CPU compiler, GPU compiler,
interpreter, editor, and archive writer without letting the first backend fix
the physical meaning for every later consumer.

TTX therefore cannot answer target object layout, field offset, register,
address space, pointer form, or calling convention questions by itself. Each
compiler derives and validates those facts for its own Terminal. That extra work
is the price of keeping the graph target neutral.

An empty Layout is a valid zero value shape and fits another empty Layout. An
empty Pack exposes that shape without requiring a Type identity.
Atomic Types cannot launder that shape because their Value Layout contains
their own exact identity. A Type with an empty Layout may still own contextual
facts, but it cannot enter value flow and no Addressable can name it.

## Semantic defaults

Every concrete Type admitted to ordinary value flow has one default selected
by its owning language. This is a total language invariant, not a shared TTX
value representation. It lets safe selection, omitted initialization, and
other concrete operations ask their language owner for a value without making
nullable references or target zero bits part of the semantic model.

The distinction matters for composite and managed values. A target may obtain
cleared storage, but a language default may still require recursive Field
initialization or allocation of a fresh nonnull identity. Conversely, the
default of an optional value may be empty without constructing its payload.
Completion rejects a recursive value Layout whose real Type and Addressable
edges cannot reach terminal leaves. Default construction therefore needs no
second recursion transaction.

Default value and empty Layout are also independent. A View with no elements
is still one exact View value, so the Pack carrying it has a one entry Layout.
An empty Layout instead describes zero value flow. TTX preserves this
distinction while leaving every concrete default constructor with its language
owner.

## Terminal products and reconstruction

A Terminal is a completed output that leaves the live semantic graph. Its use
is independent of that graph and its process identities. Examples include
formatted text, editor data, LLVM IR, SPIR-V words, debug data, object modules,
native binaries, and semantic archives.

Workspace completion is the explicit handoff from semantic raising to Terminal
production. A target producer begins lowering and owns every representation
decision for its destination. Formatters and editor producers project selected
facts, Package serializes an Archive, and Linker composes native products.
Dialects do not receive producer callbacks, and product facts do not flow back
into them.

This creates two complementary ways to compose the complete toolchain. Dialects
choose the meanings that can participate in a Workspace. Terminal producers
choose the destinations available after that meaning completes. Their symmetry
is compositional rather than structural: a Dialect owns live semantic objects,
while a producer consumes them under the contract of one concrete product.

Terminal is a role at a boundary, not an Abstract category or a universal
product hierarchy. Each concrete producer retains ownership of its format. A
linker object and a semantic archive are both Terminal products, but their
different purposes do not create a generic product model.

No live Abstract identity or Reference crosses this boundary. A Terminal may
encode facts chosen by its owner, but its bytes or text are not semantic
objects. A later process that needs semantic meaning validates the format
defined by its producer and asks a concrete graph owner to construct a new
graph.

Most Terminals are intentionally lossy. LLVM IR, debug data, and object files
preserve the target facts needed by their consumers, not the complete language
graph that produced them. Feeding those products back as TTX Type, Layout, or
identity authority would ask a lowered representation to recover meaning that
it no longer carries.

Terminal describes departure from the Workspace rather than the end of all
lowering. An LLVM or MLIR module can leave Tetrodotoxin as a Terminal product
and continue through the ordinary lowering pipeline owned by that ecosystem.

A semantic archive has a different purpose. It lets a graph owner avoid source
acquisition, lexing, and parsing when it retains the facts and relations
required by every included language. The result is a fresh graph with
equivalent observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and concrete owner facts,
followed by the graph owner's validation, completion, and publication contract.
Its supporting shape and process addresses may differ because they are not
public semantic observations.

Equivalence is relative to the Terminal's declared query contract rather than
the complete source graph. A compiled library can reconstruct the Types,
Addressables, Callables, and folded constants that another graph may query while
its object module carries executable behavior. This is the semantic equivalent
of a Foreign wrapper: the reconstructed graph describes how to interact with
the compiled implementation without preserving the operations that produced
it. A different Dialect may retain a different closed observation set.

The reconstruction payload may therefore be much smaller than a memory image.
It records sufficient owner facts rather than private graph structure. This is
a format benefit rather than a requirement that every language support
persistence.

The distinction also shapes verification. Graph queries prove resolution,
identity, fitting, order, and completion. An independent consumer proves a
Terminal format. A fresh graph reconstructed from an archive proves durable
semantics. A textual graph, AST, or IR dump is representation evidence only,
except where exact text or bytes are themselves part of a Terminal contract.

## Choosing this boundary

TTX is useful when several concrete languages or semantic domains must compose
before lowering, when exact identity matters across language and tool
boundaries, or when one completed graph must feed several independent
products. It is especially valuable when forcing every participant through one
declaration tree would erase distinctions that their defining languages still
need.

That architecture carries costs. The host must own graph lifetime and
completion barriers. Each language must define its semantics. A persistent
language must also define and validate its reconstruction facts. Languages
used only from source need no reconstruction contract. Each target must derive
physical representation. Generic AST traversal, operation rewriting, and
backend services come from other layers rather than TTX.

A smaller source model may be enough when one language owns the whole program
and one consumer owns every output. TTX earns its place when independent owners
still need to share meaning. Concrete languages decide which Types exist, how
names are published, which writes are legal, how Callables are selected, and
how expressions produce Packs. Targets, runtimes, Packages, and tools consume
those facts without becoming new TTX categories.
