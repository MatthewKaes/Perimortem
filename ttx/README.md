# TTX

TTX is a small source IR format. It provides a human-authored source surface,
token bytecode, and a queryable Abstract object model. A host can execute that model,
inspect it, or use it as an interchange boundary. Tetrodotoxin is the reference
host in this repository. It provides the VM, CLI, LSP, Bazel integration, source
graph, packaging, ISA dispatch, and backend entry points.

The design goal is simplicity: lower source text into a compact token stream,
let a host execute that stream with the active instruction set, and expose
queryable TTX facts as more context becomes available. Abstract identities,
types, callables, layouts, and ISA-specific contracts remain separate facts in
one graph.

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

Puffer begins full-source execution by calling its `Boot` ISA directly. Boot
understands Puffer's source preamble: documentation, the `dialect : Name;`
instruction, and imports. The dialect instruction selects the next body ISA from
Tetrodotoxin's active `Isa::Registry`. It is part of the token stream, not an
out-of-band parser option. Puffer calls Boot directly for complete source files
instead of installing it as a body ISA.

The Puffer resolver loads the import closure, resolves package names such as
`Perimortem.Graphics` to manifests, checks that imported files declare the
requested ISA, and binds each import to the local name written in the source
file. It also owns the cache rules that keep source records valid when a
dependency changes.

The declared ISA then evaluates the remaining token bytecode with those imports
available. A Package ISA can export package objects. A Library ISA can expose
types, values, and callable facts. Shader and Render ISAs can add their own
legality and lowering facts. Another host could choose a different envelope or
skip the envelope entirely when the evaluator is already known.

The ISA or backend that owns an output also owns its lowering. Resolved TTX
facts flow directly to that owner.

## Reference source envelope

Puffer source files start in the Boot ISA. The order is fixed for that host:

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

import Graphics : Package = Perimortem.Graphics;

private Default2D : alias = Graphics::Shaders::Default2D;

// The rest belongs to the Library ISA.
```

The dialect instruction does not select a closed enum. It is an instruction in
the token stream that names the ISA that should evaluate the body. Boot records
that name and the imports. Puffer resolution then loads the required source
files and asks the toolchain's `Isa::Registry` for the body evaluator once the
local import environment is complete.

Puffer splits source execution across three owners:

```text
Puffer Boot ISA: execute preamble + imports
Resolver: load files + bind import aliases
Body ISA: evaluate remaining bytecode with resolved imports
```

Tetrodotoxin owns the filesystem and package graph. TTX remains focused on the
language model, while body ISAs receive the type and package names requested by
the source.

## ISAs

An ISA is an installed semantic instruction set with a name and behavior. Its
host-owned installation is local configuration. Durable semantic identity comes
from the named Abstract facts the ISA exposes, never from a process address.

That matters because ISAs are open. Adding `Shader`, `Render`, or a
project-specific authoring space means installing an ISA evaluator into the
toolchain, not editing a package-kind enum in multiple places.

Puffer Boot's job stays small. It reads the ISA name, validates that the active
toolchain installed an evaluator for that name, records the requested imports,
and leaves the remaining bytecode for that evaluator once resolution has bound
the local import names.

The ISA then owns its own instruction set, exported facts, and lowering path. A
Shader ISA can expose shader stage facts. A Package ISA can expose package
exports. A Library ISA can expose callable functions and ABI facts. Those are
ISA-owned enrichments over the same TTX token bytecode.

## Abstracts and the model

Every queryable semantic identity implements `Abstract`. Derived contracts expose
the operations that make the object useful:

```text
Ttx::Abstraction::Abstract
├── Ttx::Abstraction::Alias
├── Ttx::Abstraction::Invalid
├── Ttx::Model::Generic
├── Ttx::Model::Pack
│   ├── Ttx::Model::Packs::Positional
│   └── Ttx::Model::Packs::Named
├── Ttx::Model::Expression
│   ├── Ttx::Model::Projection
│   ├── Ttx::Model::Binding
│   └── Ttx::Model::Constant
│       └── Ttx::Model::Constants::{Unsigned, Signed, Real, Flag, Bytes}
├── Ttx::Model::Type
│   └── ISA-defined model types
├── Ttx::Model::Callable
│   ├── Ttx::Model::Static
│   └── Ttx::Model::Self
└── Ttx::Model::Addressable
```

The `Ttx::Abstraction` namespace owns the restricted resolution substrate. The
`Ttx::Model` namespace owns the shared semantic vocabulary built on that
substrate, including Type, Layout, Callable, metadata, and their supporting
value facts.

`Abstract` owns only local naming, identity resolution, and context resolution
over borrowed `View::Bytes`. `Alias` is the closed named redirect to another
Abstract. `Invalid` is the closed stateless absorbing failure. `Type` adds a
Structured Layout and Type-owned query surfaces. `Generic` is an instruction
that creates or finds a compiler-owned Type from accepted arguments. `Callable`
adds complete parameter and result Layouts plus
an Addressable query. Static calls have no receiver. Self calls include the
receiver as parameter zero. Addressable is a named semantic edge whose
resolution supplies the addressed Abstract.

Type is not the universal semantic base, and there is no vague `Typed` marker.
A consumer resolves Abstract identity, proves the contract it needs, and then
uses that narrower interface. Contexts own their lookup representation and may
interpret a route atomically, slice it, or redirect it unchanged. No central
class database, stored Route, or resolution-state object participates.

The same ordered query chain from the same Abstract is deterministic until the
DAG changes. Differently partitioned routes need not be equivalent. Public
names walk an explicitly selected named ownership chain reversibly and never
use a signature hash.

Failure produces `Invalid : Abstract`, not a null semantic pointer. Invalid is
stateless and absorbing. The source-owning query retains the failed route and
diagnostic cause. Empty scopes use empty views. Unresolved callable linkage uses
an explicit unresolved Addressable or Invalid.

A Layout is an ordered fitting contract over real Abstracts. Fluid represents
positional value flow, Named represents uniquely named value flow, and
Structured is a Type's stable sequence of actual Addressable objects. Layout
does not copy their names, Types, documentation, attributes, defaults, or target
storage into a Member record. Its contiguous storage uses non-null borrowed
Reference values rather than nullable semantic pointers.

Pack is the Abstract carrier for grouped value flow. It is not one Expression,
a Type, runtime storage, or an Addressable. Its Layout is the identity-free
fitting view over the carried Abstracts. Positional Packs expose Fluid and
borrow an entry view that the evaluator has already flattened. Named Packs
expose Named over actual named Abstracts and never flatten. An authored field
uses Binding when its name is a new edge to an underlying Expression. Both Pack
forms own their Layout object rather than inheriting a second public contract.

Expression is one evaluatable value whose identity remains distinct from its
result Type. It exposes the proven Type, its ordered input Layout, and whether
the value fits another Type. Projection is the Expression selecting one
Addressable through one receiver. Binding is the authored named edge to one
underlying Expression, not a symbol table or binding phase. Constant is an
immutable Expression with an empty input Layout. Unsigned, Signed, Real, Flag,
and Bytes are open Constant domains, not alternatives in one central tagged
expression. Constants compare by domain, resolved Type, and payload. Real NaNs
compare as one semantic value so equality remains suitable for caches. TTX has
no native String Constant.

Fluid and Named fitting normally compare resolved semantic identity. When the
source entry is an Expression they instead ask it to fit the target Type. This
permits proven constant narrowing without adding numeric rules to Layout.
Structured fitting continues to preserve actual Addressable identity.

`Type::get_layout()` returns a Structured Layout. A Terminal has an empty
Structured Layout plus direct size/alignment queries. Lowering proves Terminal
before inspecting Layout. Every non-Terminal Type recursively resolves its
Addressables, including a valid empty aggregate. A Generic must first produce a
resolved Type. An authored `@abi` number is not a substitute
for Terminal contract proof.

Source hosts may reserve nonmoving Type and Callable objects before every fact
is known. An incomplete Type or containing system resolves to Invalid. Layout
has no Incomplete state. The host may enrich that object, replace an enclosing
resolver, or build immutable snapshots according to its own cache model. Public
export is optional and does not determine whether a Type is semantically real.

`Terminal : Type` publishes direct byte size and alignment. `Unsigned`,
`Signed`, `Real`, and `Flag` provide the standard terminal domains, while an
active toolchain constructs and names only the widths it supports. Current
Perimortem `Bits_*` names implement `Unsigned`. They do not create a Bits
contract. Register and instruction width remain compiler decisions, so a
one-byte terminal may still use a wider carrier. Core TTX owns no prelude,
global width table, or concrete class for each spelling.

Documentation is separate from identity resolution. The declaration that
introduces an Alias may own contextual documentation, while the resolved target
keeps its own prose. Alias itself owns neither documentation nor a
`display_name` identity substitute.

Type parameterization proves that the resolved Abstract implements Generic,
resolves the arguments, and asks it for a concrete Type. `View[Bits_8]`,
`Vec[Real_32, 4]`, and `List[Sprite]` follow the same rule. They are not a
parallel template or generated-type system.

A source context registers named Generic formulas. Each formula owns its
accepted argument shape, materialization, and cache lookup. The evaluator gives
it an ordered Argument sequence whose Abstract entries have already resolved
through aliases and type-producing expressions. Bool and unsigned entries use
their tagged values. Constants remain value-bearing Abstract entries and compare
by domain, resolved Type, and payload. That sequence is the complete
formula-local cache key. Names, routes, parents, and hashes are not cache
identity. The context owns name resolution. The parser does not hard-code
`Vec`, `View`, or another formula name.

Names belong to the actual Abstracts in a Named or Structured Layout. Repack
operations such as grouping, swizzle, and constant slice produce Fluid layouts
unless the syntax explicitly authors named objects. Named fitting rejects empty
or duplicate names and matches names independently of target order.

Layout fitting is directional: `source.fits(target)`. Core fitting does not
manufacture omitted defaults. A language or ISA that supports omission resolves
defaults through the real Addressables and completes the source value flow
before fitting. `get_fitted()` exposes which original source Abstract supplies
each target slot. A failed fit, invalid index, or missing mapping returns
Invalid so the source owner can report the authored shape error.

The active toolchain may install scalar, vector, and memory Types as top-level
names such as `Void`, `Real_32`, and `Vec2D`. They are ordinary Abstracts in
that toolchain's resolution context, not injected members of a core prelude.

## Operators

TTX keeps three access modes separate.

`.` is named layout access. It works on layouts, and on typed values or Types
after querying the Type's Layout.

```ttx
sprite.size_pixels.width
uniform.image
```

Layouts allow transformations called repacks. A repack produces a new fluid
layout. Its result is positional unless the syntax explicitly authors names or
the receiving boundary later supplies them. `.[` swizzle references member names
from the receiver layout while selecting values, then produces a positional
pack. `:[` index or slice requires constant-evaluated Unsigned arguments and
selects by position.

```ttx
// swizzle to repack
color.[r, g, b] // Select r, g, and b into a positional pack.
color.[r, r, r] // Repeated members remain separate positional entries.
color.[r]       // Swizzles may contain one member.

// Indexes and slices require constant-evaluated arguments.
color:[1]           // Read the member at index 1.
color:[first_index] // Valid when first_index evaluates to a Constant.

// Slices read consecutive members. This selects the same members as
// color.[g, b], but it selects through constant positions.
color:[1, 2]
```

The two forms answer different questions. `color.[r, g]` names fields in
`color`'s layout and cannot read local variables named `r` and `g`.
`color:[r, g]` is valid only when both expressions evaluate to Unsigned
Constants. Dynamic slicing uses ordinary dispatch such as
`color -> slice(start, count)` and returns one View-like typed value rather than
a compile-time pack.

Bare `[...]` is reserved only for layouts. Function parameters and return values
are layouts. They may be named, but those names belong to the declared boundary,
not to arbitrary expressions that later fit that boundary. Type arguments are
types with layout parameterization, so they still use the same layout syntax
rather than a second bracket meaning. Value indexing uses `:[...]`.

`(...)` regroups a layout:

```
(color.[r, g], color.b, alpha)
```

`::` is Type access after an Abstract has been bound into the current scope. It
performs a context query and proves the resulting Type contract. It does not
degrade to Layout, and it is not string concatenation.

```ttx
Perimortem.Graphics
Graphics::Sprite
Graphics::Shaders::Default2D
Render2D::Renderer2D
```

`->` is Callable dispatch. A Type or package receiver resolves Static. An
addressable value resolves Self through its resolved Type. It requires a
dispatchable identity and does not work on a pure Layout.

```ttx
Count -> from(value)
source -> get_size()
uniform.image -> sample(texture_uv)
```

This is invalid:

```ttx
(.x = 2, .y = 3) -> format()
```

The pack has shape, but no Type identity and no Callable children. It may fit into
a target type later, such as assignment to `Vec2D` or passing into a parameter
with a concrete expected type. Until that context exists, there is nothing to
dispatch.

## Packages

Package imports resolve to package identities and exported objects in hosts that
provide a package layer.

```ttx
import Graphics : Package = Perimortem.Graphics;
private Default2D : alias = Graphics::Shaders::Default2D;
```

Package names are `Type("." Type)*` values. The authored name identifies both
the package and its module directory. Tetrodotoxin resolves
`Perimortem.Graphics` through its registered Puffer Buffer rather than guessing
where the source manifest lives:

```text
Perimortem.Graphics/binary_archive.puffer
```

The source manifest that produced the package buffer can import concrete source
files and expose public aliases or groups:

```ttx
dialect : Package;

import Color : Library = "color.ttx";
import Renderer2D : Render = "renderer2d.ttx";
import Default2D : Shader = "shaders/default2d.ttx";

expose Sprite : alias = Sprite::Sprite;
expose Shaders : group {
  expose Default2D : alias = Default2D;
}
```

The package file is not a second language. It is TTX token bytecode evaluated by
a Package ISA. Puffer provides the package identity through compiler
configuration, and the source describes package exports through the same type
and layout model.

A host owns the walk from package name to package artifact. Puffer creates a
root resolver from its active toolchain and registers dependency package
buffers before loading source. Each package compile owns the resolver for its
private source workspace, so package internals such as `shaders/default2d.ttx`
are not part of the public Puffer resolver API.
External sources import `Perimortem.Graphics`, then resolve
`Graphics::Shaders::Default2D` through the package's exports.

TTX owns what the resolved package, type, and layout facts mean once a host
hands them back under names like `Graphics`, `Types`, or `Render2D`.

## Errors

Failures are reported where the owning query has enough information to answer.

An empty lookup produces Invalid while the owning query retains the missing-name
diagnostic.

A pack that cannot fit a target layout becomes a layout-mismatch diagnostic.

A call receiver with no Type or dispatchable identity produces Invalid while
the call owner reports the dispatch diagnostic.

An imported file with a different ISA than the import requested becomes an
ISA-mismatch diagnostic at the import.

The model stays small because each owner reports its own failure and returns the
same absorbing Invalid. There is no need for a separate layer whose job is to
rediscover what packages, ISAs, Types, Layouts, ABI providers, or backends
already know.

## Repository map

The TTX directory is the language core:

- [`lexical`](lexical/) lowers source text into stable token bytecode
- [`model`](model/) owns the shared Type, Layout, Pack, Expression, Constant,
  Callable, Attribute, and Documentation vocabulary
- [`model/documentation.hpp`](model/documentation.hpp) models source-authored
  documentation owned beside semantic objects by declarations and contexts
- [`abstraction/abstract.hpp`](abstraction/abstract.hpp) is the root semantic
  query contract
- Alias and Invalid are closed Abstract concepts. Type, Generic, Pack,
  Expression, Constant, Projection, Binding, Callable, Static, Self,
  Addressable, and ISA-specific contracts extend the graph with narrow
  operations
- [`model/layout.hpp`](model/layout.hpp) defines the ordered fitting contract.
  [`model/layouts`](model/layouts/) contains Fluid, Named, and Structured
- [`model/pack.hpp`](model/pack.hpp) supplies grouped value identity.
  [`model/packs`](model/packs/) contains the zero-allocation Positional and
  Named carriers. [`model/projection.hpp`](model/projection.hpp) and
  [`model/binding.hpp`](model/binding.hpp) preserve selected and named value
  provenance without adding parser operations to the model
- [`model/type.hpp`](model/type.hpp) supplies the narrow target-independent Type
  contract. [`model/types`](model/types/) adds Terminal, Unsigned, Signed, Real,
  and Flag contracts. [`model/expression.hpp`](model/expression.hpp),
  [`model/constant.hpp`](model/constant.hpp), and
  [`model/constants`](model/constants/) supply value and constant-domain
  contracts. [`model/callable.hpp`](model/callable.hpp),
  [`model/static.hpp`](model/static.hpp), [`model/self.hpp`](model/self.hpp), and
  [`model/addressable.hpp`](model/addressable.hpp) supply invocation contracts
  without making Callable a subtype of Type
- active toolchain contexts provide the scalar, vector, and memory Types they
  support as ordinary resolvable Abstracts

Tetrodotoxin is the surrounding toolchain:

- [`../tetrodotoxin/puffer/main.cpp`](../tetrodotoxin/puffer/main.cpp) is the `puffer`
  command-line surface
- [`../tetrodotoxin/puffer/isa/boot`](../tetrodotoxin/puffer/isa/boot/) owns
  Puffer's source preamble ISA
- [`../tetrodotoxin/puffer/resolution`](../tetrodotoxin/puffer/resolution/) owns source
  loading, package loading, import binding, the source cache, and cache validity
- [`../tetrodotoxin/lsp`](../tetrodotoxin/lsp/) serves editor features
- [`../tetrodotoxin/isa`](../tetrodotoxin/isa/) owns the VM instruction sets
  such as Package, Library, Shader, and Render
- [`../tetrodotoxin/standard/perimortem/graphics/package.ttx`](../tetrodotoxin/standard/perimortem/graphics/package.ttx)
  describes the Perimortem graphics ABI as a TTX package
- [`../toolchain/tetrodotoxin.bzl`](../toolchain/tetrodotoxin.bzl) integrates
  TTX with Bazel
- [`../tetrodotoxin/compiler`](../tetrodotoxin/compiler/) owns the per-build
  Abstract DAG and memory boundary, terminal planning, target backends, and
  private terminal instruction encoders
- [`../tetrodotoxin/linker`](../tetrodotoxin/linker/) packages terminal object
  records and link targets

The split keeps TTX focused on source IR, token bytecode, and the Abstract query
model while Tetrodotoxin supplies VM execution, files, packages, editor
integration, build integration, and terminal artifacts.
