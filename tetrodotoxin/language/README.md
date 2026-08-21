# Tetrodotoxin Language

The Tetrodotoxin Language contract explains how a source language joins a
Workspace. Each language is a Dialect. It reads the Tokens for one source and
returns a Monograph, which is the lasting result that tools and other languages
can inspect.

Package, Library, App, Scene, Render, and Shader all follow this lifecycle. They
do not translate their source into one common syntax tree or type system. Each
Dialect keeps the rules that make its language unique and exposes shared TTX
Types, Layouts, and relationships where another tool can use them.

Together, the Monographs form the live program in a Workspace. General tools
can use their shared TTX surface, while language-aware tools can ask a concrete
Dialect for richer details.

## When to implement a Dialect

A Dialect is appropriate when a source body has its own grammar, semantic
invariants, and completion work. A spelling variation over an existing language
usually belongs in that language instead. A grammar rule can be shared by
several Dialects when the complete construct and returned contract are genuinely
the same.

Adding a Dialect means owning the complete source-language contract. The Dialect
defines how source is read, how names are resolved, which errors are reported,
how its result is completed, and what an Archive must store. General tools still
use the common TTX surface, while richer tooling uses the concrete Dialect.

Every top-level Dialect provided by this repository publishes a canonical G4
grammar reference for authored language shape and parse order. These references
describe valid input. The toolchain does not generate or run its parsers from
them. A custom Dialect owns its grammar but does not have to express it in G4.

The shared grammar uses `Definition` for the common prefix of a declaration.
It contains Documentation, Attributes, Visibility, evaluation modifiers, and a
name followed by `:`. The concrete language reads the qualifier that follows
and decides what kind of declaration it creates. Definition records how that
object was introduced, but it is not a second declaration object or a universal
syntax-tree node.

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

Environment consumes the envelope with one source transaction Cursor and calls
the selected Dialect directly with that Cursor, the source-backed
Documentation, its Anchor, and the semantic context. The Dialect constructs one
Monograph in the Cursor's Arena and returns it through an `Option`. Absence is
the only parse-failure result; there is no second success flag or transaction
wrapper. Environment links and finalizes that Monograph before retaining its
Arena and publishing it.

An installed Dialect is itself an ordinary TTX Abstract context. Its exact live
identity selects Monograph layers, its installed name answers source dispatch,
and its contextual resolution exposes immutable language vocabulary. A Dialect
is stateless after Toolchain construction and can serve every Workspace that
borrows that Toolchain. The operation-local Cursor traverses the source and
publishes textual reports, while Workspace's local Arena handle carries the
produced root until the Workspace retains or releases it.

The context local to a source during interpretation is an ordinary TTX
Abstract. A direct source may receive the Workspace. A Package member may
receive its Package Monograph. The concrete Dialect decides which contextual
queries that object supports.

Package can be installed in a Tetrodotoxin Toolchain without becoming an
implicit context for every source. A standalone Toolchain may omit the Package
Dialect. Package participates when the request composes a Package, acquires its
resources, or restores an Archive.

## Source transaction

Environment owns one local Arena handle and constructs the retained source
bytes, Tokenizer, and operation-local Cursor in that Arena. It passes the
Cursor, opening Documentation, source Anchor, and source semantic context
directly to the selected installed Dialect. The Dialect uses
`Cursor::get_arena()` for every source-backed semantic fact and returns one
optional Monograph reference from that same Arena.

Comments, Attributes, Tokens, and semantic objects may therefore retain direct
source-backed views without proxying them into another domain. Workspace keeps
the Arena handle only after the Monograph completes; dropping a failed handle
releases the whole transaction. An embedded layer uses the same Cursor, Arena,
and semantic context with its exact child language identity; it does not add a
transaction wrapper or temporarily mutate shared Dialect state.

Archive reconstruction does not introduce a parallel Restoration context. A
persistent Dialect receives its destination Arena, opaque payload, and exact
Package context directly and returns one optional Monograph reference through
the same ownership contract. A fixed child receives its own payload section
with that same Package context. Source-free validation and toolchain failures are written to
Perimortem Diagnostics instead of manufacturing a source Cursor.

## Dialect dependencies

Some languages build on the work of another language. Scene uses Library for
its CPU state and functions. Shader uses Library for CPU helpers and Render for
GPU data. The Workspace creates these dependencies once and gives each language
the same shared instance.

Dependencies only point from a higher-level language to a lower-level one.
Library does not depend on Scene or Shader. Render does not depend on Shader,
and Shader does not depend on Vulkan. This rule also applies to build targets.
If two language targets need each other, the shared contract belongs in a
lower-level owner.

When a required language is missing, Tetrodotoxin reports the problem before it
tries to finish the source. A dependency loop is always an invalid Workspace.

## Contextual resolution

`resolve()` follows represented identity. `resolve_context(name)` asks the
receiving Abstract to interpret one borrowed, unqualified name in its own
domain. A concrete grammar operator owns punctuation, resolves a selected Alias,
and issues the next segment as another query. No Abstract accepts `A::B` as one
lookup key. The consumer then proves the category required by its grammar.

Three questions recur across the provided languages:

1. Address access selects an Addressable through an applicable Layout.
2. A context route reaches an object, and the consuming position checks that it
   belongs to the required category.
3. Call access lets a concrete language select and invoke a Callable using its
   parameter and result Layouts.

These are shared question domains rather than one universal operator grammar.
Each Dialect decides which questions its source can ask and what additional
policy applies.

## Monograph

A Monograph is the retained result of reading one source with one Dialect. It
provides:

- stable TTX identity
- opening Documentation
- the source transaction Arena that owns source bytes and its semantic graph
- name resolution defined by its Dialect
- link and finalize lifecycle stages

A Monograph may expose no Types, one global Type, several independent Types,
package members, entry policy, or another semantic context. Its role is the
retained root of one source, not a promise that every language has the same
shape.

A Monograph may contain a small, fixed set of child layers built by its language
dependencies. A Scene contains one Library layer. A Shader contains one Library
layer and one Render layer. Tools can ask the outer Monograph for a layer by
using the same Dialect instance installed in the borrowed Toolchain.

This lookup is intentionally narrow. It does not search by name, follow Aliases,
or create a wrapper around the child. A top-level Monograph answers with itself.
A Scene answers with its Library child. A Shader answers with its Library or
Render child. Any other request has no result.

The Package table borrows only each outer member Monograph. Workspace owns those
member handles and moves them through linking and finalization with their exact
source Cursors. A fixed child layer remains owned by its outer Monograph and does
not become a separate Package member or copied view of the same declarations.

The Monograph remains queryable for the lifetime of its Workspace. It retains
semantic facts rather than parser positions or source traversal state.

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

An unrecognized semantic name still resolves to TTX `Invalid`. Resource and
Error therefore distinguish successful data, recognized failure, and ordinary
absence without introducing a universal error enum.

## Failure reporting

The Cursor owns all textual TTX contents and the ordered reports produced while
that source is parsed, linked, and finalized. The outer Monograph and its fixed
child layers receive that operation-local Cursor explicitly, so lexical and
semantic failures point into the authored text without a retained Language
Diagnostic collection. A Monograph never keeps a Cursor after the operation.

Puffer, an editor, or another source-evaluation caller presents the textual
reports written through that Cursor directly to the end user. Binary Archive
validation and other source-free system or toolchain failures use Perimortem
Diagnostics, whose severity and persistence policy belongs to the host. They do
not invent an authored Token or a second Tetrodotoxin diagnostic model.

## Semantic lifecycle

One source participates in three stages:

1. The selected Dialect constructs one optional parse-valid Monograph in the
   source transaction Arena.
2. Linking resolves every route available to that source and reports failures
   to its Cursor.
3. Finalization performs language work that depends on linked declarations.

Workspace performs all three stages in one direct-source call and publishes
only the completed Monograph. Package is the sole multi-source model: it may
interpret all declared members first, then links every member before finalizing
any of them, and publishes only its completed root. A concrete Monograph may
organize its own internal dependencies while presenting the same link and
finalize boundary to its transaction owner.

## Persistence

A language that supports Archives defines the data needed to rebuild one of its
Monographs without the original source. Languages that are always read from
source do not need an Archive format. Package stores each language's data under
the corresponding member and leaves its contents to that language.

Persistent payloads have two profiles:

- `Complete` keeps the public and private observations promised by the
  persistent Dialect.
- `Interface` keeps only the public observations required by dependent
  consumers.

Neither profile implies executable bodies. Each Dialect retains the smallest
closed set of facts that can reconstruct an equivalent graph for its promised
queries. Native objects, SPIR-V, and other compiled implementations remain
separate Terminal products.

The selected profile also applies to child layers. The outer language stores a
separate section for each child, but only the child's language reads and checks
that section. Neither profile stores parser state, temporary caches, generated
IR, live runtime handles, or process addresses. Debug symbols and source mapping
belong to a separate output.

Archive reconstruction creates a fresh graph with equivalent observable
semantic facts and identity relations. It applies the same link and finalize
lifecycle as authored source. Package remains independent of the payload schema.

The payload is part of a Terminal product and carries reconstruction facts
rather than live graph identities. Equivalence means that a fresh Workspace
exposes the same observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and language facts. The
internal graph shape and process addresses may differ.

A payload may be much smaller than a memory image because it records only the
owner facts needed for those observations. Compactness is a format benefit. It
does not define whether a Dialect is persistent.

When reconstructing a Package, Workspace creates its description Monograph
before its members. Every member receives that same Package context, so mappings
and resources work the same way they do for authored source. Scene and Shader
pass the context to their child layers. Language dependencies still come from
the Workspace, not from the Package. If a child rejects its data, the outer
Monograph also fails.

The Dialect validates its complete bounded payload before returning an optional
Monograph reference from its reconstruction Arena. Workspace holds those Arena
handles, links every member, and then finalizes every member before it publishes
the completed root.
A target
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
source-backed Documentation directly to the selected Dialect, and the resulting
Monograph retains it.

Concrete body grammar starts immediately afterward. A grammar rule belongs to
the shared Language layer only when multiple concrete Dialects use its source
shape and its returned semantic contract.

See [Environment](../environment/README.md) for Workspace lifetime and
[TTX semantics](../../ttx/ttx_semantics.md) for the Abstract query model.
