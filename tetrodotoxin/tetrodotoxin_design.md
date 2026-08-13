# Tetrodotoxin Design

Tetrodotoxin is a host for programs made from several purpose specific source
languages. It lets those languages share semantic identity and value shape
without requiring them to share one declaration hierarchy, type system, or
lowered representation. Together their concrete objects form one live multi
domain semantic IR inside a Workspace.

The architecture divides the work by owner:

* TTX defines the lexical and semantic contracts that can be shared without
  knowing a language.
* A Dialect gives one source body grammar and semantic meaning.
* Environment gives related Monographs one Workspace lifetime and drives their
  completion.
* Package supplies the required toolchain services for reproducible composition
  and semantic Archives.
* Compiler, linker, runtime, and application components own their products and
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

## What Tetrodotoxin takes from LLVM

Tetrodotoxin draws from LLVM IR's successful separation between source language
semantics and a compact representation built for optimization and code
generation. Completed CPU facts can lower to LLVM IR without asking LLVM to
remain the Package model, editor model, or durable source type system.

The same modular instinct applies earlier in the toolchain. Each Tetrodotoxin
Dialect uses a model suited to its own source questions, while TTX carries the
smaller set of facts that other languages and tools can use directly. A
consumer reaches the language object that owns a fact until it deliberately
derives a product for another system.

There has been continuing work across LLVM, including LLDB, to improve this kind
of subsystem ownership and to build compatibility representations only where a
consumer requires them. Tetrodotoxin explores the other logical extreme. It was
designed without inheriting a C frontend or debugger compatibility surface, so
independent language ownership and one composable source tree are foundational
rather than retrofitted boundaries.

LLVM IR remains an important destination in this architecture. Once IR leaves
the compilation request, it is a Terminal product containing the target facts
needed by LLVM and its downstream consumers. The live Tetrodotoxin graph keeps
the richer language meaning that lowering was allowed to discard.

The detailed comparison with LLVM IR is in
[TTX design](../ttx/ttx_design.md#lessons-from-llvm-ir).

## The choices and their costs

None of these choices is universally better than the familiar alternative.
They move complexity to the component that has enough information to own it.

| Design choice                                       | What it makes possible                                                                    | What the project must provide                                                                                                             |
| --------------------------------------------------- | ----------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| Concrete semantic objects instead of one shared AST | A Dialect preserves the distinctions its language and tools actually use                  | Rich tooling must use that Dialect because the common TTX view is deliberately smaller                                                    |
| Workspace local borrowed identity                   | Languages and consumers share one unambiguous object without copying or merging it        | References end with their Workspace and cannot become persistent handles                                                                  |
| Interpretation, linking, and finalization barriers  | Forward references and recursive groups retain stable identity while they become complete | Consumers must respect publication and treat unanswered queries observed during construction as provisional                               |
| Packs distinct from Layouts                         | Empty, scalar, named, and multiple value flow can remain live without an anonymous Type    | Dialects must retain producer identity separately from the descriptor used for fitting                                                      |
| Semantic Layout                                     | One language shape can feed CPU, GPU, interpreter, editor, and archive consumers          | Every backend must derive and validate its own physical layout                                                                            |
| Typed Terminal products                             | Each output preserves the facts and validation contract its next consumer needs           | There is no generic product registry or common output object                                                                              |
| Dialect owned Archive payloads                      | Source independent restoration can reconstruct equivalent observable language meaning     | A persistent Dialect must maintain and validate its reconstruction schema, while a Dialect used only from source needs no Archive payload |

The architecture earns its complexity when several domains would otherwise
maintain shadow graphs or repeatedly import semantic facts between models. A
single language compiler with one AST, one typed intermediate representation,
and one backend may not need these boundaries.

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

A concrete language may expose a completed authored Definition as
Authorship with no identity. Library requires every Composite and Enumeration Type
to retain one Definition. Source, Structure, and Object follow that Type rule,
while Field, Function, and authored Alias retain a Definition without changing
their Addressable, Callable, or Alias categories. Definition contributes no
second semantic identity or inheritance path.

Every Library Monograph creates one Source Definition with the reserved name
`<source>`, which cannot be emitted. The Definition retains the opening
Documentation and Environment's exact source envelope Anchor. It invents no
Tokens and Source exposes no Authorship. Its host is the Monograph. Ordinary
members use their containing Composite. Forwarding aliases created by imports
remain synthetic TTX identities without Definitions.

Those concrete objects collectively form the shared semantic IR. The common
part is TTX identity, category, resolution, and Layout rather than a universal
node model. Each Dialect keeps the richer facts needed by its own language and
tools.

This removes a translation layer and gives a compiler or editor one stable
subject to query. Library lowering consumes the exact Library Function. Package
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

The common Monograph surface provides stable identity, Documentation,
Diagnostics, and the link and finalize hooks required by Environment. That is
enough for a Workspace to retain heterogeneous sources without flattening them
into one member inventory.

Adding a Dialect is therefore closest to adding a compiler frontend, not adding
an enum case to a parser. The [Language extension model](language/README.md)
describes the contract and responsibilities in detail.

## Workspace identity and completion

A Workspace is one semantic island. It installs the concrete Dialects available
to a tool, retains every resulting Monograph, and gives their borrowed TTX edges
a common lifetime.

A Dialect may keep localized graph state, but every borrowed edge and cross
language query remains inside the one Workspace lifetime.

Source construction has three semantic stages:

1. Interpretation reserves identities derived from source and records authored
   routes that may still be unanswered.
2. Linking connects those routes after the complete source group is known.
3. Finalization performs language work that requires linked declarations.

Every Monograph in a retained group links before any Monograph finalizes. The
group becomes publicly queryable as completed input only after both barriers
succeed.

These barriers let recursive and mutually dependent sources keep stable
identity. A route may be unanswered during interpretation and become valid
after linking. Once a query returns an identity successfully, later work cannot
redirect it to another object.

This model gives immutable consumers a clear starting point. A compiler,
Archive writer, or other Terminal producer begins after completion. A
tool that chooses to inspect the graph earlier accepts that negative answers
are provisional.

A Monograph retains the ordered Diagnostics produced during interpretation and
completion. Environment combines each retained Diagnostic with the source
Origin only when it presents the authored error.

The [Environment guide](environment/README.md) explains Workspace integration
and lifecycle in more detail.

## Contextual queries instead of a universal member model

Tetrodotoxin syntax identifies the semantic question being asked:

| Syntax                        | Semantic result                                                         |
| ----------------------------- | ----------------------------------------------------------------------- |
| `expression.name`             | One named value selected from the receiver Pack's applicable Layout     |
| `expression::Name`            | One exact Type result whose Library output Type is `Descriptor`         |
| `receiver -> name(arguments)` | One registered Callable invocation fitted from its argument Pack        |

Every Library access evaluates the one Expression on its left. An Expression's
exact semantic result is distinct from its output Type: the result preserves a
selected Type or Addressable identity, while the output Type states which value
operations apply. A result that selects a Type uses the singleton `Descriptor`
output Type without copying or wrapping the selected Type.

Postfix `::` is consequently a Library access Expression. Its receiver must
produce an exact Type result, and the access produces the selected Type as its
own result. Declaration positions instead retain a type reference with no
identity. It contains one contextual route with an optional Generic argument
Layout. A Type entry may itself be another type reference, so materialization
can be nested. Without an argument Layout, the route must resolve to a Type. With one,
the route must resolve to a Generic formula that materializes the exact Type
during linking. An explicit empty Layout applies a formula with no arguments.
It is distinct from an omitted Layout. The route can cross Alias, Package, Monograph,
source root, Type, or another Abstract context after the relevant Type inventory
exists. A declaration reference never becomes an Expression or pretends its
intermediate contexts are Types.

A Structure may expose a Field, Callable, and nested Type with the same
spelling because the authored operator already identifies the query domain.
Those Addressable, Callable, and Type spaces remain independent. A Composite
rejects duplicate Callable spelling within one receiver role during
registration, while admitting the same spelling once for Static and once for
Self. Invocation therefore selects one registered Callable by name and role.
It never constructs an overload set or defers ambiguity to call time.

Address access selects one semantic Addressable and its Type. An Addressable
receiver may select state relative to that receiver or a const Field owned by
its Type. An exact Type receiver selects ordinary Static Fields and const
Fields. An exact Source receiver selects its ordinary Static Fields and const
Fields. Arbitrary computed values do not provide mutable Address access.
Selecting a const Field through its Type, an Addressable, or Source returns the
same foldable identity. Named flow selects the real producer at its slot without
inventing an aggregate Type. Callable access chooses Static when the evaluated
receiver result is an exact Type and Self when the receiver is a typed value. A
backend may materialize or eliminate a physical address without changing the
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
none, so `Void`, `[]`, `()`, and other domains with no values can fit across
Dialect boundaries without a shared `Void` Type, but no Addressable can name
them.

A compiler maps scalar abstract machine storage facts and derives target object
layouts, ABI alignments, offsets, pointer forms, calling convention carriers,
registers, and relocations only after the semantic graph is complete.
Library lowering can consume completed CPU facts owned by Library, App, or Scene
without converting those Monographs into Library source. Shader lowering
consumes Shader facts independently. Linker owns object modules, symbols,
relocations, target encoding, and final native products.

This separation lets several targets consume the same language meaning. It also
means Tetrodotoxin cannot answer target layout questions by consulting the
semantic Layout alone. Each backend must perform and verify that mapping.

Runtime policy follows the same boundary. Library defines Object identity and
language lifetime. Allocation strategy, collector policy, pointer
representation, and reclamation timing belong to the runtime that realizes
those promises.

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
defines concrete scalar Types, Generic materialization, Constants, Expressions,
Functions, Structs, Objects, Enumerations, and Field policy while reusing TTX
identity, Pack, and Layout contracts. Its
[language guide](library/README.md) explains those semantics.

App owns startup profiles and application lifecycle. Scene owns live Scene
state, signals, hosted graphics relationships, render submission facts, and
lifecycle roles. App owns transitions between Scene identities instead of
asking Library to turn those concepts into ordinary source declarations.

Render declares the semantic interfaces used by rendering. Shader implements a
Render contract, owns GPU Stage bodies, and lowers completed facts into a GPU
Terminal product. Foreign embeds an external ABI declaration surface inside a
parent Dialect that already supports CPU execution.

Graphics is not another Dialect. It defines the language neutral hosting and
frame submission boundary between completed Scene state and a rendering
backend. Scene keeps exact Field and Object identity, Render and Shader keep
their semantic contracts, and target resources remain with the backend
consumer.

The standard Math, System, and Graphics surfaces are ordinary Packages. They
use the same dependency, Library, Foreign, persistence, and native publication
contracts as application Packages. The compiler gives them no private lookup
path or implicit namespace.

These languages share only the TTX facts needed at their boundaries. Their
differences remain visible to the consumers that understand them.

## Terminal production and reconstruction

A Terminal product is the point where a consumer leaves the live TTX graph.
Tetrodotoxin has several typed Terminal products because their formats,
validation rules, and next consumers differ.

A Linker object module is a native Terminal product owned by Linker. The
[Linker guide](linker/README.md) describes object input, archive resolution,
dynamic dependencies, and executable production. LLVM IR is a target Terminal
product limited to the compilation request that emits it. SPIR-V words are a
Shader Terminal product. Each product preserves the target facts its next
consumer needs, which makes it useful precisely because it can leave unrelated
language meaning behind.

The Package Archive is the canonical semantic Terminal product for
Tetrodotoxin use without source. Package defines the envelope, Package identity,
pinned dependencies, member routes, Dialect names, and native artifact
locators. Each persistent Dialect defines the payload and
reconstruction procedure needed to create a new Monograph.

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
-> each Dialect constructs a new Monograph in the model it owns
-> Environment retains the complete group
-> every Monograph links
-> every Monograph finalizes
-> completed root names become visible
```

The restored Workspace contains new process objects that reproduce every public
observation promised by the Archive. Names, categories, represented identity
relations, semantic edges, order, Layout behavior, completion, and concrete
Dialect facts provide continuity. Internal graph shape and process addresses
do not.

A Dialect payload may be much smaller than a memory image because it records
only sufficient reconstruction facts. Compactness is a useful format property,
not the persistence contract. A persistent Dialect is one that defines and
validates a complete reconstruction payload. Other Dialects do not have to be
persistent.

Puffer is the user facing compiler driver and LSP application shell. Its caller
or build integration supplies declared inputs and outputs. Puffer constructs
the Workspace, stops before Terminal production when Diagnostics exist,
requests each typed product from its defining component, and writes the
declared outputs. It coordinates the transaction without becoming another
semantic model or product owner.

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

It is a weaker fit when a project needs one established language frontend, one
generic rewrite IR, or immediate access to a mature optimizer and debugger
ecosystem. Those projects can use Clang, MLIR, or LLVM directly with less host
machinery. When Tetrodotoxin is used with a downstream IR system, completed
facts can lower into a suitable representation such as LLVM IR.

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
