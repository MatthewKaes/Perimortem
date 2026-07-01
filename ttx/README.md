# TTX

TTX is the small source-shaped language at the center of the Tetrodotoxin
toolchain. Tetrodotoxin owns the CLI, LSP, Bazel integration, future
source/package graph management, packaging, and backend entry points. TTX owns
the language model those tools share.

The design goal is simplicity: parse the source once, keep the authored shape,
and answer richer questions from the same tree as more context becomes
available. Most of those questions reduce to types and layouts.

## Pipeline

TTX source moves through a short set of owners:

```text
source text -> lexical -> package envelope -> Tetrodotoxin package graph -> dialect body -> lowering
```

Each step adds facts. It should not clone the program into a second semantic
tree just to ask the same questions again.

`lexical` turns source bytes into token classes. Fixed spellings such as `.`,
`.[`, `:[`, `::`, `->`, `[`, and `;` live here.

`parse` owns the common TTX envelope and the small lingua franca pieces shared
by every dialect: documentation, identifiers, constants, type paths, imports,
and layouts. The first parse stage stops after the dialect header and imports.

Tetrodotoxin will own source-tree identity once the package graph layer is
rebuilt. That future layer loads the import closure, resolves package paths such
as `TTX::Graphics` to manifests, checks that imported files declare the
requested dialect, and binds each import to the local name written in the source
file. The old resolution code was removed so this layer can be rebuilt from the
current TTX source envelope and dialect model.

`dialect` owns the extension table. A source file declares a dialect in its
header, and that registered dialect parses the rest of the file after
Tetrodotoxin has supplied the resolved imports under their local aliases.

Lowering belongs to the dialect or backend that owns the requested output.
There is no extra source-of-truth layer between resolved TTX and the thing being
emitted.

## Source Envelope

Every source file starts in the TTX root parser. The order is fixed:

```text
optional documentation comment
required dialect header
zero or more imports
dialect-owned body
```

In source form:

```ttx
// Optional package docs.
dialect : Library;

import Graphics : Package = TTX::Graphics;
import Default2D : Shader = Graphics::Shaders::Default2D;

// The rest belongs to the Library dialect.
```

The dialect header does not select a closed enum. It names a dialect. The root
parser records that name and the imports. Tetrodotoxin package loading will
then load the required source files and ask the registered dialect table for the
body parser once the local import environment is complete.

The source pass is intentionally split:

```text
TTX parse: header + imports
Tetrodotoxin package graph: load files + bind import aliases
TTX dialect parse: body with resolved imports
```

That keeps filesystem and package graph management out of TTX while still
letting TTX dialects parse with the type/package names requested by the source.

## Dialects

A dialect is a registered object with a name and behavior. Its address is its
identity inside the active toolchain configuration.

That matters because dialects are open. Adding `Shader`, `Render`, `Entity`, or
a future project-specific dialect should mean registering a dialect object, not
editing a package-kind enum in multiple places.

The root TTX parser only needs to know:

```text
What dialect name did the source declare?
Which object registered that name?
Which imports were requested and what local names should they use?
Can that object parse the remaining body with those imports?
```

The dialect then owns its own parse shape, exported facts, and lowering path.
A Shader dialect can expose shader stage facts. A Package dialect can expose
package exports. A Library dialect can expose callable functions and ABI facts.
Those are dialect-owned enrichments over the same source.

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
struct fields, shader resources, ABI carriers, and package exports should all
be answerable through type and layout queries. Documentation is carried by the
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
merged or generated by dialects, but a source or package owner should diagnose
that only the leftmost duplicate can be reached by name.

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
rather than a second bracket meaning. Value indexing should use `:[...]`.

`(...)` allows for a "regrouping" of a layout:

```
(color.[r, g], color.b, alpha)
```

`::` is type or package access. It walks metadata owned by a package or type.
It does not degrade to layout.

```ttx
TTX::Graphics
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

Package imports resolve to package files and exported objects.

```ttx
import Graphics : Package = TTX::Graphics;
```

The package path `TTX::Graphics` should resolve to a package manifest such as:

```text
tetrodotoxin/packages/graphics/package.ttx
```

That manifest can import concrete source files and re-export their public
types, functions, resources, or nested packages:

```ttx
dialect : Package;

import Color : Library = (.source = "color.ttx");
import Renderer2D : Render = (.source = "renderer2d.ttx");
import Default2D : Shader = (.source = "shaders/default2d.ttx");

@public Sprite : alias = Sprite::Sprite;
@public Shaders : Package = Shaders;
```

The package file is not a second language. It is a dialect body whose job is to
describe package exports through the same type and layout model.

Resolution owns the source-tree walk needed to get there. TTX owns what the
resolved package, type, and layout facts mean once Tetrodotoxin hands them back
under names like `Graphics`, `Types`, or `Render2D`.

## Errors

Failures should happen where the owning query has enough information to answer.

If a lookup is empty, report the missing object.

If a pack cannot fit a target layout, report the layout mismatch.

If a call receiver has no type or dispatchable identity, report that the call
cannot dispatch.

If an imported file declares a different dialect than the import requested,
report the dialect mismatch at the import.

The model stays small because each concept reports its own failures. There is no
need for a separate layer whose job is to rediscover what packages, dialects,
types, layouts, eventual ABI providers, or backends already know.

## File Ownership

The TTX folder should read as the language core:

```text
ttx/lexical
ttx/parse
ttx/dialect
ttx/core
```

Tetrodotoxin should read as the toolchain wrapper around TTX:

```text
tetrodotoxin/cli
tetrodotoxin/lsp
tetrodotoxin/packages
tetrodotoxin/linker
tetrodotoxin/compiler/assembler
tetrodotoxin/ttx.bzl
```

Formatting is a user-facing product of a parsed dialect tree. It should come
back when there is a real tree to format, not as a token-level guess in the TTX
root.

The full compiler facade and ABI lowering should come back under Tetrodotoxin
after dialects can lower into enriched IR. The assembler, object, and linker
layers can stay because they are terminal generation pieces. Keeping the old
ABI-era compiler facade around before that point would make broken generated
symbols look like part of the current model.

The rule should stay the same: path, namespace, and concept ownership should
agree. TTX should not manage the source tree, and Tetrodotoxin should not
duplicate type/layout meaning that belongs to TTX.
