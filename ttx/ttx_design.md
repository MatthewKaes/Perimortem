# TTX Design

TTX is the shared semantic boundary between concrete languages and the systems
that consume their work. It keeps exact identity, resolution, Type,
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
directly. Facts meaningful to only one of those systems stay with that owner.

The shared vocabulary contains relationships that are genuinely cross domain
rather than source features owned by one language.
A Library Object, a Package Dependency, a Scene signal, and a Shader resource
can all expose exact TTX identities while retaining their richer rules in their
own domains.

The benefit is that a consumer can ask the original semantic object for the
contract it needs. The cost is that there is no universal declaration record to
inspect when the concrete owner has not exposed that contract.

## Lessons from LLVM IR

TTX draws heavily from LLVM IR's success as a focused intermediate
representation.
LLVM IR has a clear place in the compilation pipeline, a bounded common
vocabulary, explicit context and module ownership, and well defined transitions
into target products. TTX applies those lessons to an earlier boundary where
source languages and tools still need access to semantic meaning.

The first column names an LLVM IR design area. The other columns show what TTX
carries forward and where it chooses a different boundary.

| LLVM IR | Similarities | Key differences |
| --- | --- | --- |
| **Primary role.** LLVM IR represents a lowered program for optimization and code generation | Both provide a focused vocabulary between producers and consumers | TTX represents semantic identity and applicability before a concrete lowering has been chosen |
| **Type identity.** LLVM Types belong to an LLVM context and describe values admitted by the lowered program | Both make identity meaningful within an owning live context | A TTX Type is the exact object constructed by its language owner. Equal structure or equal Layout never creates Type identity |
| **Physical layout.** LLVM DataLayout gives target size and alignment meaning to IR Types | Both require target specific information before typed input becomes a physical machine representation | TTX Layout answers semantic fitting only. Scalar Values define abstract machine width and storage requirements. Target object layout, offsets, registers, address spaces, pointer forms, and calling conventions belong to the Terminal producer |
| **Composition.** LLVM links lowered modules and definitions | Both let independently constructed parts meet through explicit contracts | TTX connects owner constructed identities while their richer language semantics are still live in one graph |
| **Transformation.** LLVM passes intentionally replace instructions and values | Both allow later stages to derive more specific facts | A successful TTX identity remains stable while later phases connect it, complete it, and answer richer queries |
| **Extension model.** LLVM extends its program model through instructions, intrinsics, metadata, passes, and targets | Both keep specialized facts with specialized producers and consumers | TTX concrete languages retain their complete models behind a closed shared semantic vocabulary |
| **Source relationship.** LLVM IR intentionally leaves most source structure behind during lowering | Neither representation needs to be a universal syntax tree | TTX leaves spelling, syntax, and source shaped facts with the concrete language owner from the beginning |
| **Durable form.** LLVM textual IR and bitcode preserve LLVM IR outside one process | Both require an explicit format when a result must outlive its producer | The TTX graph has no generic serialized form. Each Terminal producer defines its output, and semantic restoration constructs a fresh graph from facts defined by their semantic owners |
| **Verification.** LLVM IR structure is a public product accepted by verifiers and downstream tools | Both benefit from testing observable behavior through independent consumers | TTX graph shape matters only where identity, order, resolution, or another semantic contract makes it observable. A Terminal is proved through its own format and consumer |
| **Ecosystem.** LLVM provides optimizers, code generators, debugger integration, and broad language support | Both are infrastructure intended to be embedded in larger systems | TTX stays focused on semantic composition and relies on systems such as LLVM for downstream optimization and target support |

There has also been continuing work across LLVM, including LLDB, to make source
language, debugger, and expression evaluation models more modular. Those
systems must preserve established APIs and C and C++ behavior. TTX follows the
same ownership direction from the other logical extreme. TTX was designed
without inheriting a frontend or debugger compatibility surface, so each
concrete language owns its complete set of Types from the beginning.

LLVM IR remains the stronger choice once a project needs its optimizer, target
model, code generators, or surrounding tool ecosystem. TTX is useful earlier,
when several language and tool owners must share exact semantic relationships
without turning one frontend or backend representation into the universal
model. The two layers can be used together.

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
│   └── Value
│       ├── Flag
│       ├── Real
│       ├── Signed
│       └── Unsigned
├── Addressable
└── Callable
```

`Documentation`, `Layout`, `Reference`, and `Attribute` are supporting values.
They describe or connect identities without acquiring another semantic
identity.

The graph is not a tree of declarations. One Type may be reached through a
Package Alias, a source Alias, a Generic materialization, and a direct local
route. Those paths retain one Type and do not give it a required parent.

Structural similarity therefore says nothing about identity. Two Types may
have equal Layouts and still mean different things. Two Alias objects may have
different local names and Documentation while representing the same target.
One Addressable keeps its own identity while reaching a Type. A Callable keeps
its own identity while its parameters and results expose Layouts.

This gives tools and lowerers the original semantic fact. It also means they
must use the contracts exposed by its owner. TTX offers no synchronized member
record or cloned Type graph for a consumer that wants a different shape.

## Reference and graph lifetime

A Reference is a nonnull borrowed edge to one exact semantic object. It remains
valid for the lifetime established by the graph owner and carries no absent
state.

A Reference preserves the exact object it receives. Following an Alias,
proving a Type, or reaching the Type of an Addressable remains an explicit
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

`resolve()` follows represented identity. `resolve_context(route)` asks the
receiving Abstract to interpret a borrowed route in its own domain. A result
may answer another contextual query, so a route can cross Alias, Package,
Monograph, source, and Type contexts without converting those contexts into one
common category.

The caller owns the expected contract and proves the returned identity against
the semantic category it needs. Route spelling does not infer a Type,
Addressable, Callable, or another category on the caller's behalf.

This lets one route cross several concrete contexts without requiring a
universal member table, overload set, scope object, or collision domain. The
receiving owner interprets the route it recognizes, and the concrete language
assigns meaning to the operator that initiated the query.

The cost is deliberate locality. Contextual resolution is not a global search
service, so a caller cannot rely on unrelated domains or a registry to repair
an ambiguous ownership path.

## Progressive construction

A graph owner may reserve a stable identity before all of its edges are ready.
An incomplete total query returns the shared `Invalid` object. Completion may
make that unanswered query valid, while every successful identity remains
stable.

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

## Layout is semantic shape

A Layout retains an ordered view of exact Abstract identities and answers
whether one view fits another. Fitting is directional because a source Layout
supplies the values required by a target Layout.

`Fluid` compares entries in order. `Named` compares uniquely named entries by
name and represented identity. `Ranged` repeats one entry across a fixed
interval. `Composite` preserves two complete child Layouts.

Successful fitting returns the original source edge that supplies a target
position. Layout retains order and applicability without copying field names,
Types, Documentation, Attributes, defaults, or storage facts into a generic
member record.

Keeping Layout semantic lets the same graph feed a CPU compiler, GPU compiler,
interpreter, editor, and archive writer without letting the first backend fix
the physical meaning for every later consumer.

TTX therefore cannot answer target object layout, field offset, register,
address space, pointer form, or calling convention questions by itself. Each
compiler derives and validates those facts for its own Terminal. That extra work
is the price of keeping the graph target neutral.

An empty Layout is a valid shape. It says nothing about whether a Type is a
scalar leaf or whether construction is complete.

## Terminal products and reconstruction

A Terminal is a completed output that leaves the live semantic graph. Its use
is independent of that graph and its process identities. Examples include
formatted text, editor data, LLVM IR, SPIR-V words, debug data, object modules,
native binaries, and semantic archives.

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

A semantic archive has a different purpose. It lets a graph owner avoid source
acquisition, lexing, and parsing when it retains the facts and relations
required by every included language. The result is a fresh graph with
equivalent observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and concrete owner facts,
followed by the graph owner's validation, completion, and publication contract.
Its supporting shape and process addresses may differ because they are not
public semantic observations.

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

LLVM IR is the direct choice when the problem begins with lowered computation.
MLIR is the stronger foundation when extensible operation based transformation
is the central design. Clang is the stronger foundation when faithful C or C++
semantics and tooling are the product. A conventional language AST and typed
intermediate representation are usually simpler when one frontend owns the
whole program.

TTX occupies the earlier boundary where independent owners still need to share
meaning. Concrete languages decide which Types exist, how names are published,
which writes are legal, how Callables are selected, and how expressions
evaluate. Targets, runtimes, packages, and tools consume those facts without
becoming new TTX categories.
