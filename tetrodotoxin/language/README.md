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
keep any number of Attributes, including repeated keys. The concrete language
decides which keys it understands, whether repetition is meaningful, and which
combinations are invalid. The shared parser only preserves the authored data.

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

`resolve()` follows represented identity. `resolve_context(route)` asks the
receiving Abstract to interpret a route in its own domain. The consumer then
proves the category required by its grammar.

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
- name resolution defined by its Dialect
- ordered diagnostics
- link and finalize lifecycle stages

A Monograph may expose no Types, one global Type, several independent Types,
package members, entry policy, or another semantic context. Its role is the
retained root of one source, not a promise that every language has the same
shape.

A Monograph may contain a small, fixed set of child layers built by its language
dependencies. A Scene contains one Library layer. A Shader contains one Library
layer and one Render layer. Tools can ask the outer Monograph for a layer by
using the same Dialect instance that the Workspace installed.

This lookup is intentionally narrow. It does not search by name, follow Aliases,
or create a wrapper around the child. A top-level Monograph answers with itself.
A Scene answers with its Library child. A Shader answers with its Library or
Render child. Any other request has no result.

The Package keeps only the outer Monograph as a member. That outer Monograph
keeps its children alive, moves them through linking and finalization, shows
their diagnostics with its own, and stores their Archive data. The children do
not become separate Package members or copied views of the same declarations.

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

## Diagnostics

Each source or restored Package member collects errors in one ordered list. The
outer Monograph and all of its child layers write to that same list while they
read, link, finalize, or restore their data. Related errors therefore appear
together, and a child cannot hide a separate list of failures.

The source reader still tracks the current Token and reports lexical errors at
that location. Semantic errors can point to a TTX Anchor when source text
exists. Errors from restored or generated data can omit a source location while
keeping a useful message and hint.

Environment combines the Diagnostic with the source origin it retained for the
Monograph. Binary Archive validation and other operations that have no source
context report their own domain details rather than inventing authored Tokens.
Perimortem's process-wide Diagnostics remain reserved for fatal host state, not
ordinary source or restoration failures.

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

A language that supports Archives defines the data needed to rebuild one of its
Monographs without the original source. Languages that are always read from
source do not need an Archive format. Package stores each language's data under
the corresponding member and leaves its contents to that language.

Persistent payloads have two profiles:

- `Complete` keeps public and private declarations, executable bodies,
  expressions, control flow, access edges, and the relationships needed to
  restore and compile the graph again.
- `Interface` retains public Types, Layouts, Fields, Callable signatures,
  folded public constants, ABI requests, publication relationships, bridge
  facts, and compiled artifact locations, but no executable bodies.

The selected profile also applies to child layers. The outer language stores a
separate section for each child, but only the child's language reads and checks
that section. Neither profile stores parser state, temporary caches, generated
IR, live runtime handles, or process addresses. Debug symbols and source mapping
belong to a separate output.

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

When restoring a Package, Environment creates its Package Monograph before it
restores the members. Every member receives that same Package context, so
imports and resources work the same way they do for authored source. Scene and
Shader pass the context to their child layers. Language dependencies still come
from the Workspace, not from the Package. If a child rejects its data, the
outer Monograph also fails.

The Dialect validates its complete bounded payload before returning a
Monograph. Environment then retains the reconstructed group and runs the same
link and finalize barriers used for authored source. A target representation
such as LLVM IR cannot substitute for this payload because it has already lost
owner facts that were meaningful in the source language.

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
