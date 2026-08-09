# Tetrodotoxin Language

`Tetrodotoxin::Language` is the extension boundary shared by concrete source
Dialects. It defines how a Dialect receives a source body, how its result remains
queryable, and how diagnostics and durable payloads cross language boundaries.

It does not define one common source body or type system. Package, Library, App,
Scene, Render, and Shader each own the grammar and semantic rules they add.

## Dialect

A Dialect interprets one kind of source body. Environment installs each concrete
Dialect under the exact name accepted by the source envelope:

```ttx
dialect : Library;
```

Environment consumes the envelope and gives the remaining Token stream to the
selected Dialect. The Dialect constructs one concrete Monograph and may retain
language state shared by other Monographs in the same Workspace.

The source-local context supplied during interpretation is an ordinary TTX
Abstract. A direct source may receive the Workspace; a Package member may
receive its Package Monograph. The concrete Dialect decides which contextual
queries that object supports.

## Monograph

A Monograph is the retained result of one Dialect invocation. It provides:

- stable Abstract identity;
- opening Documentation;
- contextual resolution defined by its concrete Dialect;
- ordered diagnostics;
- link and finalize lifecycle hooks.

A Monograph is not required to be a Type. It may expose no Types, one global
Type, several independent Types, package members, entry policy, or another
semantic context.

The Monograph remains queryable for the lifetime of its Workspace. It retains
semantic facts rather than parser positions or source traversal state.

## Contextual values

Two cross-Dialect Abstract contracts let a semantic context answer requests
without sharing its private policy.

### Resource

`Language::Resource` exposes stable retained bytes acquired by another owner.
It does not assign those bytes a Type or interpretation. Empty bytes are a
successful Resource.

For example, Package can resolve `$[resources/icon.png]` to a Resource while
Library constructs a Bytes Constant and Shader constructs a shader-specific
fact from the same result.

### Error

`Language::Error` represents a contextual request that was recognized but
failed in the receiving domain. The concrete owner retains the cause; the
source consumer supplies the authored location and presentation.

An unrecognized semantic name still resolves to TTX `Invalid`. Resource and
Error therefore distinguish successful data, recognized failure, and ordinary
absence without introducing a universal error enum.

## Diagnostics

`Language::Diagnostic` is a source-independent failure fact retained by a
Monograph. An authored diagnostic can name an exact TTX Anchor. A restored or
synthetic diagnostic can omit source coordinates while retaining its message
and hint.

Environment combines the Diagnostic with the source origin it retained for the
Monograph. Binary Archive validation and other context-free operations report
their own domain details rather than inventing authored Tokens.

## Semantic lifecycle

Concrete languages participate in three stages:

1. Interpretation creates stable source-shaped identities and records authored
   routes that may not resolve yet.
2. Linking connects those routes after the complete source group is available.
3. Finalization performs language work that depends on every linked declaration.

Environment links every Monograph in a retained group before finalizing any of
them. A concrete Monograph may organize its own internal dependencies while
presenting the same link and finalize boundary to Environment.

## Persistence

A concrete Dialect owns the payload required to recreate its source-free
Monograph. Package Archive frames the payload and records the Dialect name;
Language provides the dispatch between that name and the installed Dialect.

Restoration creates the same public semantic identities and applies the same
link and finalize lifecycle as authored source. Package remains independent of
the payload schema.

## Shared source envelope

The shared envelope contains required opening Documentation and one Dialect
declaration:

```ttx
// Package source documentation.
dialect : Package;
```

An explicit empty comment represents intentionally empty Documentation;
absence is a malformed source envelope. Environment passes the exact
Documentation to the selected Dialect, and the resulting Monograph retains it.

Concrete body grammar starts immediately afterward. A parser fragment belongs
to the shared Language layer only when several real Dialects use both its source
shape and its returned semantic contract.

See [Environment](../environment/README.md) for Workspace lifetime and
[TTX semantics](../../ttx/ttx_semantics.md) for the Abstract query model.
