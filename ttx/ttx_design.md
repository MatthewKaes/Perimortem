# TTX Language Design

TTX is a frontend source IR language. It makes data movement, layout, and
lowering intent visible without giving up the basic comfort of a hand-authored
format. A TTX file reads like a program, but it carries enough structure that a
host does not have to guess what kind of thing it is looking at.

The language is useful wherever the shape of a program should be explicit:
binary formats, shader stages, data transforms, package boundaries, FFI-like
source interchange, and other code where layout is not an implementation detail.
TTX is not trying to hide the machine. It is trying to make the machine legible.
Tetrodotoxin is the reference host in this repository, but the TTX source IR,
token bytecode, and Abstract query model are intentionally reusable outside
that toolchain. `Type` and `Layout` are important contracts in that model. They
are not the root of every semantic object.

The organizing idea is **monotonic context layering**. A TTX file begins as an
authoring surface, then is lowered into token bytecode. A host can then evaluate
an envelope, attach an Environment or module, execute a Dialect, and query Type, Layout,
provider, backend boundary, and output owners. Each layer enriches the same
source structure with additional context. It does not erase what came before
until the host intentionally emits a terminal artifact such as formatted text,
SPIR-V, LLVM IR, an object file, or editor JSON.

This file describes the language as an author sees it. The stricter rules that
matter to the compiler live in [ttx_semantics.md](ttx_semantics.md).

## Design Goals

TTX borrows from LLVM IR, MLIR, Zig, Rust, and GDScript, but it is not a direct
clone of any of them.

- Like LLVM IR, TTX is typed, explicit, and designed around values, calls,
  branches, and memory access.
- Like MLIR, TTX treats semantic domains as real extension boundaries, but uses
  named Dialects over a concrete Lexer Code stream instead of extensible
  operation syntax.
- Like Zig, it prefers simple grammar, compile-time-visible structure, and
  explicit conversions over implicit magic.
- Like Rust, it treats visibility and addressability as part of the program's
  contract.
- Like GDScript, it keeps the surface syntax light enough that authored data and
  small systems code remain easy to scan.

The main design tradeoff is intentional rigidity. TTX gives up some general
source-language conveniences so the parser, formatter, compiler, and reader all
see the same shape.

That rigidity keeps common tools cheap. Syntax highlighting needs only lexical
classification. Formatting needs syntax and comments, but not Environment. Project
navigation may need a module or package graph, but not backend code generation.
Compilation uses more layers, but it still asks the same source-shaped program
richer questions rather than reconstructing intent from a lower-level copy.

One of the central rules is the difference between reshapeable value flow and
structured typed storage. Pack syntax such as `(a, b, c)` produces a Fluid or
Named Layout directly: it can be fitted or repacked before it becomes a typed
value. A struct, color, or ABI
block normally supplies a Structured Layout made from its real Addressables. A
fixed homogeneous vector may instead supply Ranged and repeat one Type over its
index interval. Both retain Type identity. Terminal Types publish their direct
size and alignment. Composite offsets and aggregate storage are derived recursively.
Carrier and calling-convention rules remain compiler decisions. TTX lets value
flow initialize structured values when the shape fits, but it does not silently
unpack typed values back into packs. Source uses swizzle or slice syntax when it
wants to cross that boundary explicitly.

## Relationship To LLVM IR And MLIR

The short version is that LLVM IR, MLIR, and TTX all live in the IR family, but
they optimize for different moments in the toolchain:

| Area                   | LLVM IR                                             | MLIR                                                          | TTX                                                                |
| ---------------------- | --------------------------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------------ |
| Primary representation | Lowered SSA module/function/block/instruction IR    | Extensible operation/SSA IR with regions                      | Source IR plus token bytecode and context layers                   |
| Main extension point   | Intrinsics, metadata, passes, and targets           | Dialects define operations, types, attributes, and interfaces | Fixed syntax with Dialects defining evaluation, legality, and metadata |
| Typical motion         | Optimize and transform already-lowered IR           | Rewrite and convert operations between dialects               | Evaluate token bytecode and enrich queryable facts                 |
| Text form              | Debug, test, and serialization form for compiler IR | Debug, test, and serialization form for multi-level IR        | Human-authored canonical source surface                            |
| Extension granularity  | Target and metadata oriented                        | Operations from many dialects can coexist freely              | One declared Dialect controls the legal semantic world             |
| Source preservation    | Mostly lowered away before LLVM IR                  | Supported through locations and higher-level dialects         | Central design constraint                                          |
| Tooling goal           | Optimizer and code-generation substrate             | Reusable compiler infrastructure                              | Shared frontend IR for compiler, editor, and build tooling         |
| Lowering               | Already lowered enough for optimization             | Core workflow through dialect conversion                      | Delayed until terminal artifacts                                   |

TTX is closer to a human-editable source IR than to C++ with different
punctuation. It shares LLVM's bias toward explicit values, typed calls, and
visible memory shape, but it keeps source-level concepts alive much longer than
LLVM IR would.

The source format is inspired by LLVM concepts, but TTX does not globally define
every construct that might lower to LLVM-like IR. Lexical lowers authored source
into token bytecode. A host selects an evaluator for the relevant bytecode span.
That evaluator decides which token spans are executable instructions, what facts
they make queryable, and whether those facts can later lower to LLVM IR, SPIR-V,
object code, editor data, or something else.

That makes the reusable TTX model smaller than a full language tree:

| TTX data or token shape               | Usually becomes                                               |
| ------------------------------------- | ------------------------------------------------------------- |
| `Ttx::Concept::Abstract`              | naming, documentation, identity, and progressive resolution   |
| `Ttx::Concept::Layout`                | ordered Abstract shape and directional fitting                |
| `Ttx::Concept::Documentation`         | ordered borrowed authored or generated prose                  |
| `Ttx::Model::Type`                    | resolved type identity and Layout                             |
| `Ttx::Model::Types::Managed`          | proof of a managed object-reference value                     |
| `Ttx::Model::Types::Terminal`         | value width, byte size, alignment, and generated prose         |
| `Ttx::Model::Expression`              | evaluatable value with result Type and ordered input queries  |
| `Ttx::Model::Constant`                | immutable zero-input value already in normal form             |
| `Ttx::Model::Callable`                | static or receiver-bound callable layout                      |
| `Ttx::Model::Body`                    | immutable body-local blocks, values, and operations           |
| `TypeAccessOp` such as `::`           | nested type query against the current type or Environment     |
| `AddressOp` such as `.`               | layout member query, package-name segment, or Dialect projection  |
| `CallOp` such as `->`                 | callable dispatch query against current Type or Dialect facts |
| modifier and attribute token Codes    | publication, evaluation, package, or Dialect-owned metadata        |
| quoted bytes, layout, and pack tokens | source-shaped operands for the active Dialect                 |

Library, Package, Shader, Render, and future Dialects then decide what larger
source forms mean. Library may define functions, control flow, and local
storage. Shader may define stage metadata and GPU legality. Package may turn
exports into reflection facts. The syntax gives those Dialects a shared
instruction stream and a shared Abstract, Type, Callable, and Layout model, but
the Dialect owns the
meaning of its body.

That split is the important difference from a lowered IR. LLVM IR is already
past most authoring concerns. TTX keeps package names, binding names, comments,
attributes, named fields, named packs, layouts, shader entry points, foreign ABI
declarations, and compile-time directives alive until the Dialect or lowerer that
understands them can use them. The compiler lowers TTX without guessing what the
author meant: the syntax stays strict enough to avoid ambiguity, while the Dialect
model keeps the source expressive enough to manage the frontend.

TTX also differs from MLIR. MLIR makes compiler IR extensible through dialect
operations, regions, rewrite passes, and legality conversion. TTX makes authored
source semantically dense enough that many tools can share it before lowering. A
TTX host can still target MLIR when MLIR is the right artifact for that host.

## Lexical Shape

TTX is meant to reach useful token bytecode in a single left-to-right pass
through source text. The lexer therefore does more than split characters into
words. It smuggles as much semantic category information as it can into direct
syntax so Dialects can execute bytecode with minimal rediscovery. As a result,
casing and punctuation are part of the instruction stream, not decoration.

| Form          | Meaning                                                     |
| ------------- | ----------------------------------------------------------- |
| `snake_case`  | addressable runtime names                                   |
| `PascalCase`  | Types, aliases, Dialect names, and package names            |
| `.snake_case` | named pack or layout field                                  |
| `.10`         | indexed pack field for sparse table initialization          |
| `@name`       | attribute or Dialect-owned directive                        |
| `_`           | discard value                                               |
| `(...)`       | forms a packed group, but can be used to emulate precedence |

`.Red` for instance is not a valid field name. Named fields are addressables, so they must
be written as `.red`. PascalCase belongs to types in **all** token contexts.

Keywords are tokenized before the parser sees them. For example, `alias`,
`func`, `dialect`, `public`, `private`, `expose`, `state`, and `const`
are distinct token Codes, not ordinary identifiers that the parser has to
reinterpret later. They are lowercase because they are grammar forms.
PascalCase names remain open type or package atoms, so user types named
`Struct`, `Object`, or `Package` are still ordinary type names.

Whitespace is not currently significant. The authoritative formatter defines
the canonical source style. Making whitespace semantic would require an
explicit language revision rather than an incidental parser change.

## Source Envelopes and Dialects

Many TTX hosts use a source envelope so a file can declare which Dialect should
evaluate its body:

```ttx
dialect : Library;
dialect : Render;
dialect : Shader;
dialect : Package;
```

The source keyword names a `Tetrodotoxin::Model::Dialect` resolved from the
host's ordinary `Dialects` Abstract context. It does not select an enum,
function-pointer record, VM object, or second registry. The Dialect owns the
next bytecode span and may expose or consume `Ttx::Concept::Abstract`,
`Ttx::Model::Type`, `Ttx::Concept::Layout`, or other host facts directly.

A Dialect controls presentation, accepted builtins, evaluation, legality, and
its additional versioned facts. It may reject or narrow a common TTX construct,
but it may not reinterpret an existing common contract. Binding an identity in
the Environment makes that identity available; it does not import the builtins
or legality rules of the identity's producing Dialect.

The lowercase `dialect` marker is a reserved keyword. The Dialect name after
the colon is still a PascalCase type atom, so names such as `Package`,
`Library`, and `Shader` remain valid in type-access expressions like
`YourType::Package`.

In TTX, an evaluator may give a lowercase addressable spelling a definition
meaning. `object` and `struct`, for example, are Dialect-owned forms used inside
a package. Unlike the fixed `alias` keyword, they are not separate lexical
classes:

```ttx
private ImageInfo : struct {
  public width  : Unsigned_32;
  public height : Unsigned_32;
}
```

The Dialect selection line has no modifier. That keeps the top of the file
visually distinct and gives envelope evaluators a stable entry point. Evaluation
can start below top level when a host already knows which evaluator should
execute the bytecode.

`Library`, `Package`, `Render`, and `Shader` are Dialects. A Dialect is more
than a backend name: it decides which builtins, Types, attributes, and
body forms are legal in that source. For example, `dialect : Render;`
and `dialect : Shader;` do not support managed runtime forms such as `object`
or `List`, because those concepts do not exist in the stage-oriented execution
model.

A Dialect may produce more than one output. `Render` and `Shader` packages may
produce shader code such as SPIR-V embedded as binary constants and host code
that loads those constants, builds the required `Layout` values, and bridges
them into the engine runtime. The shared TTX syntax substrate still owns
declarations, layouts, packs, and modifier parsing so authors do not have to learn
an unrelated grammar for each Dialect name.

## Container Environments

Hosts construct one shared Environment before evaluating any member Source.
Package resolutions and Source membership are container inputs, not Source
statements:

```ttx
resolve Graphics : Perimortem.Graphics = "2.2";
source MathTypes : Library = "math.ttx";
```

In canonical `package.ttx`, these declarations appear after
`dialect : Package;` and before the Package-Dialect export body. Puffer parses
the descriptor first, resolves the named Dialect objects, and records the exact
Environment requests and ordered members. Once the package container completes
those inputs, Descriptor evaluation verifies the same Source and evaluates its
body from the exact remaining token. The descriptor root plus its declared
members are the Package's complete explicit Source vector.

A package resolution reads:

```ttx
resolve Alias : Package.Name = "Major.Minor";
```

The left side is the Environment-wide local Alias. The package name and
Major.Minor select one exact external artifact. Every member Source borrows
that package-owned Environment, so files can use `Graphics` without owning
package edges. The active Container completes external bindings first and then
publishes each completed member for later descriptor members. Arbitrary
cross-member declarations require a future package-owned synchronization
phase over their eventual real owners. `source` is package-container syntax.
It selects and names a member but does not become an edge on either Source. A
standalone script receives the same kind of Environment dynamically from its
host. Package names are
`Type("." Type)*`. Versions are quoted canonical text parsed directly into two
unsigned components without a floating-point intermediate; `"0.1"` is valid
and `"0.0"` is unset.

Environment bindings do not import Dialect semantics. A `Shader` Source does
not inherit managed-library semantics like `object` or `List` because a Library
product is available. It queries that product through its own Dialect's rules
after Abstract identity resolution.

As an example, injecting a Shader product into a Library Environment lets the
Library name exposed shader definitions without changing Library legality.

## Definitions

Most declarations follow one of these shapes:

```ttx
[publication] [evaluation] name : dialect;
[publication] [evaluation] name : dialect = value;
[publication] [evaluation] name : dialect { ... }
```

Publication is one of `public`, `expose`, or `private`. Evaluation is one of
`state` or `const`. A definition has at least one modifier, publication precedes
evaluation when both are present, and the active Dialect decides which
combinations are legal. The name is always either `Type` or `Addressable`. The
word after `:` selects the Dialect that evaluates the remaining definition.
Sometimes it is a normal Type query such as `Count` or `Header`. Sometimes it is
a toolchain Dialect such as `struct`, `foreign`, or `group`. `alias` selects the
Dialect that constructs an Abstract redirection, though the parent Dialect still
decides whether that continuation is legal in its scope.

The parent configures the shared `Definitions<...>` grammar with constexpr
Definition mappings. Each mapping carries the Dialect and its accepted ordered
modifier prefixes without constructing an evaluator, function-pointer record,
or polymorphic registry. Definitions consumes `(Documentation)*`, the modifier
prefix, name, and `Define`, then passes those values and the durable export owner
directly to the selected definition Dialect. A duplicate Dialect name is
rejected when the mapping type is instantiated. No transient Abstract is
created for the handoff. The selected Dialect consumes its suffix and constructs
the real TTX fact directly. “Subdialect” only describes this recursive
relationship. It is not a separate contract.

```ttx
private count : Count = 4;
private converted : Count = Count -> from(4);
```

PascalCase names define types or compile-time names. Snake_case names define
addressable values.

```ttx
private Header : struct { ... }                     // type definition
private header : Header = (.width = 4, .height = 2); // addressable value
```

This casing rule removes a common vexing parse: after `PascalCase :`, the parser
knows it is reading a type-like definition. After `snake_case :`, it is reading
an addressable definition.

### Member Order And Late Binding

Library sources follow the same presentation order as owned C++: published
members precede private members, and addressable definitions precede functions
within each publication block. Published addressables retain authored `public`
and `expose` order. The evaluator consumes and roots declarations in that order.
Duplicate-name and no-shadowing checks therefore remain ordered source checks.
Type-like definitions occupy the corresponding non-function group. Unpublished
Library-owned member `state` storage occupies the private-addressable group;
`state` does not create a third publication block.

This ordering does not introduce forward declarations. A declaration pass may
retain bounded initializer and function-body token ranges until the owning Type
or source Abstract is complete. The selected Dialect then consumes each range
once and attaches an immutable Body to the real owner. A public function body
may therefore call a private function written later, and a type-owned constant
may invoke a later intrinsic callable, without constructing a placeholder
declaration or a second semantic object. Cursor state and token ranges are not
the published executable form. Declaration headers are still checked when
consumed; a public signature cannot expose an unresolved private implementation
type.

A bodyless callable is legal only when its Dialect supplies the complete
implementation contract, such as a Foreign linkage or a Library allocation
intrinsic. It is never a prototype for a second authored declaration.

## Definition Modifiers

Definition modifiers occupy independent publication and evaluation slots. They
are ordinary fixed keyword tokens. The tokenizer provides their classes; the
active Dialect validates the combination and constructs graph relationships
instead of storing a visibility or storage Kind on the resulting Abstract.

| Publication | Meaning |
| ----------- | ------- |
| `public` | publish the target with every capability it actually proves |
| `expose` | publish owner-written state through a non-writable Addressable projection |
| `private` | retain the definition in its owner without exporting it |
| omitted | retain the definition only in the current lexical or evaluation context |

| Evaluation | Meaning |
| ---------- | ------- |
| `state` | mutable runtime storage owned by the active scope or Type |
| `const` | compile-time evaluation to a stable materialized Abstract, or a source error |
| omitted | the selected Dialect's ordinary definition semantics |

`public` publishes the target unchanged. A public writable Addressable remains
writable and a public Callable remains invocable. `expose` is legal only for
owner-written state Addressables. The owner retains the writable storage while
external resolution returns the stable Addressable supplied by
`Writable::get_read_only()`. That projection keeps the same name and resolved
Type, does not prove Writable, and does not resolve back to the writable
identity. The projected value is not constant: repeated reads can observe owner
changes. Callables, Types, aliases, enum cases, and compile-time constants use
`public` or `private`, never `expose`.

Address write capability is distinct from the Type of the value read through
that address. Exposing an `Access[T]` prevents assignment to the exposed address
but still hands the consumer the loaded `Access[T]`. Transitive read-only access
requires a published `View[T]` value.

Type-owned state participates in the Type's Structured storage Layout even when
private. External completion and ordinary inspectors enumerate exported
definitions rather than storage Layout, while lowering and explicit raw-layout
inspection use Structured. A tool-only presentation preference belongs in an
attribute or the tool instead of a core visibility modifier.

`const` never means write-once runtime storage. The active Dialect must evaluate
its initializer completely and bind the stable result. Scalars materialize as
Constants, Type expressions remain Types, and a Dialect may define another
stable compile-time domain such as a type-owned singleton. The const binding has
no writable runtime storage, though an ABI may later materialize an address for
its terminal representation.

## Attributes And Directives

Attributes are compiler directives attached to the next member, parameter, or
field:

```ttx
@builtin @slot(0) .source : View[Unsigned_8]
@packed
@stage(fragment)
@shader_type(Vec4D)
```

An attribute is either a marker or one named scalar fact. Declarations that
need several facts use several attributes, such as `@binding @set(0) @slot(1)`.
This keeps metadata lookup direct and prevents attributes from growing a second
untyped object model beside Layout.

The scalar carrier is `Core::Static::Union`. `Attribute` adds the authored key.
It does not define another tag, storage union, or dispatch mechanism.

Target-specific attributes can attach lowering facts to a Type. For example,
`@shader_type(Vec4D)` says that the authored type intentionally lowers through
the shader ABI as `Vec4D`. That fact is separate from layout fitting. A type
with four `Real_32` members is not a shader vector unless the Dialect exposes the
metadata or the query resolves to the vector Type.

`@if` is an attribute-shaped directive:

```ttx
@if(enabled) {
  state generated : Count = 1;
}
```

The disabled marker `/>` is source-level sugar for disabling the following
member. It is intended for hand-authored experiments and formatter-preserving
comments, not as a semantic feature.

## Abstract Query Model

TTX's semantic world is a directed graph of named `Abstract` objects. Every
object exposes the virtual contracts it implements and the named objects
reachable from its context. Tools ask for the contract they need instead of
assuming that every declaration is a Type:

```text
Ttx::Concept
├── Abstract
│   ├── Alias
│   ├── Invalid
│   ├── Ttx::Model::Exports
│   ├── Ttx::Model::Types::Generic
│   ├── Ttx::Model::Expression
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

`Ttx::Concept` owns foundational contracts and values that have no narrower
semantic owner. Abstract supplies identity, progressive resolution, and a
stable Documentation query. Layout supplies shape and fitting without identity.
Documentation preserves borrowed authored or generated prose. `Ttx::Model` owns the shared source-IR mechanisms layered on
those concepts. It does not own a registry, global object collection, or
parallel semantic graph.

This hierarchy is semantic, not C++ RTTI. Native and foreign-language objects
answer the same progressive Abstract queries without a central class authority.
Lower layers resolve identity and then ask for `Type` without knowing whether
the authored query passed through an Alias, Generic construction, or
Dialect-specific context. Documentation tools can inspect an authored Alias before
resolution. Any future class, schema, or reflection description is itself
another Abstract in the graph, not a repository attached to the base class.

Each declared native contract owns a stable 128-bit interface UUID and proves
its own direct inheritance chain. `abstract.is<Type>()` performs two word
comparisons per shallow level. `abstract.assume<Type>()` checks that proof and
returns `const Type&`; its name makes the caller-held invariant explicit. There
is no hash, allocation, nullable cast, global class table, or centrally assigned
type number. These UUIDs identify contract schemas only: Abstract objects still
use stable local addresses, resolution still uses borrowed names, and exports
still render reversible named ownership chains.
Shared implementation bases are not query contracts unless they explicitly
declare an identifier. The native v1 contract hierarchy has one semantic
inheritance spine per object, and an implementation may prove only its public
C++ base contracts. Foreign-language adapters may prove the same identifiers by
implementing the corresponding native adapter contract without exposing C++
vtables across the ABI.

The core contracts are deliberately narrow:

| Contract        | Responsibility                                                   |
| --------------- | ---------------------------------------------------------------- |
| `Abstract`      | name, documentation, identity redirection, and context resolution |
| `Alias`         | closed local name, accumulated documentation, and redirection     |
| `Exports`       | ordered public definition edges and contextual name resolution    |
| `Documentation` | borrowed ordered prose with no semantic identity                  |
| `Layout`        | identity-free ordered shape, fitting, and fitting evidence        |
| `Type`          | Layout shape and direct Abstract name resolution                   |
| `Terminal`      | Type leaf with value width, byte size, alignment, and prose       |
| `Unsigned`      | Terminal non-negative integer domain                             |
| `Signed`        | Terminal signed integer domain                                   |
| `Real`          | Terminal floating-point domain                                   |
| `Flag`          | Terminal two-value logical domain                                |
| `Generic`       | instruction that creates or finds a concrete Type from arguments |
| `Expression`    | one value with result Type, input Layout, and fitting queries     |
| `Constant`      | immutable zero-input Expression with value equality              |
| `Projection`    | Expression selecting an Addressable through a receiver           |
| `Binding`       | Expression giving another Expression an authored flow name       |
| `Callable`      | complete parameter/result layouts                                |
| `Static`        | invocation selected without a runtime receiver                    |
| `Self`          | invocation on an addressable receiver included as parameter zero |
| `Addressable`   | named address to typed data with an explicit Type query           |
| `Writable`      | Addressable assignment proof and stable read-only projection      |
| `Invalid`       | an absorbing failed query                                        |

The initial native query surface is intentionally small and total:

```text
Type::get_layout()             -> const Concept::Layout&
Exports::get_export_count()    -> Count
Exports::get_export(index)     -> const Abstract&
Terminal::get_width()          -> Count
Terminal::get_size()           -> Count
Terminal::get_alignment()      -> Count
Callable::get_parameters()     -> const Concept::Layout&
Callable::get_results()        -> const Concept::Layout&
Addressable::get_type()        -> const Abstract&
Writable::get_read_only()      -> const Addressable&
Generic::get_parameterization() -> View::Vector<Parameters>
Generic::find(arguments)        -> Option<Type&>
Expression::get_type()         -> const Abstract&
Expression::get_inputs()       -> const Concept::Layout&
Expression::fits(type)         -> Bool
Constant::equals(constant)     -> Bool
Layout::get_fitted(target, i)  -> const Abstract&
```

Addressable type failure is Invalid while the Addressable retains its identity.
Writable is the narrower capability proving that assignment may target that
address. Its read-only projection is a separate stable Addressable edge with
the same name and resolved Type; it does not prove Writable or resolve back to
the writable identity. The concrete producer retains the storage connection and
the active Dialect owns write evaluation and lowering.
Layout stores no copied Member record. It exposes real Abstracts, and Structured
narrows those entries to the actual Addressables owned by a Type. Names, child
Types, documentation, attributes, defaults, and Dialect facts remain on the real
objects or their richer contracts rather than becoming nullable Layout fields.
Contiguous model collections use non-null `Concept::Reference<Contract>`
values rather than raw semantic pointers.

There is no generic `Typed` marker. A query asks for the real operation it
needs. The remaining route is a borrowed `View::Bytes`. The public virtual
`Abstract::resolve()` may redirect identity. The public virtual
`resolve_context(route)` lets the object interpret that route inside its own
context. It may treat the route atomically, split it, forward a suffix, or
redirect the unchanged view. Alias redirects both operations to its target.
Named source and package contexts are host-owned Abstract extensions rather
than Types with fabricated Layouts.

The contract is intentionally flexible about lookup but strict about meaning:

- `get_name()` is local. `resolve().get_name()` asks for the canonical name.
- `resolve()` is idempotent for an unchanged valid DAG.
- differently partitioned routes need not be equivalent.
- an empty context route need not behave like `resolve()`.
- repeating the same ordered chain from the same Abstract is deterministic
  until the DAG changes.
- context redirection may forward an unchanged route, but valid graph
  construction must guarantee termination.
- every result is an Abstract reference. Semantic failure is Invalid, not null.
- pure construction and query paths remain `constexpr` when their dependencies
  permit it. Constant evaluation never justifies duplicating a contract or
  moving runtime-owned logic across its boundary.

This is the ground-truth Abstract model from which the other TTX contracts are
refined. Its surface is heavily restricted, not permanently closed. A proposed
addition must be fundamental to every semantic object and must not be a Type,
package, registry, reflection, diagnostic, ownership, or traversal concern in
disguise. As a practical warning boundary, `abstract.hpp` should remain around
150 lines or fewer.

Alias and Invalid are closed concepts. Alias owns a local name, stable
documentation that presents its local lines before the target's visible lines,
and a borrowed target. It redirects both Abstract queries and relies on the
graph owner for target lifetime and cycle rejection. Invalid is a
stateless absorbing Abstract. Diagnostics remain with the source-owning query.
Neither class grows Type, Layout, package, ownership, route-history, or
specialized failure responsibilities.

Resolution policy stays with the owning Abstract. A Type answers its named
context directly. `Widget -> open()` requires the selected Callable to prove
Static. `widget -> open()` resolves the receiver Type and requires the selected
Callable to prove Self. Receiver form does not create a second TTX lookup
interface. A missing name or wrong Callable contract returns Invalid.

An object can be reachable through several Environment bindings or Aliases. Diagnostics keep
the authored input bytes. Public symbols walk an explicitly selected ownership
chain and encode its names reversibly. They do not allocate a semantic path,
hash a signature, or choose a lexicographically preferred alias.

`Exports : Abstract` is the optional public-graph contract. Its total
`get_export_count()` and `get_export(index)` queries enumerate real exported
Abstract edges in authored publication order; an invalid index returns Invalid.
Every entry has a unique non-empty local name and is the same edge returned by
direct contextual resolution of that name. Alias therefore preserves authored
naming and documentation at the export boundary before canonical resolution.
No other name resolves from that Exports context. Nested lookup begins only
after selecting an exported edge, so private roots and outer Environments cannot
leak through the public boundary.

Exports is also the common dependency product. Source and package locators may
be completely different, but both bind their local Alias to an Exports object.
Nested groups, interpreted packages, restored packages, archive traversal,
reflection, and tools then consume the same graph without knowing whether a
Dialect, cache, or Puffer Buffer produced it. Imports, private definitions,
storage Layout, and locator records remain absent unless explicitly published.

Failure is also an object. A missing name, wrong requested contract, ambiguous
query, rejected alias-cycle construction, invalid archive, or unresolved
linkage produces an `Invalid : Abstract` result rather than `nullptr`.
Continuing a query through Invalid returns the same Invalid, leaving the source
owner's original cause authoritative so one bad name does not become a cascade
of unrelated errors.
Empty child sets are empty views. Absence is never modeled by a null
pseudo-Abstract.

Invalid is one binary-wide stateless object with private construction. An
Abstract, Layout, evaluator, or context returns that object directly when a
semantic query fails. It does not store an Invalid reference as configuration
and does not construct a subsystem-local sentinel.

TTX does not prescribe one durable Group, Namespace, Source, module, or package
contract. A host defines those domains as Abstracts, owns their indexing and
child-edge policy, and proves Exports only when the public roots are enumerable.
The durable owner answers `resolve_context()` from its Environment and
definitions already rooted into it. Evaluating a literal body enriches that
same owner instead of creating an unnamed lookup frame.

Evaluation scopes never shadow. Before accepting a parameter, loop binding, or
local definition, the evaluator checks the complete active context. If that name
already resolves, the new declaration is invalid. Leaving a nested block may
remove its local names from the active context, but entering a block never makes
an existing name available for reuse. Package and owner state remain explicit
queries such as `package.value` or `self.value`, so they do not need implicit
local aliases.

### Stable Construction And Resolution

Real TTX envelopes and Dialects may require preregistration or more than one
evaluation pass. The Abstract model permits that without imposing one
construction/publication lifecycle on every host.

A host may reserve nonmoving objects and install names that later declarations
can query. Until a requested Type or system has enough facts, it resolves that
query to Invalid. Once resolution reaches the real Type, its Layout is a total
reference. Layout therefore has no Incomplete state. An empty
Structured Layout may belong to a Terminal or a valid empty aggregate, so a
consumer proves Terminal before treating it as a scalar leaf.

The Package, Library, Foreign, Shader, Render, Scene, App, or another active
Dialect completes the facts it owns using the pass structure appropriate to
that dialect. A host may enrich the reserved object, replace an enclosing
context, or build an immutable snapshot. Abstract, Type, Layout, and Callable
prescribe none of those mutation, snapshot, or pass-management policies.

Abstract determinism applies while the DAG is unchanged. The owner that changes
or replaces the DAG also owns reference lifetime, cache invalidation, and any
revision used by its readers. Process addresses can be local identity while
kept stable, but are never durable names.

Public export is a separate optional concern. A package host may validate and
archive an immutable public surface, while an evaluator or compiler may use
internal and transient Types that are never exported. Resolution, not
publication status, determines whether a semantic fact is currently available.

## Type References

Type references are progressive PascalCase Abstract queries whose final result
must prove the `Type` contract, with optional type arguments:

```ttx
Unsigned_32
Fixed[Unsigned_8, 4]
Graphics::Image
Math::Matrix[Real_32, 4, 4]
```

The first type token resolves from the current context. `::` establishes a
nested context query. It is not string concatenation. The receiving Abstract
may interpret the remaining borrowed `View::Bytes` atomically, slice a suffix,
or redirect them unchanged. An evaluator may issue ordered queries at source
operators or offer a combined route, and those forms are not required to be
equivalent. The final result must prove Type, and no path object is allocated.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric size arguments, such as the `4` in
`Fixed[Unsigned_8, 4]`, are part of the type reference.

Parameterized types are not templates and do not generate code by themselves.
Resolution first builds the arguments, proves that the resolved object
implements `Generic`, then asks it for a concrete Type. `View[Unsigned_8]`,
`Fixed[Real_32, 4]`, and `List[Graphics::Sprite]` are all the same operation. The
compiler owns the resulting concrete object and makes it queryable through the
same Type contract as any authored type.

Each Generic is a named formula available from the current source context or a
toolchain's immutable builtin table. `Fixed` owns Fixed materialization and its
concrete-Type cache. `View` owns the corresponding View rules. There is no
mutable formula registry and no parser switch on formula names.

The formula publishes its complete ordered parameter signature up front. Each
parameter is `Type`, `Unsigned_64`, `Signed_64`, or `Bool`. The parser uses that
signature to consume and diagnose the authored list in one pass, then supplies
a compact ordered `Union<const Type&, Unsigned_64, Signed_64, Bool>` view to
`find()`. The union is a non-semantic call carrier: Type arguments preserve
resolved object identity, while scalar arguments are direct compile-time
values. Unrelated Types with the same local name remain distinct. Names,
routes, parents, and hashes do not participate in the cache key.

Malformed source or rejected values produce a parser diagnostic and
`Utility::None`; parse failure is not an Abstract graph identity. A successful
formula returns its cache-owned Type. A formula may publish a class-specific
materialized contract such as `View::Type`, allowing consumers to query
`is<View::Type>()` without treating the `View` generator as a Type.

Aliases are closed compile-time Abstract redirects. Alias preserves its local
name while `resolve()` follows the target's represented identity and
`resolve_context(route)` gives the complete route to the resolved target. Its
Documentation view returns local lines first and then the target's visible
lines. A chain of Aliases therefore accumulates authored context without
copying or modifying any target Documentation. If the target is a Type, the
consumer proves the Type contract after resolution. Alias cycles are rejected
during graph construction.

Types are also compile-time values of the standard `Type` type. This keeps a
resolved identity available for reflection and editor tooling without reducing
it to source text:

```ttx
const reflected_type : Type = Graphics::Image;
```

The stored value is a reference to the resolved Type object, not a copied source
string, allocated path, or runtime pointer ABI. Diagnostics retain the authored
route and source context. A terminal that needs runtime reflection consumes an
explicit Abstract contract defined by that runtime. C++ object addresses and
vtables never become the cross-language representation.

When a host exports an object, it renders a selected named ownership chain.
Tooling may show either the authored Alias name or the resolved target's
ownership chain, but no `display_name` attribute or side-table path is needed to
reconstruct identity. Within one compiler boundary object identity can be
compared by stable handle. Durable export identity is reversible names, never
the process address.

### Terminal Type Registration

Core TTX defines `Terminal : Type` with fixed name, Documentation, value-width,
byte-size, and byte-alignment queries. `Unsigned`, `Signed`, `Real`, and `Flag`
add the standard semantic family contracts. Width-specific classes such as
`Unsigned_8`, `Signed_32`, and `Real_64` return literal names, exact
representation facts, and rich documentation directly. Core owns neither a
prelude nor a supported-width registry. The active toolchain constructs and
installs the concrete Types it supports in its own Abstract context. An
unsupported name resolves to Invalid through that context.

The standard classes are stateless apart from their C++ virtual identity. Each
returns a literal authored name and a static Comment directly:

| Concrete Type | Family     | Value width | Storage  | Documentation                                                |
| ------------- | ---------- | ----------- | -------- | ------------------------------------------------------------ |
| `Unsigned_8`  | `Unsigned` | 8 bits      | 1 byte   | `Unsigned_8 is stored as a 1 byte unsigned integer.`         |
| `Unsigned_16` | `Unsigned` | 16 bits     | 2 bytes  | `Unsigned_16 is stored as a 2 byte unsigned integer.`        |
| `Unsigned_32` | `Unsigned` | 32 bits     | 4 bytes  | `Unsigned_32 is stored as a 4 byte unsigned integer.`        |
| `Unsigned_64` | `Unsigned` | 64 bits     | 8 bytes  | `Unsigned_64 is stored as an 8 byte unsigned integer.`       |
| `Signed_8`    | `Signed`   | 8 bits      | 1 byte   | `Signed_8 is stored as a 1 byte two's-complement integer.`   |
| `Signed_16`   | `Signed`   | 16 bits     | 2 bytes  | `Signed_16 is stored as a 2 byte two's-complement integer.`  |
| `Signed_32`   | `Signed`   | 32 bits     | 4 bytes  | `Signed_32 is stored as a 4 byte two's-complement integer.`  |
| `Signed_64`   | `Signed`   | 64 bits     | 8 bytes  | `Signed_64 is stored as an 8 byte two's-complement integer.` |
| `Real_32`     | `Real`     | 32 bits     | 4 bytes  | `Real_32 is stored as a 4 byte IEEE floating value.`         |
| `Real_64`     | `Real`     | 64 bits     | 8 bytes  | `Real_64 is stored as an 8 byte IEEE floating value.`        |
| `Real_128`    | `Real`     | 128 bits    | 16 bytes | `Real_128 is stored as a 16 byte extended floating value.`   |
| `Boolean`     | `Flag`     | 1 bit       | 1 byte   | `Bool is stored as a 1 byte logical value.`                  |

The current Perimortem C++ toolchain can register `Bool`, `Unsigned_8`,
`Unsigned_16`, `Unsigned_32`, `Unsigned_64`, `Signed_8`, `Signed_16`,
`Signed_32`, `Signed_64`, `Real_32`, `Real_64`, and `Real_128`. `Count` may
Alias the registered 64-bit Unsigned instance, while `CppSize` may Alias the
Unsigned instance for the active C++ interface. `True` and `False` are Flag
values rather than Types.

Terminal value width describes the accepted value domain. Size and alignment
describe storage. None of those facts mandate an instruction width. An
eight-bit, one-byte Unsigned Type may use a wider register or move when its
observable semantics remain correct. Another toolchain may install a different
supported set or add another Terminal subtype without changing a central type
authority.

## Builtin Definition Kinds

Only builtin definition kinds can be followed by a scope:

```ttx
private Data      : struct  { ... }
private Manager   : object  { ... }
private Api       : foreign { ... }
private StageData : Shader  { ... }
```

Other types are values and use `=` or `;`:

```ttx
private count : Count = 1;
private octet : Unsigned_8;
```

This is one of the places where TTX is deliberately closer to IR than to a
general source language. The definition keyword tells the parser what kind of
semantic object is being created.

The active Dialect may reject otherwise valid builtin forms. `object`
can be a legal builtin in a `Library` package while remaining invalid in a
`Shader` package.

`Unsigned_8` is also the byte element Type. There is no separate scalar
`Bytes` alias. Byte arrays use `View[Unsigned_8]`, `Access[Unsigned_8]`, or a
real Library-owned collection Type, while `Constants::Bytes` remains the
literal-value contract for byte arrays.

`Void` is the result Type of a Callable invocation with an empty result Layout.
It preserves an effectful invocation in the executable Body even though the
invocation produces no value. It is not a stored value or an empty aggregate,
and an optimizer must prove the invocation has no observable effects before it
may remove it.

## Functions

Functions are explicit callable signature objects, not values declared as
`Func`.

```ttx
public func sample[self, .frag_uv : Vec2D] -> Color {
  state s : Color = self.icon_texture -> sample(frag_uv);
  return (s.[r, g, b], s.a * Push.alpha);
}
```

The function syntax is:

```ttx
modifier? func name[params] -> returns block
```

Both parameters and returns are Layouts. The selected Dialect owns the
implementation block. The resulting semantic object implements `Callable` and
carries the callable signature, documentation, dispatch name, and common
identity-free Body. Body-local IDs name parameters, locals, values, and blocks;
operations retain non-null edges to real Types, Callables, Addressables,
Constants, or explicit intrinsic owners. The Body is not an Abstract, replayed
token Cursor, parse tree, or generic statement class hierarchy. A single type
may be written directly:

```ttx
public func size[] -> Count { ... }
```

Callable has two semantic subtypes. `Static` is selected through a named
context and does not consume a runtime receiver. `Self` is selected through a
runtime value and requires that receiver as parameter zero:

```ttx
public Counter : struct {
  public func from[.value : Count] -> Counter { ... }
  public func increment[self, .amount : Count] -> Counter { ... }
}

state counter : Counter = Counter -> from(1);
counter = counter -> increment(2);
```

The Library dialect expands its bare `self` shorthand into the ordinary layout
member `.self : Counter` and constructs a `Self` object. It is not an implicit
local laundered into the function by the compiler, nor is its spelling a hidden
flag. `Self::get_parameters()` exposes the complete effective layout including
that receiver. Reflection, pack fitting, invocation, and ABI lowering therefore
consume the same truth, and no compiler layer prepends `self` again. Library
syntax requires this shorthand first and rejects it for a package-level
function because a package is not a runtime value.

Static and Self may expose the same callable name because Type owns independent
static and self lookup surfaces. `Counter -> identity(...)` queries the static
surface; `counter -> identity(...)` queries the self surface. Duplicates inside
one surface are invalid. Completion selects the same surface before it
enumerates entries.

Placing a Static callable beneath a Type makes it type-owned. It does not create a
third callable subtype. “Typed function” is therefore not a semantic category.
An ABI or execution contract may enrich either callable with resolved or
external linkage. Core Callable does not acquire an address query merely
because it can be invoked.

A named layout uses fields:

```ttx
public func decode[.source : View[Unsigned_8]] -> [
  .ok : Bool,
  .image : Image,
] {
  ...
}
```

The Foreign Dialect accepts public function declarations without bodies:

```ttx
private C : foreign {
  public func inflate[
    .source : View[Unsigned_8],
    .destination : Access[Unsigned_8],
  ] -> Count;
}
```

This is not a general `external` keyword. Forward declarations are not a TTX
feature. In a `foreign` context, a function without a body is an ABI promise
owned by that Dialect.

## Grouped Value Flow

Parentheses author grouped value flow:

```ttx
()                       // empty flow
(value)                  // one positional value
(one, two)               // positional flow
(.ok = true, .value = 7) // named flow
(.10 = 50, .65 = 14)     // indexed source syntax
```

Grouped flow has no separate Abstract identity. An evaluator normalizes the
authored values and exposes `Layouts::Fluid` for positional order or
`Layouts::Named` for named fitting. The Layout directly borrows the real
Abstract entries; there is no carrier object whose only operation returns that
Layout.

Nested positional grouping exposes one flat sequence, while Composite can
retain the child Layout representations. A typed object remains one value
unless swizzle or slice explicitly produces its fields:

```ttx
(1, (2, 3)) == (1, 2, 3)
(screen_pos, 0.0, 1.0)
(screen_pos.[x, y], 0.0, 1.0)
```

Named and indexed syntax remains evaluator policy. The receiving Type and
evaluator validate designators and expand indexed source syntax into complete
positional flow or Invalid before Layout fitting. Positional, named, and
indexed modes cannot be mixed in one authored group.

## Layouts

Layouts describe the shape expected by a function, return, or loop value:

```ttx
Count
[]
[Unsigned_32, Unsigned_32]
[.x : Unsigned_32, .y : Unsigned_32]
[@builtin @slot(0) .source : View[Unsigned_8], .count : Count]
```

The model uses five narrow contracts rather than one tagged record:

- `Fluid` carries ordered positional Abstract values.
- `Named` carries ordered, uniquely named Abstract values and fits by name.
- `Structured` is returned by Type and carries its actual Addressable objects.
- `Ranged` compactly repeats one Abstract across a fixed interval and returns
  Invalid outside it.
- `Composite` joins two complete positional Layouts and delegates indexing and
  fitting to the child that owns each target segment.

Fluid and Named normally compare resolved identity against each target slot.
When a source entry implements Expression, they instead prove the target's Type
and call `Expression::fits()`. This is how a Constant can safely fit a narrower
numeric slot without teaching Layout about numeric domains. Structured fitting
continues to preserve actual Addressable identity.

Layout owns order and fitting only. It does not copy a field's Type,
documentation, attributes, default, or target storage into a generic member.
Those facts stay on the Addressable, its source owner, or a richer derived
contract. A Type or system that is not ready resolves to Invalid. There is no
Incomplete Layout and no publication bit.

Fitting also exposes its ordering evidence.
`source.get_fitted(target, target_index)` returns the original source Abstract
for that target slot. A failed fit, invalid index, or missing mapping returns
Invalid. Named therefore publishes the permutation it proved instead of making
every Dialect repeat name matching. This operation does not allocate or create
another Layout.

A named layout field starts with `.` and an addressable name. Attributes may
decorate layout fields, but the field marker remains visible before the field
body. An unnamed layout is just a list of type references.

`for` uses a layout because one produced iteration value may have more than one
field:

```ttx
for [.i : Count] in 0...count {
  ...
}

for [.x : Real_32, .y : Real_32] in points {
  ...
}
```

The iterable owns advancement and the Layout of one produced iteration value.
The binding Layout must fit that value. Its field count never controls stride.

## Expressions

Expressions are values. Assignment is not an expression in TTX because it does
not produce a value.

The shared `Expression` contract represents one evaluatable value without
prescribing its syntax or executor. Expression identity remains distinct from
result Type identity. `get_type()` returns the proven Type or Invalid,
`get_inputs()` exposes the Layout of values required to evaluate it, and
`fits(type)` answers whether the value can safely occupy a target Type. The
default fit is exact resolved Type identity.

Layout is the fundamental identity-free shape and fitting concept. Grouped
flow and Expression dependency lists use Layout directly because neither an
operand list nor a multi-value result needs a second Abstract identity.

`Projection` and `Binding` are concrete Expressions. Projection retains a
receiver and selected Addressable. Binding retains an authored name and one
underlying Expression so Named flow contains real Abstracts instead of shadow
name and value arrays.

`Constant` is an immutable Expression already in normal form. It has an empty
input Layout and compares by domain, resolved Type, and payload. The common
open domains are Unsigned, Signed, Real, Flag, and Bytes. Integer domains may
prove narrower contextual fits from their values, and Flag fits any Flag Type.
Real and Bytes use exact Type fitting in the first slice. Real NaNs compare as
one semantic value so equality remains suitable for caches. TTX has no native
String Constant. Quoted source decodes to bytes, while a language may build a String
Type and operations above that data. Parsing, evaluation, folding, and lowering
remain Dialect or compiler concerns. A future foldable expression can expose a
Constant without adding evaluation to every Expression.

Executable syntax is owned by the Dialect that understands it. The Dialect
consumes that syntax into one common identity-free Body retained by a stable
Callable, Shader Stage, or App lifecycle owner. Different host and target
consumers read the same Body; they do not maintain competing Library and Shader
statement trees. Target Representation derives offsets, address spaces,
storage classes, register classes, and ABI carriers without adding any of those
facts to Layout.

The expression grammar follows a conventional precedence ladder:

```text
range
or
and
comparison
addition/subtraction
multiplication/division/modulo
unary
postfix
primary
```

Unary `-` is an operator, not part of the numeric token. The lexer always emits
`-` as subtraction, and the syntax parser reads it as unary only when an operand
is expected. That keeps `value-1`, `value - 1`, and `value - -1` independent of
whitespace and free of speculative parsing.

The logical operators are words:

```ttx
ready and count > 0
failed or !valid
```

Bitwise `&` and `|` are reserved. Bit operations are methods so width
conversion stays explicit:

```ttx
Unsigned_32 -> from(value) -> bit_and(Unsigned_32 -> from(0xFF));
```

## Access Chains

Postfix access is the heart of TTX expression syntax:

```ttx
Graphics.version
self.texture
source:[0]
source:[0, 4]
source -> slice(start, count)
color.[r, g, b]
source -> get_size()
Image -> from_bytes(bytes)
Color -> from(value)
callback -> invoke(args)
```

The access forms are:

| Syntax            | Meaning                                   |
| ----------------- | ----------------------------------------- |
| `.field`          | field or named-context member access      |
| `:[index]`        | positional index access                   |
| `:[start, count]` | fixed-count positional slice              |
| `.[a, b, c]`      | swizzle into a positional pack            |
| `-> name(pack)`   | callable dispatch from the left-side base |

Calls take a pack because call arguments are written with `(...)`, and `(...)`
is always a pack. `.` is lookup only. `->` marks every call. A Type or Source
context resolves a `Static` callable. An addressable value queries a
`Self` callable from its Type and contributes the declared receiver
argument.
Function-pointer dispatch is Self dispatch on the callable value, such as
`callback -> invoke(args)`.

`:[index]` produces one element Expression. `:[start, count]` produces a Fluid
Layout whose `count` must evaluate to an Unsigned Constant because it fixes the
result shape. A constant index or start can select from any known Structured or
Ranged Layout. A concrete Addressable selection uses Projection. A uniform
positional selection is a Dialect-owned indexed Expression: a fixed homogeneous
Type proves its element through Ranged, while a dynamic-extent View uses its
Type's narrow Dialect-owned indexed access contract. A dynamic index or start is
legal only along that uniform path. It retains the runtime position without
creating a dynamic Addressable or adding runtime members to Layout.

An assignable fixed slice is a fixed aggregate target. A concrete selected
Addressable must prove Writable. A dynamic uniform position instead requires
write capability from the receiver's Dialect-owned indexed access contract; it
does not manufacture one Writable Addressable per element. The right side must
fit the homogeneous range or positional Layout.
`target:[8, 5] = 0x[08 06 00 00 00];` is therefore one fixed wire write rather
than five scalar statements. This does not make a typed value splat in ordinary
value flow because the authored slice is the explicit assignment boundary.

When the count itself is dynamic, ordinary Self dispatch such as
`value -> slice(start, count)` returns one View-like typed value. The receiver's
indexed access contract owns its uniform element facts and runtime bounds
behavior; the result is not a variable-arity pack.

## Aggregate Initialization And Explicit Conversion

Aggregate values are initialized by fitting packs to an expected type:

```ttx
state uv    : Vec2D = (0.0, 1.0);
state color : Color = (.r = 1.0, .g = 0.0, .b = 0.0, .a = 1.0);
```

The target type supplies the layout. A positional pack must match that layout
by order. A named pack must match the target's field names exactly.

`-> from(...)` is reserved for explicit conversion, not ordinary aggregate
construction:

```ttx
state count : Count = Count -> from(width);
state kind  : ColorType = ColorType -> from(byte);
```

Use conversion when the value is changing representation or meaning. Use a pack
when the values already are the aggregate's components.

TTX does not support braced initializers. Braces are for scopes and statement
blocks. Aggregate values are initialized with packs:

```ttx
private values : Fixed[Unsigned_32, 4] = (1, 2, 3, 4);

private quad_uvs : Fixed[Vec2D, 6] = (
  (.x = 0.0, .y = 0.0),
  (.x = 1.0, .y = 0.0),
  (.x = 1.0, .y = 1.0),
  (.x = 0.0, .y = 0.0),
  (.x = 1.0, .y = 1.0),
  (.x = 0.0, .y = 1.0),
);
```

This keeps initialization on the same path as calls, returns, and attributes:
produce a pack, then fit it to the expected type.

## Assignment Statements

Assignment is a statement:

```ttx
target = value;
target += value;
target -= value;
```

The left side must be an assignable access chain. A concrete final Addressable
must prove Writable; a uniform runtime index instead requires the receiver's
Dialect-owned indexed write proof. A read-only projection returned for
`expose state` remains readable but cannot be assigned. Valid roots are:

```ttx
name
self.field
```

`self` is only an assignment root when it has an access suffix. Bare
`self = value;` is invalid. A package Source is not a lowercase pseudo-root or
a runtime value. Package-owned state is reached through an ordinary bound
Addressable or Type context.

This rule is why the grammar has `assignChain assignOp expr` instead of parsing
assignment as a low-precedence expression. It gives better errors and prevents
side-effecting assignments from appearing where a value is expected.

## Statements

A statement begins with one of a small number of shapes:

```ttx
state total : Count = 0;     // declaration
return total;                 // return
if (total > 0) { ... }        // scope keyword
source -> copy_to(dest);      // expression statement
total += 1;                   // assignment statement
```

Only keywords spawn scopes: `if`, `for`, `while`, and `match`. Attribute-shaped
directives such as `@if` may also spawn scopes when a Dialect chooses to own them.
`break;` and `continue;` are simple control statements, not scope forms.

`if` and `while` require a condition pack:

```ttx
if (ready) { ... }
while (index < count) { ... }
```

The complete condition pack must fit the single-value `Bool` Layout. Numeric
truthiness is intentionally not implicit.

`match` has explicit case blocks:

```ttx
match value {
  case 0: {
    return false;
  }
  case _: {
    return true;
  }
}
```

## Enums

Enums use a storage-typed brace scope:

```ttx
private Color : enum[Unsigned_8] {
  red = 1;
  green = 2;
  blue = 3;
}
```

The compiler treats enum members as public compile-time values inside the enum
namespace and checks each value against the declared storage type. The meaning
is equivalent to:

```ttx
private Color : struct {
  public const red   : Unsigned_8 = 1;
  public const green : Unsigned_8 = 2;
  public const blue  : Unsigned_8 = 3;
}
```

The internal representation is compiler-owned, but the source storage type is
part of the contract. Enum members are named compile-time values, not runtime
fields. The brace scope also leaves room for enum-owned functions without
inventing a second declaration form.

## Literals

TTX literals are intentionally small:

```ttx
42
0xFF
0.5
"Raw bytes"
0x[AA FF 12 45 ACDE]
$[path/to/file]
true
false
```

`0x[...]` is a byte literal. Whitespace separates digits for people but is not
part of the value. Hexadecimal digits are paired from left to right, so `ACDE`
contributes the two bytes `AC DE`. `$[...]` embeds a file as data. Quoted byte
literals decode escape sequences into bytes and do not include an implicit null
terminator. TTX assigns no native String meaning to those bytes.

Integer literals are exact integer values. When no narrower expected type is
present they default to the language's 64-bit integer domain, with `Count`
serving as the ordinary size/count alias. Type and layout fitting may fit an integer literal
into a narrower numeric target only when the value is provably in range.

## Documentation

Line comments start with `//`. A lexical comment token stores one stripped
comment line. Consecutive comment tokens before a type, member, or function are
collected into one `Documentation` object in source order:

```ttx
// Stored in source order.
// Attached to the following member.
private signature : Fixed[Unsigned_8, 8] = 0x[89 50 4E 47];
```

Documentation is a first-class Concept contract because every Abstract needs a
stable prose query. It is not an Abstract and does not participate in identity,
Layout equivalence, fitting, or resolution. `Comment` supplies one generated
line or the shared empty result. `Comments` borrows an ordered source-line view.
Alias supplies a stable composed implementation that presents local lines and
then its target's visible lines. Constants return the empty Comment because
documentation for a named constant belongs to its Addressable owner.

## Complete Render, Shader, And App Example

The canonical vertical uses three ordinary source files. They are shown here
in full because their owner edges, rather than hidden naming policy, define the
executable program.

The Render source declares the value state, constant arrays, push constants,
Image resource, and exact Vertex/Fragment contracts that an implementation
must satisfy:

```ttx
dialect : Render;

public Render2D : Render {
  public position : Types::Point2D = (.x = 0.0, .y = 0.0);
  public size_pixels : Math::Geometry::Size2D = (.width = 0, .height = 0);
  public tone : Types::Color = (.r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0);
  public image : Types::Image = Types::Image -> from();

  constants {
    const quad_positions : Fixed[Types::Point2D, 6] = (
      (.x = 0.0, .y = 0.0),
      (.x = 1.0, .y = 0.0),
      (.x = 1.0, .y = 1.0),
      (.x = 0.0, .y = 0.0),
      (.x = 1.0, .y = 1.0),
      (.x = 0.0, .y = 1.0),
    );

    const quad_uvs : Fixed[Types::Point2D, 6] = (
      (.x = 0.0, .y = 0.0),
      (.x = 1.0, .y = 0.0),
      (.x = 1.0, .y = 1.0),
      (.x = 0.0, .y = 0.0),
      (.x = 1.0, .y = 1.0),
      (.x = 0.0, .y = 1.0),
    );
  }

  push_constants {
    const position : Types::Point2D = self.position;
    const size_pixels : Math::Geometry::Size2D = self.size_pixels;
    const tone : Types::Color = self.tone;
  }

  resources {
    const image : Types::Image = self.image @binding(0, 0);
  }

  public vertex : stage Vertex {
    reads constant[quad_positions, quad_uvs];
    reads push[position, size_pixels];
    input [
      .vertex_index : Unsigned_32 @builtin(VertexIndex),
    ];
    output [
      .texture_uv : Types::Point2D @location(0),
      .screen_position : Vec4D @builtin(Position),
    ];
  }

  public pixel : stage Fragment {
    reads push[tone];
    reads resource[image];
    input [
      .texture_uv : Types::Point2D @location(0),
    ];
    output [
      .color : Types::Color @location(0),
    ];
  }
}
```

The Shader source names that real Render identity and supplies exactly one Body
for each required Stage. `constant`, `push`, and `resource` are Shader-owned
views of the Addressables declared by Render, not ambient globals. The sampling
expression dispatches to the real Image receiver Callable:

```ttx
dialect : Shader;

shader Default2D : Renderer2D::Render2D {
  func vertex[.vertex_index : Unsigned_32] -> [
    .texture_uv : Types::Point2D,
    .screen_position : Vec4D,
  ] {
    state quad_position : Types::Point2D =
        constant.quad_positions:[vertex_index];
    state centered_quad_position : Types::Point2D =
        quad_position - (.x = 0.5, .y = 0.5);
    state normalized_size : Types::Point2D = (
      .x = Real_32(push.size_pixels.width),
      .y = Real_32(push.size_pixels.height),
    );
    state screen_position : Types::Point2D =
        push.position + centered_quad_position * normalized_size;
    return (
      .texture_uv = constant.quad_uvs:[vertex_index],
      .screen_position = (
        .x = screen_position.x,
        .y = screen_position.y,
        .z = 0.0,
        .w = 1.0,
      ),
    );
  }

  func pixel[.texture_uv : Types::Point2D] -> [
    .color : Types::Color,
  ] {
    state sample : Types::Color = resource.image -> sample(texture_uv);
    return (
      .color = (
        .r = sample.r * push.tone.r,
        .g = sample.g * push.tone.g,
        .b = sample.b * push.tone.b,
        .a = sample.a * push.tone.a,
      ),
    );
  }
}
```

Evaluation constructs one real `Render2D` Type, its real Addressables and
required Stage Callables, one real `Default2D` Shader, two implemented Stage
owners, and two common identity-free Bodies. Shader validation proves the
directional Layout fits and declared access sets before lowering. The SPIR-V
planner derives temporary target Representation, emits and internally
validates the Vertex and Fragment modules, then publishes their stable logical
terminal paths and interface sidecars. Archive format 1 stores the semantic owner
edges, Bodies, versioned Render/Shader facts, product relations, and terminal
bytes. Restoration reconstructs the same owner relations without Source and
reuses the stored module bytes.

The complete App source uses ordinary local Callable names but assigns their
runtime meaning through direct lifecycle edges. Render submission and Shader
selection are equally explicit:

```ttx
dialect : App;

public Demo : App {
  state render : Graphics::Render2D;

  private func prepare[self] -> [] {
    return ();
  }

  private func draw[self, .frame : Runtime::Frame] -> [Flag] {
    return true;
  }

  private func release[self] -> [] {
    return ();
  }

  lifecycle {
    start = prepare;
    frame = draw;
    stop = release;
  }

  render_root render;
  bind Graphics::Render2D -> Graphics::Shaders::Default2D;
}
```

App evaluation constructs one managed App Type, real state Addressables,
self-typed Callables and Bodies, direct start/frame/stop role edges, one render
root, and one exact Render-to-Shader edge. Runtime creates a worker-local Realm,
roots App state, calls start once, supplies a typed Frame, calls frame, submits
the explicit Render-root selection through the explicit Shader binding,
consumes the real Flag continue/exit result, calls stop once, releases Graphics,
collects, and tears down the Realm. The current neutral transaction does not
yet evaluate or carry the concrete Render field value. Archive format 1 stores the
same App owner edges and Bodies, so the source-free Package follows the
identical lifecycle without magic-name search or `Invalid` as a runtime value.

The containing descriptors make external identities and member ownership
visible:

```ttx
dialect : Package;

resolve Graphics : Perimortem.Graphics = "1.0";
resolve Runtime : Perimortem.Runtime = "1.0";
source Main : App = "main.ttx";

public Demo : alias = Main::Demo;
```

### Multi-Scene Application

The larger canonical App fixture separates reusable Scene behavior from App
composition policy. Splash and Title declare only their own state, lifecycle,
render roots, and typed outcomes. The App connects those outcomes after both
Scene owners exist:

```ttx
dialect : App;

public SceneDemo : App {
  scenes {
    initial Splash::SplashScreen;
    on Splash::SplashScreen::finished replace Title::TitleScreen;
    on Title::TitleScreen::shift_pressed replace Splash::SplashScreen;
    on Title::TitleScreen::space_pressed exit;
  }

  bind Graphics::Render2D -> Graphics::Shaders::Default2D;
}
```

The two replacement edges form a runtime state-machine loop, not a package
resolution loop. `SplashScreen` does not name `TitleScreen`, and `TitleScreen`
does not name `SplashScreen`. Each frame returns a real `Scene::Flow` that stays
or emits a Scene-owned signal. The App transition table owns the target Scene
identity and the replace or exit policy.

[`apps/canonical/scene_demo`](../apps/canonical/scene_demo/) contains the full
package, Splash fade logic, Title input logic, and App composition. It is a
normative design pressure fixture. Tetrodotoxin currently parses its Package
descriptor but does not yet implement the Scene evaluator, runtime transition
transaction, evaluated Render payload submission, or Scene archive schema.
Keeping that status explicit prevents source tokenization from being presented
as semantic execution.

The smaller production-evaluated Package, Library, Render, Shader, and App
vertical has this complete implemented chain. The Scene fixture is the next
consumer of it:

```text
authored source
-> evaluated owner-shaped semantic facts and common Bodies
-> SPIR-V target Representation or Realm runtime plan
-> terminal modules or deterministic lifecycle submission
-> archive format 1 records and definition-ID edges
-> restored source-free graph
-> byte-identical modules and equivalent lifecycle behavior
```

## Why The Grammar Is Small

The grammar remains compact because many apparent language features are the
same concept:

- Calls, attributes, aggregate values, `if`, and `while` all use packs.
- Function parameters, returns, and `for` bindings all use layouts.
- Field access, swizzle, index, slice, and method call are all postfix access.
- Visibility and storage are modifier tokens.
- Type paths and aliases are type references.
- Assignment is a statement, not an expression.

The result is a language that can grow compiler power without growing many
syntactic special cases.
