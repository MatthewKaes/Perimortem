# TTX

TTX is a small source IR format. It provides a human-authored source surface,
token bytecode, and a queryable Abstract object model. A host can execute that model,
inspect it, or use it as an interchange boundary. Tetrodotoxin is the reference
host in this repository. It provides Dialect models, the CLI, LSP, Bazel
integration, source graph, packaging, and backend entry points.

The design goal is simplicity: lower source text into a compact token stream,
let a host execute that stream with the active instruction set, and expose
queryable TTX facts as more context becomes available. Abstract identities,
types, callables, layouts, and Dialect-specific contracts remain separate facts
in one graph.

## Pipeline

TTX source moves through a short chain of host-selectable layers:

```text
TTX source IR
-> lexical token bytecode
-> optional envelope evaluation
-> optional Environment injection
-> optional Dialect evaluation
-> lowering, tooling, or interchange output
```

Each layer adds context to the same source-shaped program. The early layers are
cheap enough for editor features. Later layers can add source graph, Dialect,
package, foreign-boundary, and backend facts when those facts are available.

[`lexical`](lexical/) lowers source bytes into TTX token bytecode. Fixed
spellings such as `.`, `.[`, `:[`, `::`, `->`, `[`, and `;` receive semantic
Codes in the concrete Lexer contract.

Puffer begins full-source evaluation with its Boot envelope. Boot understands
documentation and the `dialect : Name;` instruction. The dialect
instruction resolves one `Tetrodotoxin::Model::Dialect` from the host's
`Dialects` Abstract context. It is part of the token stream, not an out-of-band
parser option or a lookup in a second registry.

The Puffer resolver selects an explicit package member set, resolves package
names such as `Perimortem.Graphics` to manifests, and injects all available
bindings into one Environment before evaluation. It also owns the cache rules
that keep source records valid when a dependency changes.

The declared Dialect then evaluates the remaining token bytecode with that
Environment available. Package can export package objects. Library can expose
types, values, and callable facts. Shader and Render can add their own legality
and lowering facts. Another host may choose a different envelope or skip it
when the Dialect is already known.

The Dialect or backend that owns an output also owns its lowering. Resolved TTX
facts flow directly to that owner.

## Reference source envelope

Puffer source files start in the Boot envelope. The order is fixed for that
host:

```text
optional documentation comment
required Dialect selection instruction
Dialect-owned body
```

In source form:

```ttx
// Optional package docs.
dialect : Library;

private Default2D : alias = Graphics::Shaders::Default2D;

// The rest belongs to the Library Dialect.
```

The dialect instruction does not select a closed enum. It names the Dialect
that should evaluate the body. Boot resolves that name through the host's
ordinary `Dialects` context. Puffer resolves the exact package Environment and
explicit Source membership before evaluating any body. Every member Source in
that package transaction borrows the same Environment.

Puffer splits source execution across three owners:

```text
Container: select Sources + construct Environment
Puffer Boot: execute each Source preamble
Interpreter: execute the selected Model::Dialect with Environment lookup
```

Tetrodotoxin owns the filesystem and package graph. TTX remains focused on the
language model, while the selected Dialect receives the Type and Package names
requested by the source.

## Dialects

A Dialect is a named model that evaluates one source region or continuation.
The host supplies available Dialects through an ordinary Abstract context.
Durable semantic identity comes from the Dialect and the real Abstract facts it
produces, never from a process address or registry record.

Adding `Shader`, `Render`, or a project-specific authoring space means exposing
another Dialect in that context, not editing a package-kind enum or installing
an evaluator callback in a second registry.

Puffer Boot's job stays small. It reads the Dialect name, resolves the real
model, and leaves the remaining bytecode for that model. Dependency and local
product bindings were already installed by the caller's Environment.

The Dialect owns its instruction set, exported facts, and any narrow lowering
contracts it contributes. Shader can expose stage facts, Package can expose
package exports, and Library can expose callable functions and ABI facts. No
separate evaluator or instruction-set object sits between a Dialect and those
real models.

## Concepts and the model

Every queryable semantic identity implements `Abstract`. Derived contracts expose
the operations that make the object useful. Layout and Documentation are equally
fundamental identity-free concepts. Durable owners answer name resolution from
their borrowed Environment and already-rooted definitions:

```text
Ttx::Concept
├── Abstract
│   ├── Invalid
│   ├── Ttx::Model::Alias
│   ├── Ttx::Model::Exports
│   ├── Ttx::Model::Generic
│   ├── Ttx::Model::Expression
│   │   ├── Ttx::Model::Projection
│   │   ├── Ttx::Model::Binding
│   │   └── Ttx::Model::Constant
│   │       └── Ttx::Model::Constants::{Unsigned, Signed, Real, Flag, Bytes}
│   ├── Ttx::Model::Type
│   │   ├── Ttx::Model::Types::Terminal
│   │   │   ├── Unsigned -> Unsigned_8 / 16 / 32 / 64
│   │   │   ├── Signed -> Signed_8 / 16 / 32 / 64
│   │   │   ├── Real -> Real_32 / 64 / 128
│   │   │   └── Flag -> Boolean
│   │   └── Dialect-defined model types
│   ├── Ttx::Model::Callable
│   │   ├── Ttx::Model::Callables::Static
│   │   └── Ttx::Model::Callables::Self
│   └── Ttx::Model::Addressable
│       └── Ttx::Model::Addressables::Writable
├── Documentation
└── Layout
```

The `Ttx::Concept` namespace owns the foundational contracts and values used
across TTX. Abstract supplies semantic identity, contextual lookup, and a stable
Documentation query. Layout supplies shape and fitting without identity.
Documentation preserves borrowed authored or generated prose. The
`Ttx::Model` namespace owns the semantic mechanisms built from those concepts,
including Alias, Type, Expression, Callable, documentation implementations,
and their concrete refinements.

`Abstract` owns local naming, identity resolution, context resolution over
borrowed `View::Bytes`, and documentation visible at that exact object. `Alias`
is the closed local name, documentation chain, and redirect to another Abstract.
`Invalid` is the closed stateless absorbing
failure. `Type` adds a Layout and Type-owned query surfaces.
`Generic` is an instruction that creates or finds a compiler-owned Type from
accepted arguments. `Exports` is the narrow durable contract for an ordered
public definition surface. Direct lookup is closed over that surface, so a name
resolves exactly when it belongs to one enumerated export. Private roots and
Environment bindings cannot leak through the boundary.
Package dependencies, restored packages, nested groups, reflection, and tools
can therefore consume one graph without requiring a
universal Group, Namespace, Source, module, or package model. A host implements
those concrete domains without making containment a Type. `Callable` adds
complete parameter and result Layouts. Static calls have no receiver. Self
calls include the receiver as parameter zero. Addressable is a named address to
typed data; `get_type()` supplies Type or Invalid without resolving away the
address identity. Writable is the narrower assignment capability and supplies
the stable non-writable projection used by `expose state`. Machine linkage
belongs to an ABI or execution contract.

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
one binary-wide stateless absorbing object. Semantic owners do not store an
Invalid reference or construct local failure sentinels. The source-owning query
retains the failed route and diagnostic cause. Empty groups use empty views.
Unresolved linkage is modeled by the ABI or execution contract that owns it.

A `Concept::Layout` is an ordered fitting contract over real Abstracts. Fluid
represents positional value flow, Named represents uniquely named value flow,
Structured is a Type's stable sequence of actual Addressable objects, Ranged
compactly repeats one Abstract over a fixed interval, and Composite joins two
complete Layouts without copying their entries. `Bytes[N]` can therefore answer
indexed Type queries without allocating N edges, then compose with another
Layout without becoming a flattened Fluid. Layout does not copy names, Types,
documentation, attributes, defaults, or target storage into a Member record.

Grouped value flow uses `Layouts::Fluid`, `Layouts::Named`, or
`Layouts::Composite` directly. Layout already owns the identity-free ordered
fitting contract, so no Abstract Pack identity or `get_layout()` wrapper sits
around it. An authored field uses Binding when its name is a new edge to an
underlying Expression.

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

`Type::get_layout()` returns the base Layout contract. A Terminal has an empty
Structured Layout plus direct value-width, size, and alignment queries. An
authored aggregate normally returns Structured; a fixed homogeneous Type can
return Ranged. Lowering proves Terminal before applying the appropriate
aggregate rule. A Generic must first produce a resolved Type. An authored
`@abi` number is not a substitute for Terminal contract proof.

Source hosts may reserve nonmoving Type and Callable objects before every fact
is known. An incomplete Type or containing system resolves to Invalid. Layout
has no Incomplete state. The host may enrich that object, replace an enclosing
context, or build immutable snapshots according to its own cache model. Public
export is optional and does not determine whether a Type is semantically real.

`Terminal : Type` defines the fixed name, documentation, value-width, byte-size,
and byte-alignment queries. `Unsigned`, `Signed`, `Real`, and `Flag` provide the
standard domains. Width-specific classes such as `Unsigned_8` and `Real_64`
return their literal names and rich documentation directly. An active toolchain
constructs and installs only the classes it supports. Perimortem and TTX use
the same `Unsigned_*`, `Signed_*`, and `Real_*` names. Register and instruction
width remain compiler decisions, so a one-byte terminal may still use a wider
carrier. Core TTX owns no prelude or global terminal catalogue.

Every Abstract returns a stable Documentation reference. Authored objects may
borrow source comments, implementation concepts may expose generated comments,
and objects without useful prose return the binary-wide empty Comment. Constant
deliberately terminates this query because its Addressable owner carries any
authored context. Alias instead presents its local lines followed by the
target's visible documentation. Alias chains therefore accumulate prose without
changing identity resolution. Documentation is never a `display_name` identity
substitute.

Type parameterization proves that the resolved Abstract implements Generic,
resolves the arguments, and asks it for a concrete Type. `View[Unsigned_8]`,
`Vec[Real_32, 4]`, and `List[Sprite]` follow the same rule. They are not a
parallel template or generated-type system.

A source context registers named Generic formulas. Each formula owns its
accepted argument shape, materialization, and cache lookup. The evaluator gives
it an ordered Layout of real compile-time Abstracts. Types are direct
first-class inputs; scalar inputs are real Constants rather than inline tags in
a second argument model. Generic compares resolved identities and Constant
value equality, making that Layout the complete formula-local cache key. Names,
routes, parents, and hashes are not cache identity. The context owns name
resolution. The parser does not hard-code `Vec`, `View`, or another formula
name.

Names belong to the actual Abstracts in a Named or Structured Layout. Repack
operations such as grouping, swizzle, and fixed slice produce positional
Layouts unless the syntax explicitly authors named objects. Composite preserves
complete positional child Layouts when they are joined. Named fitting rejects
empty or duplicate names and matches names independently of target order.

Layout fitting is directional: `source.fits(target)`. Core fitting does not
manufacture omitted defaults. A language or Dialect that supports omission resolves
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

Layouts allow transformations called repacks. A repack produces a positional
Layout unless the syntax explicitly authors names or the receiving boundary
later supplies them. Joining existing positional Layouts produces Composite so
compact Ranged and Fluid representations survive the operation. `.[` swizzle
references member names from the receiver Layout while selecting values. `:[`
indexes or selects a fixed width range by position. A slice count is always a
constant evaluated Unsigned value because it determines the resulting Layout.

```ttx
// swizzle to repack
color.[r, g, b] // Select r, g, and b into a Fluid Layout.
color.[r, r, r] // Repeated members remain separate positional entries.
color.[r]       // Swizzles may contain one member.

// A constant index can select from any known Layout.
color:[1]

// A dynamic index is valid for a uniform indexed value.
pixels:[pixel_index]

// Slices select a fixed number of consecutive members. The start may be
// dynamic for a uniform indexed value, but the count remains constant.
color:[1, 2]
bytes:[offset, 4]
```

The two forms answer different questions. `color.[r, g]` names fields in
`color`'s layout and cannot read local variables named `r` and `g`.
`bytes:[offset, 4]` evaluates `offset` at runtime but produces a four-entry
Fluid Layout because `Bytes` has a uniform element Type and `4` is constant.
Constant structured positions use ordinary Addressable Projections. Dynamic
uniform positions are indexed Expressions owned by the active Dialect: Ranged or
the receiver Type's indexed access contract proves the element Type without
inventing dynamic Addressables or runtime Layout members.
When the count is dynamic, ordinary dispatch such as
`bytes -> slice(offset, count)` returns one View-like typed value rather than a
fixed Fluid Layout.

A writable fixed slice is an explicit aggregate assignment target. The source
must fit the selected homogeneous range or positional Layout; this permits
compact fixed wire writes without making ordinary typed values splat:

```ttx
target:[8, 5] = 0x[08 06 00 00 00];
target:[offset, 4] = pixel.[red, green, blue, alpha];
```

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

`->` is Callable dispatch. A Type or Source context resolves Static. An
addressable value queries Self through its Type. It requires a
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

The grouped values have a Fluid Layout, but no Type identity and no Callable
children. They may fit into a target type later, such as assignment to `Vec2D`
or passing into a parameter with a concrete expected type. Until that context
exists, there is nothing to dispatch.

## Packages

Package resolutions bind exact package identities and exported objects in hosts
that provide a package layer.

```ttx
resolve Graphics : Perimortem.Graphics = "2.2";
private Default2D : alias = Graphics::Shaders::Default2D;
```

Package names use the token shape `Type("." Type)*`. That spelling is a host
package identity, not proof that the package model implements Type. The quoted
`"2.2"` spelling is parsed directly into independent Major and Minor unsigned
components and never becomes a Real value. `"0.1"` is valid while `"0.0"` is
the unset version. Canonical text rejects leading zeroes so every accepted
version has one round trip. The authored name identifies both the package and
its module directory.
Tetrodotoxin resolves
`Perimortem.Graphics` through its registered Puffer Buffer rather than guessing
where the source manifest lives:

```text
Perimortem.Graphics/binary_archive.puffer
```

The package container selects concrete source files and constructs their shared
Environment. A Package Source itself contains only its Dialect and body:

```ttx
dialect : Package;

public Sprite : alias = SpriteTypes::Sprite;
public Shaders : group {
  public Default2D : alias = Default2D;
}
```

The package file is not a second language. It is TTX token bytecode evaluated by
a Package Dialect. Puffer projects the package key through compiler
configuration and resolution; the Package Abstract remains anonymous. Source
owns its text, Tokenizer, arena, and formatter-capable rooted Abstract graph.
It owns no import or package dependency edges. The package container explicitly
passes its member Sources to the source-backed Package, and Package exports
retain their real contracts.

Consumers resolve the same Package graph whether it was interpreted from
Sources or reconstructed from a compiled Puffer Buffer. Source-dependent tools
first prove the optional `Packages::Interpreted` Abstract contract and then ask
for its Sources. A precompiled Package cannot prove that capability and does not
pretend it can reproduce the original source.

A host owns the walk from package name to package artifact. Puffer creates a
root resolver from its active toolchain and registers dependency package
buffers before loading source. Each package compile owns the resolver for its
private source workspace, so package internals such as `shaders/default2d.ttx`
are not part of the public Puffer resolver API.
External callers inject `Perimortem.Graphics` as `Graphics`, then Sources resolve
`Graphics::Shaders::Default2D` through the package's exports.

TTX owns what the resolved package, type, and layout facts mean once a host
hands them back under names like `Graphics`, `Types`, or `Render2D`.

## Errors

Failures are reported where the owning query has enough information to answer.

An empty lookup produces Invalid while the owning query retains the missing-name
diagnostic.

Grouped value flow that cannot fit a target Layout becomes a layout-mismatch
diagnostic.

A call receiver with no Type or dispatchable identity produces Invalid while
the call owner reports the dispatch diagnostic.

A Source whose selected Dialect is unavailable produces a diagnostic at its
container-owned source selection.

The model stays small because each owner reports its own failure and returns the
same absorbing Invalid. There is no need for a separate layer whose job is to
rediscover what Packages, Dialects, Types, Layouts, ABI providers, or backends
already know.

## Repository map

The TTX directory is the language core:

- [`lexical`](lexical/) owns the concrete Lexer Code stream and the Lexicon
  mapping fixed source spellings onto those Codes
- [`concept`](concept/) owns Abstract, Invalid, Reference, Documentation, and
  Layout as the foundational TTX contracts
- [`concept/abstract.hpp`](concept/abstract.hpp) is the root semantic query
  contract. [`concept/layout.hpp`](concept/layout.hpp) defines identity-free
  shape and fitting. [`concept/documentation.hpp`](concept/documentation.hpp)
  defines the prose query
- [`model`](model/) owns Alias, Exports, Type, Expression, Constant, Callable,
  Attribute, documentation implementations, and the concrete strategies built
  on the Concept layer
- [`model/addressables/writable.hpp`](model/addressables/writable.hpp) narrows
  Addressable to assignment capability and supplies the stable non-writable
  projection required by `expose state`
- Alias and Invalid are closed Abstract implementations. Exports, Type, Generic,
  Expression, Constant, Projection, Binding, Callable, Static, Self,
  Addressable, Writable, and Dialect-specific contracts extend the graph with
  narrow operations
- [`model/layouts`](model/layouts/) contains the Fluid, Named, Structured,
  Ranged, and Composite implementations of `Concept::Layout`
- [`model/projection.hpp`](model/projection.hpp) and
  [`model/binding.hpp`](model/binding.hpp) preserve selected and named value
  provenance without adding parser operations to the model
- [`model/type.hpp`](model/type.hpp) supplies the narrow target-independent Type
  contract. [`model/types`](model/types/) adds the Terminal family contracts and
  width-specific Perimortem Types. [`model/expression.hpp`](model/expression.hpp),
  [`model/constant.hpp`](model/constant.hpp), and
  [`model/constants`](model/constants/) supply value and constant-domain
  contracts. [`model/callable.hpp`](model/callable.hpp),
  [`model/callables`](model/callables/), and
  [`model/addressable.hpp`](model/addressable.hpp) supply invocation contracts
  without making Callable a subtype of Type
- active toolchain contexts provide the scalar, vector, and memory Types they
  support as ordinary resolvable Abstracts

Tetrodotoxin is the surrounding toolchain:

- [`../tetrodotoxin/puffer/main.cpp`](../tetrodotoxin/puffer/main.cpp) is the `puffer`
  command-line surface
- [`../tetrodotoxin/puffer/isa/boot`](../tetrodotoxin/puffer/isa/boot/) is the
  legacy implementation of Puffer's source preamble while Boot migrates to the
  direct Dialect model
- [`../tetrodotoxin/puffer/resolution`](../tetrodotoxin/puffer/resolution/) owns source
  loading, package loading, Environment assembly, the source cache, and cache validity
- [`../tetrodotoxin/lsp`](../tetrodotoxin/lsp/) serves editor features
- [`../tetrodotoxin/model/dialect.hpp`](../tetrodotoxin/model/dialect.hpp) owns
  durable Dialect identity, while
  [`../tetrodotoxin/interpreter/dialects`](../tetrodotoxin/interpreter/dialects/)
  owns concrete bytecode evaluation beginning with Package, Alias, and Group
- [`../tetrodotoxin/isa`](../tetrodotoxin/isa/) is legacy evaluator code used
  only as migration reference while Library, Shader, and the remaining domains
  move to real Model contracts
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
model while Tetrodotoxin supplies Dialect evaluation, files, packages, editor
integration, build integration, and terminal artifacts.
