# Tetrodotoxin Language

This is where a new language becomes part of Tetrodotoxin instead of another
tool beside it. A language that joins here can share source handling,
diagnostics, Packages, editor sessions, cross language navigation, and Terminal
production while keeping the grammar and meaning that motivated it in the
first place.

Tetrodotoxin calls that language a **Dialect**. For each source, the Dialect
turns authored Tokens into a **Monograph**, the lasting semantic result that
other languages and tools can inspect. Related Monographs live together in one
Workspace and can refer directly to one another.

Package, Library, App, Scene, Pipeline, and Shader all use this lifecycle. Their
objects expose the TTX Types, Layouts, and relationships useful across the
platform, while language aware tools remain free to explore their richer domain
models. No common syntax tree has to stand in for the real program.

Dialects are the input side of Toolchain composition. They determine which
meanings a Workspace can construct. Terminal producers form the complementary
output side after completion, which lets a new Dialect participate in several
products without carrying Terminal policy in its language model.

## When to implement a Dialect

A Dialect is appropriate when a source body has its own grammar, semantic
invariants, and completion work. A spelling variation over an existing language
usually belongs in that language instead. A grammar rule can be shared by
several Dialects when the complete construct and returned contract are genuinely
the same.

Adding a Dialect means owning the complete source language contract. The Dialect
defines how source is read, how names are resolved, which errors are reported,
how its result is completed, and what an Archive must store. General tools still
use the common TTX surface, while richer tooling uses the concrete Dialect.

Every top level Dialect provided by this repository publishes a canonical G4
grammar reference for authored language shape and parse order. These references
describe valid input. The toolchain does not generate or run its parsers from
them. A custom Dialect owns its grammar but does not have to express it in G4.

The shared grammar uses `Definition` for the common prefix of a declaration.
It contains Documentation, Attributes, Visibility, evaluation modifiers, and a
name followed by `:`. The concrete language reads the qualifier that follows
and decides what kind of declaration it creates. Definition records how that
object was introduced, but it is not a second declaration object or a universal
syntax tree node.

Descriptor Layouts share a second small grammar block. Language reads optional
brackets, commas, Attributes, and `.name :` prefixes, then lends each entry to
the active Dialect. Library can create parameter and result edges with Generic
Type routes, while Pipeline can create contract slots with simpler Type routes.
The punctuation is shared without turning either semantic Layout into the
other.

Every Definition also remembers the language object that hosts it. The host
records where the declaration was admitted and which private access it may use.
It is not a universal parent link. Authored Definitions gain their source Anchor
only after the complete declaration parses successfully. A language can also
create a generated Definition with a truthful Anchor, but generated declarations
never pretend that source Tokens were authored for them.

An Attribute is an ordered key with at most one scalar value. A Definition can
keep any number of Attributes, including repeated keys. Definition and the
concrete declaration preserve those facts without predicting which later system
will use them. A compiler, embedding language, tool, or other consumer decides
the meaning and validity of only the keys it actually consumes. The shared
parser only preserves the authored data.

## Dialect

A Dialect interprets one kind of source body. Environment Toolchain installs
each concrete Dialect under the exact name accepted by the source envelope:

```ttx
// Reusable source.
dialect : Library;
```

Workspace lends the selected provider a retained source input and the semantic
context. The provider returns one source graph whose root and typed source
services remain available together. A native provider reads the common envelope
with a Cursor and passes its documentation and anchor to the concrete Dialect.
Another frontend may construct its graph directly from its own source format.
Both return through the same ownership boundary, including when a partial graph
can support tooling but cannot yet produce an immutable output.

An installed Dialect is itself an ordinary TTX Abstract context. Its exact live
identity selects Monograph layers, its installed name answers source dispatch,
and its contextual resolution exposes immutable language vocabulary. A Dialect
is stateless after Toolchain construction and can serve every Workspace that
borrows that Toolchain. Parsing state and allocations belong to the provider's
source graph, which Workspace retains until replacement or release. The installed
provider and its dependencies outlive every source graph that uses them.

The context local to a source during interpretation is an ordinary TTX
Abstract. A direct source may receive the Workspace, while every source in a
Package graph receives that Package root. A Monograph exposes contextual names
with an empty value Layout. Its imports retain stable source authorities, so a
later query can reach newly supplied meaning without interpreting the importer
again. The concrete Dialect decides which questions each imported root supports.

Package can be installed in a Tetrodotoxin Toolchain without becoming an
implicit context for every source. A standalone Toolchain may omit the Package
Dialect. Package participates when the request composes a Package, acquires its
resources, or restores an Archive.

## Source transaction

Workspace owns publication and the lifetime of its current source generations.
Providers own their graph allocations and expose source services through the
same typed C boundary. Using a C++ Arena is a native implementation choice, so
another frontend can use its own allocator or managed storage without adding a
second Workspace path.

A provider which borrows source bytes retains the source input. Its returned
source graph owns the semantic root, diagnostics, and source associations needed
by readers. Diagnostics and associations are synchronous projections from that
owner. An incomplete graph can still provide those services while its root or
some of its routes remain Unknown.

Replacing a source publishes another generation through its stable authority.
References query that authority again. Callers keeping immutable Packs across
replacement retain the supplying source generations until those producer
borrows end. Context retains Layout support, but does not retain source graphs
on the caller's behalf. This lets unused generations be reclaimed promptly.

### Native construction

The native provider retains the input bytes and owns an Arena, Tokenizer,
associations, and parse reports. Its operation local Cursor lends that storage
to the concrete Dialect, which uses `Cursor::get_arena()` to construct its
Monograph and source backed support values. Comments and attributes can then
borrow authored bytes without forcing another frontend to adopt a C++ allocator.

Workspace retains the returned source graph even when its root is Unknown.
That keeps useful diagnostics available after parsing fails before a Monograph
exists. When a root does exist, read only validation reports which current
relationships remain incomplete. An embedded native layer uses the same Cursor,
Arena, and semantic context with its exact installed child language identity.

Reconstruction follows the same ownership split. The provider owns reconstructed
allocations and receives the exact package context, while Workspace retains the
resulting source graph. A fixed child receives its own opaque payload section
and the same context. Reconstruction failures use their source free diagnostic
contract rather than manufacturing a Cursor over nonexistent authored text.

## Dialect dependencies

Some languages build on the work of another language. Scene authors Library
state and functions in one owned child. Shader also owns a Library child for its
executable Program and Stage meaning, while selecting Pipeline contracts through
its Workspace context. In both cases, Toolchain installs each dependency once
and every source observes the same Dialect identity.

Dependencies only point from a higher level language to a lower level one.
Library does not depend on Scene or Shader. Pipeline does not depend on Shader,
and Shader does not depend on Vulkan. This rule also applies to build targets.
If two language targets need each other, the shared contract belongs in a
lower level owner.

When a required language is missing, Tetrodotoxin reports the problem before it
tries to finish the source. A dependency loop is always an invalid Workspace.

## Contextual resolution

`resolve()` follows represented identity. `resolve_concept(name)` asks the
receiving Abstract to interpret one borrowed, unqualified concept in its own
domain. A concrete grammar operator owns punctuation, resolves a selected Alias,
and issues the next segment as another query. No Abstract accepts `A::B` as one
lookup key. The consumer then proves the category required by its grammar.

Concrete languages compose shared concept questions instead of adding operator
modes to Abstract:

1. An Addressable receiver asks its exact Type for `instance`, then asks that
   authority for the authored name.
2. A Type receiver asks itself for `static`, then asks that authority for the
   authored name.
3. The consuming language proves Addressable, Type, or Callable only after the
   concept has resolved.

The resulting questions stay nested and factual. They do not flatten member
names, invocation, or receiver policy into a shared routing table.

## Monograph

A Monograph is the retained result of reading one source with one Dialect. It
provides:

* Stable TTX identity.
* Opening documentation.
* Native graph storage supplied by its provider.
* Name resolution defined by its Dialect.
* Repeatable read only validation of its current answers.

A Monograph may expose no Types, one global Type, several independent Types,
package members, entry policy, or another semantic context. Its role is the
retained root of one source, not a promise that every language has the same
shape.

A Monograph may contain a small, fixed set of child layers when the outer source
actually authors meaning owned by that child. A Scene contains one Library
layer. Shader also contains one Library layer because its Stage bodies directly
author Library execution meaning. The selected Pipeline contract remains a
neighboring Workspace identity rather than a child.

This lookup is intentionally narrow. It does not search by name, follow Aliases,
or create a wrapper around the child. A top level Monograph answers with itself.
Scene and Shader answer with their real Library child. Any other request has no
result.

Workspace retains each outer source graph and publishes it through a stable
source authority. The package's restricted Library source exports identities,
while common imports name the graph. A fixed child remains owned by its outer
Monograph rather than becoming a separate archive member.

A Monograph remains queryable while its supplying generation is retained and
its borrowed Workspace and provider still exist. Keeping an older generation
preserves existing producer borrows, but does not freeze live Reference answers.
A query after replacement therefore retains the source closure it observes.

The concrete Dialect creates its Types, Addressables, Callables, lifecycle facts,
or Package members directly. An object keeps the Definition that introduced it,
while the Dialect decides which other facts remain part of the completed
language model. The Monograph exposes those real objects. Environment does not
wrap them in generic declarations or copy them into a shared member list.

Shared grammar rules return the complete semantic result requested by the
concrete Dialect. Definition preserves only its common authored prefix and is
retained directly instead of becoming an intermediate declaration model.

## Resource and Error

Two Abstract contracts shared across Dialects let a semantic context answer
requests without sharing its private policy.

### Resource

`Language::Resource` exposes stable retained bytes acquired by another owner.
It does not assign those bytes a Type or interpretation. Empty bytes are a
successful Resource.

For example, Package can resolve `$[resources/icon.png]` to a Resource while
Library constructs a Bytes Constant and Shader constructs a fact defined by its
own language from the same result.

### Error

`Language::Error` represents a contextual request that was recognized but
failed in the receiving domain. The concrete owner retains the cause. The
source consumer supplies the authored location and presentation.

An unrecognized semantic name still resolves to TTX `Unknown`. Resource and
Error therefore distinguish successful data, recognized failure, and ordinary
absence without introducing a universal error enum.

## Failure reporting

The native source graph retains parsing reports beside its authored bytes.
The outer Monograph and its fixed children receive an operation local Cursor
when parsing or validating, so reports keep their source locations without
keeping the Cursor alive. Typed source services lend diagnostics and associations
to editors and other consumers regardless of the provider's implementation
language.

Validation may produce new reports about current references without rewriting
the earlier parse reports. Binary archive and other source free failures keep
their own reporting context rather than inventing an authored Token.

## Semantic lifecycle

The provider constructs a source graph once, then Workspace connects its
declared dependencies to stable authorities and publishes that generation.
Queries follow those authorities whenever they need the current answer. Adding
another source can therefore satisfy an import without reparsing its consumer.

Read only validation determines whether the requested output has enough evidence
to proceed. Tooling can inspect partial generations immediately, while immutable
production waits for its required relationships to be complete. Replacing a
source changes its authority's current graph, leaving unrelated sources and
previously retained immutable support snapshots intact.

## Persistence

A language that supports Archives defines the data needed to rebuild one of its
Monographs without the original source. Languages that are always read from
source do not need an Archive format. Package stores each language's data under
the corresponding member and leaves its contents to that language.

Each persistent payload keeps the complete public and private facts promised by
its Dialect. It contains no executable bodies. Native objects, SPIR-V, and
other compiled implementations remain separate Terminal products.

Child layers belong to the same complete graph. The outer language stores an
opaque section for each child, and only the child's language reads and checks
that section. Payloads store no parser state, temporary caches, generated IR,
live runtime handles, or process addresses. Debug symbols and source mapping
belong to a separate output.

Archive reconstruction creates a fresh graph with equivalent observable
semantic relationships. Its queries and read only validation follow the same
contracts as authored source. Package remains independent of the payload schema.

The payload is part of a Terminal product and carries reconstruction facts
rather than live graph identities. Equivalence means that a fresh Workspace
exposes the same observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and language facts. The
internal graph shape and process addresses may differ.

A payload may be much smaller than a memory image because it records only the
owner facts needed for those observations. Compactness is a format benefit. It
does not define whether a Dialect is persistent.

When reconstructing a Package, Workspace creates its restricted export
Monograph, restores every member, and then reacquires the archived external Type
graph. Resources are reconstructed before a payload that refers to them. Scene
and Shader pass the surrounding context to their child layers. Exact Package
dependencies still come from the Workspace. If a child rejects its data, the
outer Monograph also fails.

The provider validates its bounded payload before returning a reconstructed
source graph. Workspace retains that graph, connects its dependencies, and
validates the observations required for the requested product. A target
representation such as LLVM IR cannot substitute for this payload because it
has already lost owner facts that were meaningful in the source language.

## Shared source envelope

The shared envelope contains required opening Documentation and one Dialect
declaration:

```ttx
// Package source documentation.
dialect : Package;
```

An explicit empty comment represents intentionally empty Documentation.
Absence is a malformed source envelope. Environment passes the exact
source backed Documentation directly to the selected Dialect, and the resulting
Monograph retains it.

Concrete body grammar starts immediately afterward. A grammar rule belongs to
the shared Language layer only when multiple concrete Dialects use its source
shape and its returned semantic contract.

See [Environment](../environment/README.md) for Workspace lifetime and
[TTX semantics](../../ttx/ttx_semantics.md) for the Abstract query model.
