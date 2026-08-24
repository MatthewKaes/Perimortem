# Tetrodotoxin Design

Tetrodotoxin is a language and toolchain platform for programs made from several
purpose specific source languages. Those languages share semantic identity and
value shape without giving up their own declaration hierarchy, Type system, or
execution model. Together their concrete objects form one live multi domain
program inside a Workspace.

The architecture divides the work by owner:

* TTX defines the lexical and semantic contracts that can be shared without
  knowing a language.
* A Dialect gives one source body grammar and semantic meaning.
* Environment gives related Monographs one Workspace lifetime and drives their
  completion.
* Package supplies the required toolchain services for reproducible composition
  and semantic Archives.
* Puffer gives command line tools and editors one application host.
* Compiler, Linker, runtime, and application components own their products and
  policies outside the live graph.

This division is useful when a Package declaration, Library Function, App
lifecycle, Scene signal, and Shader Stage need to refer to one another but do
not benefit from becoming subclasses of the same declaration record. They meet
through the TTX contracts they genuinely share and retain richer behavior in
their own languages.

The [Tetrodotoxin overview](README.md) introduces the source family and helps a
reader decide whether this architecture fits a project. This document explains
the choices and costs behind it. The precise shared contracts live in
[TTX semantics](../ttx/ttx_semantics.md).

## One toolchain, several languages

Tetrodotoxin treats language design as part of the platform rather than a
frontend that must stand alone. Each Dialect keeps the model suited to its
source questions, while TTX carries the smaller set of facts that another
language or tool can use directly.

That split gives the whole toolchain a shared shape. Environment manages live
program identity. Package manages reproducible composition. Puffer manages
command and editor sessions. Compilers and Linker own finished products. The
runtime supplies native services. Each layer can grow around the same program
without importing another layer's private representation.

A consumer reaches the language object that owns a fact until it deliberately
derives a product for a different system. CPU code may become an optimizer's
input, GPU code may become a device module, and semantic facts may become a
Package Archive. Those products carry the information needed by their next
consumer, while the live Workspace keeps the richer meaning shared by the
platform.

This is the reason the common vocabulary stays small. Tetrodotoxin can add new
application models and domain languages without turning their distinctive
concepts into fields on one central declaration record.

## The choices and their costs

None of these choices is universally better than the familiar alternative.
They move complexity to the component that has enough information to own it.

| Design choice                                       | What it makes possible                                                                    | What the project must provide                                                                                                             |
| --------------------------------------------------- | ----------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| Concrete semantic objects instead of one shared AST | A Dialect preserves the distinctions its language and tools actually use                  | Rich tooling must use that Dialect because the common TTX view is deliberately smaller                                                    |
| Workspace local borrowed identity                   | Languages and consumers share one unambiguous object without copying or merging it        | References end with their Workspace and cannot become persistent handles                                                                  |
| Retained source transactions and Workspace owned Package barriers | Incomplete edits keep their strongest semantic and lexical evidence, while one fixed manifest table retains stable identity through completion | Only completed islands enter Terminal production, and a Package member never starts another import |
| Packs distinct from Layouts                         | Empty, scalar, named, and multiple value flow can remain live without an anonymous Type    | Dialects must retain producer identity separately from the descriptor used for fitting                                                      |
| Semantic Layout                                     | One language shape can feed CPU, GPU, interpreter, editor, and archive consumers          | Every Terminal must derive and validate its own physical layout                                                                            |
| Typed Terminal products                             | Each output preserves the facts and validation contract its next consumer needs           | There is no generic product registry or common output object                                                                              |
| Dialect owned Archive payloads                      | Source independent restoration can reconstruct equivalent observable language meaning     | A persistent Dialect must maintain and validate its reconstruction schema, while a Dialect used only from source needs no Archive payload |

The architecture earns its complexity when several domains would otherwise
maintain shadow graphs or repeatedly import semantic facts between models. A
single language compiler with one AST, one typed intermediate representation,
and one Terminal may not need these boundaries.

## Direct semantic construction

A common AST is effective when all participating languages share a useful
declaration and Type model. Tetrodotoxin's Package, Library, App, Scene, Render,
and Shader languages do not. Their source forms have different invariants,
lifecycles, and consumers.

The selected Dialect therefore constructs the semantic objects defined by its
source directly. No temporary source representation survives as a second
declaration graph.

The shared `Definition` value preserves the common declaration facts:
Documentation, ordered Attributes, one Visibility, evaluation modifiers, name,
qualifier, and the exact host that admits it. It remains part of the concrete
semantic object rather than a generic declaration identity. The host supplies
mutable transaction and access authority, not universal graph parentage or a
canonical route. The concrete grammar owns legality and completes an authored
Anchor. A synthetic Definition may carry a truthful Anchor supplied by its owner
without fabricating Tokens.

Library requires every Composite and Enumeration Type to retain one Definition.
Source, Structure, and Object follow that Type rule, while Field, Function, and
authored Alias retain a Definition without changing their Addressable, Callable,
or Alias categories. Definition remains a complete concrete declaration value.
it contributes no second semantic identity, inheritance path, or lossy TTX
projection.

Every Library Monograph creates one Source Definition with the reserved name
`<source>`, which cannot be emitted. The Definition retains the opening
Documentation and Environment's exact source envelope Anchor. It invents no
Tokens. Its host is the Monograph. Ordinary
members use their containing Composite. A `using` declaration retains its
resolved object as one borrowed fallback context. It creates no Alias,
Definition, copied declaration, binding inventory, or provider closure.

Those concrete objects collectively form the shared semantic IR. The common
part is TTX identity, category, resolution, and Layout rather than a universal
node model. Each Dialect keeps the richer facts needed by its own language and
tools.

This removes a translation layer and gives a compiler or editor one stable
subject to query. Backend lowering consumes the exact Library Function. Package
retains the Alias that names another source. Shader lowering consumes the exact
Stage body. None of those consumers has to synchronize a generic declaration
node with the object that carries the language behavior. When a concrete object
retains a Definition, that value is part of the same authored object rather
than a second graph node.

The common view is consequently modest. A generic TTX tool can follow identity,
prove a shared category, retain a produced Pack, inspect its output Layout, and
ask contextual questions. An Alias is opaque: a consumer may resolve it but
cannot inspect or operate on a separate target edge. Source rewriting, language
specific completion, callable registration policy, and rich declaration
inspection belong to the concrete Dialect.

## Dialects and Monographs

A source begins with authored Documentation and one Dialect declaration:

```ttx
// Reusable image helpers.
dialect : Library;
```

Environment selects the installed Dialect named by that declaration. The
Dialect interprets the remaining body and returns one Monograph, the retained
semantic result of that source invocation.

An explicit empty comment represents intentionally empty Documentation.
Omitting the opening comment makes the source envelope malformed. Environment
passes the exact Documentation to the selected Dialect and Monograph, which
keeps authored prose attached to the semantic source it describes.

A Monograph is an Abstract context rather than a universal Type or scope. One
Dialect may expose no Types, another may expose a source root Type, and another
may expose package members or lifecycle facts. Its contextual resolution
behavior is part of the concrete language contract.

The common Monograph surface provides stable identity, Documentation, the Arena
that owns its durable semantic graph, exact Dialect layer negotiation, and the
link and finalize hooks required by Environment. Those hooks receive the Cursor
for the current textual operation rather than consulting retained diagnostics.
A Monograph answers
`get_layer(requested_dialect)` only for itself or one fixed child built from
that exact installed Dialect identity. This is capability negotiation rather
than contextual name resolution: it follows no Alias, consults no registry,
and creates no wrapper.

A plain source Monograph is one layer. A composite Dialect may own a fixed set
of child Monographs when those children are the real semantic owners of facts
used by the outer language. Only the outer Monograph is retained as the Package
member. It drives child linking and finalization with the operation's Cursor and
owns their payload framing. This is enough for a Workspace to retain
heterogeneous sources without flattening them into one member inventory or
permitting arbitrary Dialect nesting.

Adding a Dialect is therefore closest to adding a compiler frontend, not adding
an enum case to a parser. The [Language extension model](language/README.md)
describes the contract and responsibilities in detail.

## Workspace identity and completion

A Workspace is one semantic island. It borrows the concrete Dialects installed
once in its host Toolchain, retains every resulting Monograph, and gives their
borrowed TTX edges a common lifetime.

A Dialect is a stateless installed TTX Abstract context. Its immutable name and
downward Dialect dependencies may be shared across Workspaces, but semantic
state, borrowed edges, and cross language results remain inside one Workspace
lifetime. Source local transaction state never accumulates on the Toolchain.

Environment creates one source transaction Arena, copies the opened path and
bytes into it, and then constructs a Tokenizer, Associations index, and Cursor
in that Arena. Environment passes the Cursor, source backed Documentation,
source Anchor, and semantic context directly to the selected installed Dialect.
The Dialect constructs one Monograph in the Cursor's Arena and returns it
as an optional reference. Presence means the Dialect established a real
semantic root. Absence means it could not establish one.

Workspace retains one source record containing the Arena owner, outer
Monograph, and immutable Associations index whenever interpretation establishes
that root. Comments, Attributes, Tokens, semantic facts, and association edges
can therefore borrow the retained source directly. Reports written to the
Cursor during that operation keep the source from publishing, but Workspace
still attempts linking so independent and earlier facts can settle for editor
queries. Completion decides whether a Terminal may consume the result. There is
no second graph Arena or defensive source copy phase.

Installed Dialect dependencies form a strict directed acyclic graph. The host
constructing a Toolchain injects each exact dependency instance. An outer
Dialect never constructs a second instance or discovers one by name. Missing
dependencies are diagnosed before that source can complete, and a dependency
cycle is an invalid toolchain configuration. Source ownership, namespace,
path, and Bazel dependency direction must describe this same graph. A special
build carveout needed only to break a Dialect cycle is evidence that a contract
has been assigned to the wrong owner.

One direct source has four semantic stages:

1. The selected Dialect constructs one optional Monograph in the source
   transaction Arena and writes any source reports through the Cursor.
2. Workspace retains the Monograph and its lexical evidence when present.
3. Linking attempts to resolve every contextual route the retained graph can
   currently answer. Earlier declarations and independent branches can settle
   even when another source form remains incomplete.
4. Finalization runs only when interpretation and linking completed without
   errors, and performs language work that requires the complete linked island.

Workspace performs these stages synchronously with the one source Cursor. An
incomplete Monograph remains queryable as the author's current source state,
while a completed Monograph is the only state eligible for Terminal production.
Replacing an editor document rebuilds its complete Workspace session, so no
consumer keeps pointers into an older source transaction.

Package supplies a fixed Dependency and Source description table. Workspace
owns the candidate Arena handles, operation Cursors, and durable Associations
indexes. It retains every member Monograph it can create, links members whose
graphs can answer queries against the fixed Package context, and finalizes only
after every member completes interpretation and linking without errors.
Package stores only borrowed Alias mappings. A member never adds another
import, and a dependency must already be completed in the same Workspace.

This model gives immutable consumers a clear starting point. A compiler,
Archive writer, or other Terminal producer begins after completion. Only code
inside an active Package transaction may observe a route that is not settled
yet, and it must ask that question again during linking or finalization.

Authored parsing, linking, and finalization report textual failures through the
operation Cursor. The outer Monograph and every fixed child receive the matching
source Cursor explicitly during completion. Later tools recover the immutable
Associations index from Workspace using the completed outer Monograph. A
compiler receives the exact source path and bytes with the caller owned
textual error sink for its own source attributed reports. No completed consumer
recovers or retains the spent operation Cursor.

Source free system and toolchain failures use Perimortem Diagnostics with the
host's chosen severity and persistence policy. They never manufacture authored
Tokens.

The [Environment guide](environment/README.md) explains Workspace integration
and lifecycle in more detail.

## Contextual queries instead of a universal member model

Library refines the host neutral TTX Type and Addressable contracts once. The
same semantic identities and edges remain visible to other Dialects, while the
Library refinements own scalar proofs, default construction, visibility, and
Static and Self receiver behavior. A Render Type or another host neutral Type
does not acquire CPU language behavior merely because both participate in the
same graph.

Tetrodotoxin syntax identifies the semantic question being asked:

| Syntax                        | Semantic result                                                         |
| ----------------------------- | ----------------------------------------------------------------------- |
| `expression.name`             | One named value selected from the receiver Pack's applicable Layout     |
| `expression::Name`            | One exact Type result with no Library value output                      |
| `receiver -> name(arguments)` | One registered Callable invocation fitted from its argument Pack        |

Every Library access evaluates the one Expression on its left. An Expression's
exact semantic result is distinct from its output Type: the result preserves a
selected Type or Addressable identity, while the output Type states which value
operations apply. A result that selects a Type has no value output and cannot
enter Pack flow, but it retains the exact selected identity for another access.

Postfix `::` is consequently a Library access Expression. Its receiver must
produce an exact Type result, and the access produces the selected Type as its
own result. Declaration positions instead retain a type reference with no
identity. It contains one contextual route with an optional Generic argument
Layout. A Type entry may itself be another type reference, so materialization
can be nested. Without an argument Layout, the route must resolve to a Library
Type. With one, the route must resolve to a Generic formula that materializes
the exact Library Type during linking. An explicit empty Layout applies a
formula with no arguments. It is distinct from an omitted Layout. The route can
cross Alias, Package, Monograph, source root, Type, or another Abstract context
after the relevant Type inventory exists. Only its terminal result must prove
the Library Type protocol. A declaration reference never becomes an Expression
or pretends its intermediate contexts are Types.

A Structure may expose a Field, Callable, and nested Type with the same
spelling because the authored operator already identifies the query domain.
Those Addressable, Callable, and Type spaces remain independent. A Composite
rejects duplicate Callable spelling within one receiver role during
registration, while admitting the same spelling once for Static and once for
Self. Invocation therefore selects one registered Callable by name and role.
It never constructs an overload set or defers ambiguity to call time.

Address access selects one semantic Addressable and its Type. Library's
Addressable refinement forwards an explicit receiver query to its exact
Library Type as Self. A Library Type receiver makes the corresponding Static
query. The host neutral TTX contracts impose neither behavior. An Addressable
receiver may select state relative to that receiver or a const Field owned by
its Type. An exact Type receiver selects ordinary Static Fields and const
Fields. An exact Source receiver selects its ordinary Static Fields and const
Fields. Arbitrary computed values do not provide mutable Address access.
Selecting a const Field through its Type, an Addressable, or Source returns the
same foldable identity. Named flow selects the real producer at its slot without
inventing an aggregate Type. Callable access chooses Static when the evaluated
receiver result is an exact Type and Self when the receiver is a typed value. A
Terminal may materialize or eliminate a physical address without changing the
Addressable identity. A Function host grants access authority while an explicit
receiver supplies the Addressable or Source used for state selection.

No access operator inspects an Alias target. Alias resolution may reveal the
identity used by the requested category, but it does not transfer private
authority or create a second lookup path.

The benefit is that unrelated contexts can compose without one universal
member record. The cost is that a tool must know which semantic question it
wants to ask. Spelling alone cannot infer the category.

## Semantic shape and physical representation

TTX separates produced value flow from the descriptor used to fit it. A Pack
retains one producer and may carry empty, scalar, positional, named, ranged, or
composed flow. Its output Layout promises the order and applicability of those
values without becoming another semantic identity. Library uses Packs for
expressions, invocation arguments and results, returns, swizzles, and slices.
It uses Layout descriptors for Types, Fields, Function parameters and results,
and receiving declarations. Render and Shader use Layouts to agree on Stage
interfaces.

A named descriptor slot uses `.name : Type`, while a named Pack slot uses
`.name = expression`. Slot names remain independent from their source
identities. Fitting still returns the exact produced semantic object without
renaming or wrapping it. This distinction also leaves declaration owners free
to add a default with `.name : Type = expression` or infer one with
`.name := expression` without confusing a descriptor with supplied flow.

An empty Pack or one with several values remains flow and does not become an
anonymous Type.
Atomic Types expose one exact terminal value entry. An empty Layout exposes
none, so `()` can fit `[]` across Dialect boundaries without a Type identity.
A Type with an empty Layout may retain contextual facts, but it cannot enter
value flow and no Addressable can name it.

A compiler maps scalar abstract machine storage facts and derives target object
layouts, ABI alignments, offsets, pointer forms, calling convention carriers,
registers, and relocations only after the semantic graph is complete. Library
lowering is a forward operation on each real graph owner. One compiler Program
transaction retains the target configuration graph and target facts keyed by
the original Abstract identities. It never copies Library Types, Expressions,
Statements, or control owners into a Terminal model. Shader lowering follows the
same rule over its exact Render child and Shader owned bridge facts. Linker owns
object modules, symbols, relocations, target encoding, and final native products.

This separation lets several targets consume the same language meaning. It also
means Tetrodotoxin cannot answer target layout questions by consulting the
semantic Layout alone. Each Terminal must perform and verify that mapping.

Runtime policy follows the same boundary. Library defines Object identity,
aliasing, and automatic reference counted lifetime. Perimortem realizes that
contract with handles local to one worker and Bibliotheca storage, while target
lowering places construction, retain, release, and destruction at the real
value lifetime boundaries. This requires no semantic Realm, root registry,
tracing graph, collector, or hidden invocation context.

CPU target and operating system host are separate selections. A CPU target
defines ISA, data layout, and calling convention, such as x86 64 System V or
x86 64 Win64. LLVM produces the source independent Linker object contract.
Linux and Windows hosts then supply process entry, runtime and System ABI
implementations, loader inputs, executable format, and window surface policy.
Linker depends on those declared target and host facts, never on LLVM as a
semantic authority.

The GPU path is parallel. Shader and Render complete target neutral GPU facts,
the SPIR-V Terminal emits their GPU module, and Vulkan consumes that artifact
together with Graphics batches and one selected host surface. Vulkan owns
realized descriptors, offsets, commands, handles, and synchronization. Those
facts never flow downward into Shader, Render, or Library.

## Package as a composition example

Package support is part of the Tetrodotoxin toolchain. A request that composes
a Package, acquires a Package resource, or restores an Archive installs the
Package Dialect and enters its contextual model. A Workspace interpreting one
standalone source can omit Package entirely.

A Package source binds external dependencies and authored source members:

```ttx
resolve Graphics : Perimortem.Graphics = "1.0";
source Scenes::Splash from "scenes/splash.ttx";
```

`resolve` gives an external Package identity a local Alias. `source` gives one
confined input a semantic route. Package paths locate bytes, while semantic
routes identify Monographs and participate in contextual resolution.

An embedded resource operand asks the source Package for retained bytes:

```ttx
$[resources/logo.png]
```

Package defines confinement and stable resource identity. Library may interpret
the bytes as a Constant, Shader may interpret them as shader data, and another
Dialect may assign another meaning. Package transports the Resource without
acquiring the consumer's semantics.

The [Package guide](package/README.md) covers dependencies, resources, Archives,
Repositories, and restoration.

## Concrete language building blocks

The repository provides several concrete languages that can be composed as
building blocks. Library supplies the language model for CPU execution. It
refines TTX Type once and defines its scalar and Addressable refinements,
default construction, Generic materialization, Constants, Expressions,
Functions, Structs, Objects, Enumerations, and Field policy while reusing TTX
identity, Pack, and Layout contracts. Its
[language guide](library/README.md) explains those semantics.

App owns startup profiles and application lifecycle. Each Scene Monograph owns
one real Library child whose synthetic Object is the Scene instance Type. Scene
owns signals, lifecycle role edges, hosted graphics relationships, frame event
delivery, and render submission facts around that child. App owns transitions
between Scene identities.

Render declares semantic rendering interfaces and supplies the reusable GPU
semantic layer. Each Shader Monograph owns one Library CPU child and one Render
GPU child. Shader owns its source grammar, Stage organization, legality, and
the exact CPU to GPU bridge and marshaling relations between those children.
SPIR-V lowering consumes the completed GPU facts. Foreign embeds an external
ABI declaration surface inside a parent Dialect that already supports CPU
execution.

Graphics is not another Dialect. It defines the language neutral hosting and
frame submission boundary between completed Scene state and a rendering
backend. Scene keeps exact Field and Object identity, Render and Shader keep
their semantic contracts, and target resources remain with the backend
consumer.

The standard Memory, Math, System, and Graphics surfaces are ordinary Packages.
They use the same dependency, Library, Foreign, persistence, and native
publication contracts as application Packages. The compiler gives them no
private lookup path or implicit namespace.

These languages share only the TTX facts needed at their boundaries. Their
differences remain visible to the consumers that understand them.

## Terminal production and reconstruction

A Terminal product is the point where a consumer leaves the live TTX graph.
Tetrodotoxin has several typed Terminal products because their formats,
validation rules, and next consumers differ.

Dialects and Terminal producers provide complementary Toolchain composition.
The installed Dialects choose which source meanings can join a Workspace. The
selected producers choose which products can leave it. They share the completed
Workspace as a boundary rather than one base contract: Dialects own live
semantic models, while producers own lowering, projection, serialization, or
composition for one output.

A Linker object module is a native Terminal product owned by Linker. The
[Linker guide](linker/README.md) describes object input, archive resolution,
dynamic dependencies, and executable production. LLVM IR is a target Terminal
product limited to the compilation request that emits it. SPIR-V words are a
Shader Terminal product. Each product preserves the target facts its next
consumer needs, which makes it useful precisely because it can leave unrelated
language meaning behind.

Terminal is relative to the Workspace boundary. LLVM IR or an emitted MLIR
module can be a completed Tetrodotoxin product while remaining an intermediate
representation for the lowering pipeline that consumes it next.

The Package Archive is the canonical semantic Terminal product for
Tetrodotoxin use without source. Package defines the envelope, Package identity,
pinned dependencies, member routes, Dialect names, selected payload profile,
and native artifact locators. Each persistent Dialect defines the payload and
reconstruction procedure needed to create a new Monograph.

`Complete` and `Interface` are the two Archive profiles. A Complete payload
retains the public and private query contract selected by its Dialect. An
Interface payload retains only the public contract needed by dependent
consumers. For Library this includes public Types, Layouts, Fields, Callable
signatures, folded constants, ABI requests, publication relationships, and
exact artifact locators. Neither profile stores executable bodies. Debug/source
correlation and compiled code remain separate Terminal products.

The selected profile applies recursively to every embedded layer. A Complete
Scene contains the complete query contracts of its children. An Interface Scene
contains their public contracts and artifact locators. A Complete Shader
contains complete Library and Render query contracts, while its Interface
payload retains their public CPU and GPU contracts, bridge facts, and artifact
locators. The outer payload length delimits each child section, while the child
Dialect alone validates and interprets its opaque bytes.

A compiled Package behaves like a `foreign "TTX"` graph. Its restored owners
answer the same Type, Addressable, Callable, and constant queries that consumers
would ask of the live provider, while stable ABI symbols reach the separately
compiled implementation. This is why Package persistence does not need a
second executable graph.

Replaying declaration text in a later Workspace would not provide the same
continuity. The same spelling can select a different Type or Layout after a
dependency or language context changes. The Archive records the reconstruction
facts selected by the original Package transaction instead of asking a later
parser to rediscover meaning from names alone.

The Archive remains outside the semantic graph. It contains sufficient Package
and Dialect reconstruction facts rather than a serialized memory image, live
graph identities, or runtime state.

Restoration follows the ordinary ownership path:

```text
validated Package Archive
-> a fresh Environment Workspace
-> installed concrete Dialects
-> Package creates and reserves its reconstructed context
-> each outer Dialect receives its payload and that exact context
-> each Dialect returns one optional Monograph from its reconstruction Arena
-> Package links every Monograph
-> Package finalizes every Monograph
-> Workspace retains the Arenas and publishes the completed Package root
```

The restored Workspace contains new process objects that reproduce every public
observation promised by the Archive. Names, categories, represented identity
relations, semantic edges, order, Layout behavior, completion, and concrete
Dialect facts provide continuity. Internal graph shape and process addresses
do not.

A Dialect payload may be much smaller than a memory image because it records
only sufficient reconstruction facts. Compactness is a useful format property,
not the persistence contract. A persistent Dialect defines and validates each
profile it supports. Other Dialects do not have to be persistent. The common
reconstruction hook receives the destination Arena, opaque payload, and exact
Package context directly and returns one optional Monograph reference from that
Arena. It does not receive a Restoration wrapper, generic Workspace resolver,
or optional subsystem bag. Workspace owns the reconstruction Arena. Source free
payload and toolchain failures are written to Perimortem Diagnostics.

Puffer is the user facing compiler driver and LSP application shell. Its caller
or build integration supplies declared inputs and outputs. Puffer constructs
the Workspace, presents textual reports written to the caller owned source
error sink, stops before Terminal production when a source or Package
transaction fails, requests each typed product from its defining component, and
writes the declared outputs. It coordinates the transaction without becoming
another semantic model or product owner.

## Observable boundaries

The public evidence follows the same boundaries. Semantic graph queries expose
resolution, identity, fitting, ordering, and completion. An independent LLVM,
SPIR-V, object, or Archive reader validates a Terminal product. A fresh
Workspace restored from an Archive demonstrates reconstruction.

## When this architecture is worthwhile

Tetrodotoxin is a strong fit when one source tree contains several languages
that must retain their own models, when exact identity crosses those language
boundaries, and when the same completed program feeds compilers, editors,
packages, and runtimes.

A project with one small language, one semantic model, and one output may need
less host machinery. Tetrodotoxin earns its complexity when a growing system
would otherwise build a separate package graph, editor model, compiler shell,
and runtime bridge for every domain language.

Persistence is a deliberate commitment rather than a requirement for every
Dialect. A persistent Dialect owns its source semantics, completion rules,
Archive schema, payload validation, and reconstruction procedure. The reward is
that no other subsystem has to guess those facts from a representation built
for a different job.

## Documentation map

* [TTX overview](../ttx/README.md)
* [TTX design](../ttx/ttx_design.md)
* [TTX semantics](../ttx/ttx_semantics.md)
* [Language extension model](language/README.md)
* [Environment and Workspace](environment/README.md)
* [Package language](package/README.md)
* [Library language](library/README.md)
* [Standard packages](../packages/ttx/README.md)
* [Linker](linker/README.md)
* [App](app/README.md), [Scene](scene/README.md),
  [Render](render/README.md), [Shader](shader/README.md), and
  [Foreign](foreign/README.md)
