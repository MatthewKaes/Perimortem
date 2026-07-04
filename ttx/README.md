# TTX

TTX is a small source IR format. It provides a human-authored source surface,
token bytecode, and a core Type and Layout model that a host can execute, inspect,
or use as an interchange boundary. Tetrodotoxin is the reference host in this
repository: it provides the VM, CLI, LSP, Bazel integration, source graph,
packaging, ISA dispatch, and backend entry points.

The design goal is simplicity: lower source text into a compact token stream,
let a host execute that stream with the active instruction set, and publish
queryable TTX facts as more context becomes available. Most of those facts
reduce to types and layouts.

## Pipeline

TTX source moves through a short chain of host-selectable layers:

```text
TTX source IR
-> lexical token bytecode
-> optional envelope evaluation
-> optional import or module loading
-> optional ISA evaluation
-> lowering, tooling, or interchange output
```

Each layer adds context to the same source-shaped program. The early layers are
cheap enough for editor features. Later layers can add source graph, ISA,
package, foreign-boundary, and backend facts when those facts are available.

[`lexical`](lexical/) lowers source bytes into TTX token bytecode. Fixed
spellings such as `.`, `.[`, `:[`, `::`, `->`, `[`, and `;` become stable token
classes here.

Tetrodotoxin begins full-source execution by calling the `Boot` ISA directly.
Boot understands Tetrodotoxin's source envelope: documentation, the
`dialect : Name;` instruction, and imports. The dialect instruction selects the
next body ISA from Tetrodotoxin's active `Isa::Registry`; it is not an
out-of-band parser option. Boot is called directly for complete source files,
rather than installed as a body ISA.

The Tetrodotoxin resolver loads the import closure, resolves package names such
as `Perimortem::Graphics` to manifests, checks that imported files declare the
requested ISA, and binds each import to the local name written in the source
file. It also owns the cache rules that keep source records valid when a
dependency changes.

The declared ISA then evaluates the remaining token bytecode with those imports
available. A Package ISA can publish package exports. A Library ISA can publish
types, values, and callable facts. Shader and Render ISAs can add their own
legality and lowering facts. Another host could choose a different envelope or
skip the envelope entirely when the evaluator is already known.

Lowering belongs to the ISA or backend that owns the requested output.
There is no extra authority between resolved TTX and the thing being emitted.

## Reference Source Envelope

Tetrodotoxin source files start in the Boot ISA. The order is fixed for that
host:

```text
optional documentation comment
required ISA selection instruction
zero or more imports
ISA-owned body
```

In source form:

```ttx
// Optional package docs.
dialect : Library;

import Graphics : Package = Perimortem::Graphics;

private Default2D : alias = Graphics::Shaders::Default2D;

// The rest belongs to the Library ISA.
```

The dialect instruction does not select a closed enum. It is an instruction in
the token stream that names the ISA that should evaluate the body. Boot records
that name and the imports. Tetrodotoxin package loading then loads the required
source files and asks the toolchain's `Isa::Registry` for the body evaluator
once the local import environment is complete.

The source execution is intentionally split:

```text
Boot ISA: execute header + imports
Resolver: load files + bind import aliases
Body ISA: evaluate remaining bytecode with resolved imports
```

That keeps filesystem and package graph management out of TTX while still
letting Tetrodotoxin ISAs evaluate with the type and package names requested by
the source.

## ISAs

An ISA is an installed semantic instruction set with a name and behavior. Its
evaluator address is its identity inside the active toolchain configuration.

That matters because ISAs are open. Adding `Shader`, `Render`, `Entity`, or a
project-specific authoring space means installing an ISA evaluator into the
toolchain, not editing a package-kind enum in multiple places.

Boot's job stays small. It reads the ISA name, validates that the active
toolchain installed an evaluator for that name, records the requested imports,
and leaves the remaining bytecode for that evaluator once resolution has bound
the local import names.

The ISA then owns its own instruction set, exported facts, and lowering path. A
Shader ISA can expose shader stage facts. A Package ISA can expose package
exports. A Library ISA can expose callable functions and ABI facts. Those are
ISA-owned enrichments over the same TTX token bytecode.

## Types And Layouts

A layout is shape:

- field order
- optional field names
- field types
- pack fitting rules

A type is identity plus behavior:

- name
- documentation
- parent or package ownership
- types
- functions
- structural members and defaults

This is the center of TTX. Values, packs, function arguments, function returns,
struct fields, shader resources, ABI carriers, and package exports are modeled
through type and layout queries. Documentation is carried by the
type/member/function that owns it, but it does not participate in type identity
or layout fitting.

Documentation is not canonicalized with aliases. An alias can carry its own
documentation while `canonical()` still reaches the root type and its
documentation. Tools can therefore present the alias name with alias
documentation, the alias name with canonical documentation, or a stacked view
that amends the root documentation with each alias layer in the current context.

Type parameterization is type dispatch over a resolved layout. A source spelling
such as `View[Bits_8]` first resolves `View`, then resolves `[Bits_8]` as the
parameter layout, then asks the `View` type object to produce the concrete type
identity for that layout. `Vec[Real_32, 4]`, `List[Sprite]`, and similar forms
follow the same rule. They are not templates or generated type families; the
parameterized type object answers with a concrete address that later type and
layout queries can use.

`Layout(type)` converts a type into the member view it exposes. A layout stores
only a `Type::Member` view, so the same object can describe aggregate members,
function parameters, return values, swizzles, slices, and other authored packs.
It does not own or upgrade itself into a type.

Names in a layout are authored or boundary-provided facts. An explicit named
pack authors names. A function parameter layout, function return layout, type
layout, or other receiving boundary can provide names for fitting and later
access. Repack operations such as grouping, swizzle, and slice produce
positional layouts unless the operator explicitly authors or preserves names.
This keeps temporary expression shape from accidentally inheriting names through
composition. Duplicate names are still representable because layouts can be
merged or generated by ISAs. Source and package diagnostics can report that
only the leftmost duplicate is reachable by name.

Layouts do not own storage facts in the root model. Size, alignment, and offsets
are produced later by the lowering owner that knows the target ABI or backend.
Primitive types therefore keep type identity without pretending to expose byte
storage at this level. Aggregate types expose structural members. Layout fitting
uses `source.fits(target)`; exact structural equality uses `equivalent_to`.
Defaulted members are only valid as a trailing suffix. A target layout with a
defaulted member followed by a required member does not fit, even when the
source provides that later required member.

Core scalar, vector, and memory types are prelude types. They are injected into
every source context as top-level names such as `Void`, `Real_32`, and `Vec2D`;
they are not accessed through `Core::Void` or imported as `TTX::Core`.

## Operators

TTX keeps three access modes separate.

`.` is named layout access. It works on layouts, and on typed values or types
after converting the type with `Layout(type)`.

```ttx
sprite.size_pixels.width
uniform.image
```

Layouts allow transformations called repacks. A repack produces a new fluid
layout. Its result is positional unless the syntax explicitly authors names or
the receiving boundary later supplies them. `.[` swizzle references member names
from the receiver layout while selecting values, then produces a positional
pack. `:[` index or slice evaluates expressions and selects by position.

```
// swizzle to repack
color.[r, g, b] // selects r g b from color and produces a positional pack.
color.[r, r, r] // repeats are allowed and still produce positional entries.
color.[r] // swizzles may be any size.

// indexes and slices evaluate their arguments.
color:[1] // one member at index 1.
color:[start] // one member at the evaluated start index.

// slices get consecutive elements based on order.
// This call gets two consecutive members starting at index 1 which in this case is the
// same as `color.[g, b]`.
color:[1, 2]
```

The split is intentional. `color.[r, g]` cannot read local variables named `r`
and `g`; it names fields in `color`'s layout. `color:[r, g]` evaluates local
state and uses the result as index/slice arguments.

Bare `[...]` is reserved only for layouts. Function parameters and return values
are layouts. They may be named, but those names belong to the declared boundary,
not to arbitrary expressions that later fit that boundary. Type arguments are
types with layout parameterization, so they still use the same layout syntax
rather than a second bracket meaning. Value indexing uses `:[...]`.

`(...)` allows for a "regrouping" of a layout:

```
(color.[r, g], color.b, alpha)
```

`::` is type or package access. It walks metadata owned by a package or type.
It does not degrade to layout.

```ttx
Perimortem::Graphics
Graphics::Sprite
Graphics::Shaders::Default2D
Render2D::Renderer2D
```

`->` is callable dispatch. It requires a dispatchable identity: a typed value,
a type object, a package object, or eventually a function pointer. It does not
work on a pure layout.

```ttx
Count -> from(value)
source -> get_size()
uniform.image -> sample(texture_uv)
```

This is invalid:

```ttx
(.x = 2, .y = 3) -> format()
```

The pack has shape, but no type identity and no function table. It may fit into
a target type later, such as assignment to `Vec2D` or passing into a parameter
with a concrete expected type. Until that context exists, there is nothing to
dispatch.

## Packages

Package imports resolve to package identities and exported objects in hosts that
provide a package layer.

```ttx
import Graphics : Package = Perimortem::Graphics;
private Default2D : alias = Graphics::Shaders::Default2D;
```

In Tetrodotoxin, the package name `Perimortem::Graphics` resolves to a package
manifest such as:

```text
perimortem/graphics/package.ttx
```

That manifest can import concrete source files and expose public aliases or
groups:

```ttx
dialect : Package;

@package_name = Perimortem::Graphics;

import Color : Library = "color.ttx";
import Renderer2D : Render = "renderer2d.ttx";
import Default2D : Shader = "shaders/default2d.ttx";

expose Sprite : alias = Sprite::Sprite;
expose Shaders : group {
  expose Default2D : alias = Default2D;
}
```

The package file is not a second language. It is TTX token bytecode evaluated by
a Package ISA. Package declares its package identity and describes package
exports through the same type and layout model.

A host owns the walk from package name to package manifest. Tetrodotoxin tools
create a root resolver from their active toolchain. Each package can own a local
resolver for its private files, so package internals such as
`shaders/default2d.ttx` are not part of the public Tetrodotoxin resolver API.
External sources import `Perimortem::Graphics`, then resolve
`Graphics::Shaders::Default2D` through the package's exports.

TTX owns what the resolved package, type, and layout facts mean once a host
hands them back under names like `Graphics`, `Types`, or `Render2D`.

## Errors

Failures are reported where the owning query has enough information to answer.

An empty lookup becomes a missing-object diagnostic.

A pack that cannot fit a target layout becomes a layout-mismatch diagnostic.

A call receiver with no type or dispatchable identity becomes a call-dispatch
diagnostic.

An imported file with a different ISA than the import requested becomes an
ISA-mismatch diagnostic at the import.

The model stays small because each concept reports its own failures. There is no
need for a separate layer whose job is to rediscover what packages, ISAs,
types, layouts, eventual ABI providers, or backends already know.

## Repository Map

The TTX directory is the language core:

- [`lexical`](lexical/) lowers source text into stable token bytecode
- [`documentation.hpp`](documentation.hpp) models source-authored
  documentation attached to language objects
- [`type.hpp`](type.hpp) and [`type.cpp`](type.cpp) model type identity,
  aliases, members, nested types, functions, and type-owned documentation
- [`layout.hpp`](layout.hpp) and [`layout.cpp`](layout.cpp) model shape,
  fitting, exact equivalence, named member access, and type-to-layout views
- [`core/prelude.hpp`](core/prelude.hpp) provides the top-level prelude types
  available to ordinary source contexts

Tetrodotoxin is the surrounding toolchain:

- [`../tetrodotoxin/cli`](../tetrodotoxin/cli/) is the command-line surface
- [`../tetrodotoxin/lsp`](../tetrodotoxin/lsp/) serves editor features
- [`../tetrodotoxin/isa`](../tetrodotoxin/isa/) owns the VM instruction sets
  such as Boot, Package, Library, Shader, and Render
- [`../tetrodotoxin/resolution`](../tetrodotoxin/resolution/) owns source
  loading, package loading, import binding, the source cache, and cache validity
- [`../perimortem/graphics/package.ttx`](../perimortem/graphics/package.ttx)
  describes the Perimortem graphics ABI as a TTX package
- [`../tetrodotoxin/ttx.bzl`](../tetrodotoxin/ttx.bzl) integrates TTX with
  Bazel
- [`../tetrodotoxin/compiler/assembler`](../tetrodotoxin/compiler/assembler/)
  emits terminal instruction streams such as SPIR-V and x86-64
- [`../tetrodotoxin/linker`](../tetrodotoxin/linker/) packages terminal object
  records and link targets

The split keeps TTX focused on source IR, token bytecode, and the Type and Layout
model while Tetrodotoxin supplies VM execution, files, packages, editor
integration, build integration, and terminal artifacts.
