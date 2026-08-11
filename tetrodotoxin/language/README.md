# Tetrodotoxin Language

The Tetrodotoxin Language contract defines how a source language participates
in a Workspace. It is closest to a compiler frontend interface. A Dialect
receives the Tokens for one source body and returns a retained semantic root
that tools and other languages can query.

Package, Library, App, Scene, Render, and Shader use the same lifecycle without
translating their source into one common AST or type system. Each Dialect keeps
the grammar and semantics unique to its language while exposing shared TTX
identity, resolution, and Layout where another domain can use them.

Together those concrete objects form the Workspace's live multi domain semantic
IR. The shared part is the TTX contracts rather than a common node schema. A
generic tool can use those contracts while richer tooling continues through the
concrete Dialect.

## When to implement a Dialect

A Dialect is appropriate when a source body has its own grammar, semantic
invariants, and completion work. A spelling variation over an existing language
usually belongs in that language instead. A grammar rule can be shared by
several Dialects when the complete construct and returned contract are genuinely
the same.

Adding a Dialect means owning a real frontend contract. It defines
interpretation, contextual queries, Diagnostics, link and finalize behavior,
and any payload required for source independent restoration. Generic tools can
use the common TTX surface. Rich language tooling depends on the concrete
Dialect.

Every top level Dialect provided by this repository publishes a canonical G4
grammar reference for authored language shape and parse order. These references
describe valid input. The toolchain does not generate or run its parsers from
them. A custom Dialect owns its grammar but does not have to express it in G4.

The shared grammar defines `Definition` as greedily retained Documentation,
Attributes, modifiers, and a name followed by `:`. The next Token is its
qualifier and remains for the concrete language to dispatch. Definition is a
source value, not an Abstract, common AST node, declaration hierarchy, or
semantic category and is attached by the parent parser to the actual dispatched
parser type.

An Attribute is one ordered key with at most one scalar value. Every Definition
can retain arbitrary Attributes (any number with duplicates being valid). The
concrete consumer decides which keys it interprets, whether repeated keys are
meaningful, and which local combinations are invalid. The shared parser never
rejects an Attribute because of the qualifier that follows it and it does not
validate its contents if it has any.

## Dialect

A Dialect interprets one kind of source body. Environment installs each concrete
Dialect under the exact name accepted by the source envelope:

```ttx
// Reusable source.
dialect : Library;
```

Environment consumes the envelope and gives the remaining Token stream to the
selected Dialect. The Dialect constructs one concrete Monograph and may retain
language state shared by other Monographs in the same Workspace.

The context local to a source during interpretation is an ordinary TTX
Abstract. A direct source may receive the Workspace. A Package member may
receive its Package Monograph. The concrete Dialect decides which contextual
queries that object supports.

Package is part of the Tetrodotoxin toolchain without becoming an implicit
context for every source. A Workspace that interprets one standalone source may
omit the Package Dialect. Package participates when the request composes a
Package, acquires its resources, or restores an Archive.

## Contextual resolution

`resolve()` follows represented identity. `resolve_context(route)` asks the
receiving Abstract to interpret a route in its own domain. The consumer then
proves the category required by its grammar.

Three questions recur across the provided languages:

1. Address access selects an Addressable through an applicable Layout.
2. An owner directed contextual route reaches an identity, and the consuming
   position proves the category it requires.
3. Call access lets a concrete language select and invoke a Callable using its
   parameter and result Layouts.

These are shared question domains rather than one universal operator grammar.
Each Dialect decides which questions its source can ask and what additional
policy applies.

## Monograph

A Monograph is the retained result of one Dialect invocation. It provides:

* stable Abstract identity
* opening Documentation
* contextual resolution defined by its concrete Dialect
* ordered diagnostics
* link and finalize lifecycle hooks

A Monograph may expose no Types, one global Type, several independent Types,
package members, entry policy, or another semantic context. Its role is the
retained root of one source, not a promise that every language has the same
shape.

The Monograph remains queryable for the lifetime of its Workspace. It retains
semantic facts rather than parser positions or source traversal state.

The concrete Dialect constructs its Types, Addressables, Callables, lifecycle
facts, or package members directly. As an example, a semantic object should
retain the exact Definition that introduced it but it's up to the Dialect's
concrete parsers to define what are actual durable semantics. The concrete
Monograph exposes that result directly. Environment does not wrap those objects
in generic declaration identities or copy them into a shared member inventory.

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

## Diagnostics

`Language::Diagnostic` is a failure fact that does not depend on source. A
Monograph retains it. An authored diagnostic can name an exact TTX Anchor. A
restored or synthetic diagnostic can omit source coordinates while retaining
its message and hint.

Environment combines the Diagnostic with the source origin it retained for the
Monograph. Binary Archive validation and other operations that have no source
context report their own domain details rather than inventing authored Tokens.

## Semantic lifecycle

Concrete languages participate in three stages:

1. Interpretation creates stable identities derived from source and records
   authored routes that may not resolve yet.
2. Linking connects those routes after the complete source group is available.
3. Finalization performs language work that depends on every linked declaration.

Environment links every Monograph in a retained group before finalizing any of
them. A concrete Monograph may organize its own internal dependencies while
presenting the same link and finalize boundary to Environment.

## Persistence

A concrete Dialect owns the payload schema and reconstruction procedure needed
to construct a new Monograph without source. This is an optional capability. A
Dialect that is always interpreted from source does not need an Archive
payload. Package Archive frames a persistent Dialect's payload and records its
name. Environment selects the installed Dialect by that exact name and gives it
the exact member payload bytes.

Restoration constructs a fresh graph with equivalent observable semantic facts
and identity relations. It applies the same link and finalize lifecycle as
authored source. Package remains independent of the payload schema.

The payload is part of a Terminal product and carries reconstruction facts
rather than live graph identities. Equivalence means that a fresh Workspace
exposes the same observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and language facts. The
internal graph shape and process addresses may differ.

A payload may be much smaller than a memory image because it records only the
owner facts needed for those observations. Compactness is a format benefit. It
does not define whether a Dialect is persistent.

The Dialect validates its complete payload before returning a Monograph.
Environment then retains the reconstructed group and runs the same link and
finalize barriers used for authored source. A target representation such as
LLVM IR cannot substitute for this payload because it has already lost owner
facts that were meaningful in the source language.

## Shared source envelope

The shared envelope contains required opening Documentation and one Dialect
declaration:

```ttx
// Package source documentation.
dialect : Package;
```

An explicit empty comment represents intentionally empty Documentation.
Absence is a malformed source envelope. Environment passes the exact
Documentation to the selected Dialect, and the resulting Monograph retains it.

Concrete body grammar starts immediately afterward. A grammar rule belongs to
the shared Language layer only when multiple concrete Dialects use its source
shape and its returned semantic contract.

See [Environment](../environment/README.md) for Workspace lifetime and
[TTX semantics](../../ttx/ttx_semantics.md) for the Abstract query model.
