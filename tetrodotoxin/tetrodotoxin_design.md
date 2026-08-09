# Tetrodotoxin Design

Tetrodotoxin turns TTX source into concrete language objects. Its architecture
keeps four kinds of fact separate:

- TTX owns shared lexical and semantic contracts.
- A Dialect owns the meaning of one source body.
- Environment owns the Workspace that retains and connects those bodies.
- Package, compiler, linker, and runtime owners handle their own external
  products and policies.

## Dialects and Monographs

A source begins with required Documentation and one Dialect declaration:

```ttx
// Reusable image helpers.
dialect : Library;
```

Environment selects the installed Dialect named by that declaration. The
Dialect interprets the remaining body and returns one Monograph: the durable
semantic result of that particular source invocation.

An explicit empty comment represents intentionally empty source Documentation;
omitting the opening comment is a malformed source envelope. Environment passes
the exact Documentation to the selected Dialect and its Monograph.

A Monograph is an Abstract context, not a universal Type or scope. A concrete
Dialect may expose no Types, one Type, several Types, Callables, package members,
or another contextual shape. Its `resolve_context()` behavior is part of that
Dialect's language contract.

This lets App expose startup and lifecycle facts without inheriting Library's
type system, while Library can expose its synthetic source context and Package
can expose its member bindings through the same TTX query boundary.

## Workspace

A Workspace is one semantic island. It installs the concrete Dialects available
to a tool, retains every resulting Monograph, and gives them a common lifetime
for borrowed TTX edges.

Source construction has three semantic stages:

1. Interpretation reserves source-shaped identities and records unresolved
   authored routes.
2. Linking connects those routes after the complete source group is known.
3. Finalization performs language work that requires linked declarations.

Every Monograph in a retained group links before any Monograph finalizes. The
group becomes publicly queryable as completed input only after both barriers
succeed.

Diagnostics remain attached to the language fact that discovered them.
Environment combines those facts with the retained source origin when it
presents an authored error.

## Contexts and access

Tetrodotoxin uses TTX contextual resolution rather than one universal member
model. Syntax selects the requested category:

| Syntax | Semantic result |
| --- | --- |
| `value.name` | one Addressable selected from an applicable named Layout |
| `context::Name` | one Type-shaped route traversed through contextual resolution |
| `receiver -> name(arguments)` | one Callable invocation |

An intermediate `::` context may be an Alias, Package, Monograph, source root,
Type, or another Abstract. The consuming grammar proves the terminal category
it requires; only a Type position requires Type. This same rule carries package
names, `using` routes, and nested Types without a Package-shaped Type or a
Type-shaped Monograph.

A Structure can expose all three categories, but each remains independent. A
Field, Callable, and nested Type may share one spelling because the authored
operator already identifies the query domain.

Address access identifies one semantic Addressable and its Type. It does not
decide whether the terminal uses a stack slot, an inline base plus offset, an
Object reference plus offset, or a folded value.

A Function host grants access authority but does not supply a receiver.
Functions select Fields through an explicit value and `.`, including private
Fields admitted by that host. A Static Function therefore cannot resolve a host
Field as a bare identifier.

## Package language

A Package source binds two kinds of authored facts:

```ttx
resolve Graphics : Perimortem.Graphics = "1.0";
source Scenes::Splash from "scenes/splash.ttx";
```

`resolve` gives an external Package identity a local Alias. `source` gives one
confined input a semantic route. Package paths locate bytes; semantic routes
identify Monographs and are traversed through TypeAccess syntax.

An embedded resource operand asks the exact source Package for bytes:

```ttx
$[resources/logo.png]
```

Package owns confinement and stable resource identity. Library may interpret
the bytes as a Constant, Shader may interpret them as shader data, and another
Dialect may define another meaning.

Package Archive is the durable semantic product. It contains Package identity,
version, dependencies, named members, Dialect payloads, and native artifact
locators. Each Dialect encodes and restores its own payload, so Package does not
learn Library, App, Scene, Render, or Shader semantics.

## Library language

Library supplies the CPU-oriented language model shared by reusable libraries
and executable Dialects. Its public concepts include concrete scalar Types,
Generic materialization, Constants, Expressions, Functions, Structs, Objects,
Enumerations, and Field access policy.

Each Library Monograph owns one synthetic source Structure with an empty
instance Layout. Its exact `source` route exposes that Structure, top-level
declarations enter its Static surface, and ordinary Monograph lookup forwards
only the external part of that surface. The Structure retains the source
Documentation, so a Package member Alias can reach one documented root Type
without owning a Package-shaped Type or copying the prose.

An authored Struct is an inline value Type with a named Layout of real Field
Addressables. An Object uses the same Structure model while adding nonnull
managed reference identity and language lifetime semantics.

Field visibility and writability are independent. Visibility determines who can
select a Field. Writability determines whether mutation is available generally,
only to code hosted by the Structure, or only during initialization.

Functions distinguish two invocation roles:

- Static has no implicit Self value and is invoked through a Type or source
  context.
- Self has the reserved `self` Addressable as parameter entry zero. Its Type is
  the selected receiver's exact Type.

Both are reached through `->`; the parameter Layout itself carries the role,
and neither Callable enters a value Layout.

Library uses TTX Layouts for parameters, results, named value packs, fields,
indexed value access, and swizzles. A Layout never becomes an anonymous Type
merely because a source expression produces several values.

Library keeps indexed reference and value selection separate. An
`Access[T]` value uses `access[index]` to try to produce an optional reference.
`value:[index]` returns an element by value and `value:[start, count]` returns a
read-only ranged value. Colon bracket selection is safe: an unavailable
element or range produces its result Type's default rather than a bounds
failure.

## Application Dialects

App owns startup profiles and application lifecycle. Program lifecycle selects
one Static Callable for generated entry. Scene lifecycle owns the live Scene
stack and transitions between Scene identities.

Scene owns its state, signals, declared child identities, render submission
facts, and `prepare`, `pause`, `resume`, `update`, and `release` roles. App owns
the transition that follows a Scene signal.

Render declares render-facing data and Stage interfaces. Shader implements one
Render contract and owns GPU Stage bodies and lowering facts. Foreign embeds an
external ABI surface inside a CPU-capable parent rather than creating a top
level source island.

## Compilation and runtime

Semantic Layouts describe language shape. A compiler derives target sizes,
alignments, offsets, pointer forms, calling convention carriers, registers, and
relocations from completed facts.

Library lowering consumes completed CPU facts owned by Library, App, or Scene
without converting those Monographs into Library source. Shader lowering
consumes Shader facts independently. Linker owns object modules, symbols,
relocations, target encoding, and final native products.

Object lifetime is a Library semantic promise; allocation strategy, collector
policy, pointer representation, and reclamation timing are runtime choices.
Likewise, a Package Archive contains durable semantics and native locators but
does not contain live semantic identities or runtime addresses.

## Documentation map

- [TTX semantics](../ttx/ttx_semantics.md)
- [Language extension model](language/README.md)
- [Environment and Workspace](environment/README.md)
- [Package language](package/README.md)
- [Library language](library/README.md)
- [App](app/README.md), [Scene](scene/README.md),
  [Render](render/README.md), [Shader](shader/README.md), and
  [Foreign](foreign/README.md)
