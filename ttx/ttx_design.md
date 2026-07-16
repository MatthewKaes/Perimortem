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
an envelope, attach imports or modules, execute an ISA, and query type, layout,
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
  installed ISAs over a fixed token bytecode instead of extensible operation
  syntax.
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
classification. Formatting needs syntax and comments, but not imports. Project
navigation may need a module or package graph, but not backend code generation.
Compilation uses more layers, but it still asks the same source-shaped program
richer questions rather than reconstructing intent from a lower-level copy.

One of the central rules is the difference between reshapeable value flow and
structured typed storage. A pack such as `(a, b, c)` is Fluid or Named: it can
be fitted or repacked before it becomes a typed value. A struct, vector, color,
scalar, or ABI block supplies a Structured Layout made from its real
Addressables and retains Type identity. Terminal Types publish their direct size
and alignment. Composite offsets and aggregate storage are derived recursively.
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
| Main extension point   | Intrinsics, metadata, passes, and targets           | Dialects define operations, types, attributes, and interfaces | Fixed syntax with ISAs defining evaluation, legality, and metadata |
| Typical motion         | Optimize and transform already-lowered IR           | Rewrite and convert operations between dialects               | Evaluate token bytecode and enrich queryable facts                 |
| Text form              | Debug, test, and serialization form for compiler IR | Debug, test, and serialization form for multi-level IR        | Human-authored canonical source surface                            |
| Extension granularity  | Target and metadata oriented                        | Operations from many dialects can coexist freely              | One declared ISA controls the legal semantic world                 |
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

| TTX data or token shape                | Usually becomes                                               |
| -------------------------------------- | ------------------------------------------------------------- |
| `Ttx::Abstraction::Abstract`           | named semantic identity and progressive context resolution    |
| `Ttx::Model::Type`                     | resolved type identity and Structured layout                  |
| `Ttx::Model::Types::Terminal`           | direct target byte size and alignment                         |
| `Ttx::Model::Expression`               | evaluatable value with result Type and ordered input queries  |
| `Ttx::Model::Constant`                 | immutable zero-input value already in normal form             |
| `Ttx::Model::Callable`                 | static or receiver-bound callable layout and linkage query    |
| `Ttx::Model::Layout`                   | ordered Abstract shape and directional fitting                |
| `Ttx::Model::Documentation`            | source-authored prose for tools and exported facts            |
| `TypeAccessOp` such as `::`            | nested type query against the current type or import context  |
| `AddressOp` such as `.`                | layout member query, package-name segment, or ISA projection  |
| `CallOp` such as `->`                  | callable dispatch query against the current type or ISA facts |
| modifier and attribute token classes   | visibility, storage, package, or ISA-owned metadata           |
| quoted bytes, layout, and pack tokens  | source-shaped operands for the active ISA                     |

Library, Package, Shader, Render, and future ISAs then decide what larger source
forms mean. A Library ISA may define functions, control flow, and local storage.
A Shader ISA may define stage metadata and GPU legality. A Package ISA may turn
exports into type facts. The syntax gives those ISAs a shared instruction stream
and a shared Abstract, Type, Callable, and Layout model, but the ISA owns the
meaning of its body.

That split is the important difference from a lowered IR. LLVM IR is already
past most authoring concerns. TTX keeps package names, imports, comments,
attributes, named fields, named packs, layouts, shader entry points, foreign ABI
declarations, and compile-time directives alive until the ISA or lowerer that
understands them can use them. The compiler lowers TTX without guessing what the
author meant: the syntax stays strict enough to avoid ambiguity, while the ISA
model keeps the source expressive enough to manage the frontend.

TTX also differs from MLIR. MLIR makes compiler IR extensible through dialect
operations, regions, rewrite passes, and legality conversion. TTX makes authored
source semantically dense enough that many tools can share it before lowering. A
TTX host can still target MLIR when MLIR is the right artifact for that host.

## Lexical Shape

TTX is meant to reach useful token bytecode in a single left-to-right pass
through source text. The lexer therefore does more than split characters into
words. It smuggles as much semantic category information as it can into direct
syntax so ISAs can execute bytecode with minimal rediscovery. As a result,
casing and punctuation are part of the instruction stream, not decoration.

| Form          | Meaning                                                     |
| ------------- | ----------------------------------------------------------- |
| `snake_case`  | addressable runtime names                                   |
| `PascalCase`  | types, aliases, ISA names, and package names                |
| `.snake_case` | named pack or layout field                                  |
| `.10`         | indexed pack field for sparse table initialization          |
| `@name`       | attribute or ISA-owned directive                            |
| `_`           | discard value                                               |
| `(...)`       | forms a packed group, but can be used to emulate precedence |

`.Red` for instance is not a valid field name. Named fields are addressables, so they must
be written as `.red`. PascalCase belongs to types in **all** token contexts.

Keywords are tokenized before the parser sees them. For example, `alias`,
`func`, `import`, `dialect`, `public`, `private`, `expose`, `state`, and `const`
are distinct token classes, not ordinary identifiers that the parser has to
reinterpret later. They are lowercase because they are grammar forms.
PascalCase names remain open type or package atoms, so user types named
`Struct`, `Object`, or `Package` are still ordinary type names.

Whitespace is not currently significant. The authoritative formatter defines
the canonical source style. Making whitespace semantic would require an
explicit language revision rather than an incidental parser change.

## Source Envelopes and ISAs

Many TTX hosts use a source envelope so a file can declare which ISA should
evaluate its body:

```ttx
dialect : Library;
dialect : Render;
dialect : Shader;
dialect : Package;
```

The source keyword remains `dialect`, but semantically this instruction names an
evaluator installed in the active host. Puffer implements this convention with a
direct Boot ISA call and Tetrodotoxin's `Isa::Registry`, but that registry is a
toolchain detail. An ISA is not a separate lowered IR stage. It is the
instruction set that owns the next bytecode span and may expose or consume
`Ttx::Abstraction::Abstract`, `Ttx::Model::Type`, `Ttx::Model::Layout`, or
other host facts.

The lowercase `dialect` marker is a reserved keyword. The ISA name after
the colon is still a PascalCase type atom, so names such as `Package`,
`Library`, and `Shader` remain valid in type-access expressions like
`YourType::Package`.

In TTX, an evaluator may give a lowercase addressable spelling a definition
meaning. `object` and `struct`, for example, are ISA-owned forms used inside a
package. Unlike the fixed `alias` keyword, they are not separate lexical
classes:

```ttx
private ImageInfo : struct {
  public width  : Bits_32;
  public height : Bits_32;
}
```

The ISA selection line has no modifier. That keeps the top of the file visually
distinct and gives envelope evaluators a stable entry point. Evaluation can
start below top level when a host already knows which evaluator should execute
the bytecode.

`Library`, `Package`, `Render`, and `Shader` are ISAs. An ISA is
more than a backend name: it decides which builtins, types, attributes, and
body forms are legal in that source. For example, `dialect : Render;`
and `dialect : Shader;` do not support managed runtime forms such as `object`
or `List`, because those concepts do not exist in the stage-oriented execution
model.

An ISA may produce more than one output. `Render` and `Shader` packages may
produce shader code such as SPIR-V embedded as binary constants and host code
that loads those constants, builds the required `Layout` values, and bridges
them into the engine runtime. The shared TTX syntax substrate still owns
declarations, layouts, packs, and modifier parsing so authors do not have to learn
an unrelated grammar for each ISA name.

## Imports

For hosts that use the common envelope, imports define named package
dependencies to translate into the local ISA:

```ttx
import Graphics : Package = Perimortem.Graphics;
import Math     : Library = "math.ttx";
```

An import reads like a special definition:

```ttx
import Alias : IsaName = source;
```

The left side creates the local name. The ISA name describes what the host
expects the target source to declare. The right side is either a source path,
such as `"math.ttx"`, or a package name such as `Perimortem.Graphics`.
Package names are `Type("." Type)*`. A parsed package name is already a valid
cache key and folder name for hosts that persist package artifacts.

Imports do not import ISA semantics. A `Shader` package does not inherit
managed-library semantics like `object` or `List` by importing a `Library`.
Instead the imported definitions are queried through the importing ISA's rules
after Abstract identity resolution.

As an example: importing a `Shader` into a `Library` allows the `Library` to set
push constants to the Shader or talk to the GPU via exposed definitions.

## Definitions

Most declarations follow one of these shapes:

```ttx
modifier name : qualifier;
modifier name : qualifier = value;
modifier name : qualifier { ... }
```

The qualifier is always written at the declaration site. It is the source word
that tells the active ISA how to evaluate the definition. Sometimes that word is
a normal type query, such as `Count` or `Header`. Sometimes it is an ISA-owned
builtin such as `struct`, `foreign`, or `group`. `alias` is the fixed
Abstract-redirection definition word, though the active ISA still decides
whether that form is legal in its scope.

The shared definition parser only extracts the shape: modifier, name,
`Define`, and qualifier. The active ISA then decides whether the modifier and
qualifier are legal, whether the body opens a scope, and what TTX facts become
queryable.

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

## Modifiers

Modifiers describe visibility, ownership, and storage shape. They are ordinary
keyword tokens. The exact semantic contract belongs to the active ISA, but the
shared spellings let ISAs reuse the same definition spine:

| Modifier  | Intended meaning                                     |
| --------- | ---------------------------------------------------- |
| `public`  | visible API that other sources may read or call      |
| `private` | local implementation detail owned by the current ISA |
| `expose`  | externally readable data, written by its owner       |
| `state`   | stateful storage that is not part of the value shape |
| `const`   | write-once or compile-time data                      |

The tokenizer only provides the keyword class. Library, Package, Shader, and
future ISAs decide which modifiers are legal at each instruction and what facts
they expose through the TTX data model.

## Attributes And Directives

Attributes are compiler directives attached to the next member, parameter, or
field:

```ttx
@builtin @slot(0) .source : View[Bytes]
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
with four `Real_32` members is not a shader vector unless the ISA exposes the
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
Ttx::Abstraction::Abstract
├── Ttx::Abstraction::Alias
├── Ttx::Abstraction::Invalid
├── Ttx::Model::Generic
├── Ttx::Model::Expression
│   └── Ttx::Model::Constant
│       └── Ttx::Model::Constants::{Unsigned, Signed, Real, Flag, Bytes}
├── Ttx::Model::Type
│   ├── Ttx::Model::Types::Terminal
│   │   └── Unsigned / Signed / Real / Flag
│   └── ISA-defined model types
├── Ttx::Model::Callable
│   ├── Ttx::Model::Static
│   └── Ttx::Model::Self
└── Ttx::Model::Addressable
```

`Ttx::Abstraction` owns only the restricted identity and resolution substrate.
`Ttx::Model` owns the shared source-IR vocabulary layered on that substrate. It
does not own a registry, global object collection, or parallel semantic graph.

This hierarchy is semantic, not C++ RTTI. Native and foreign-language objects
answer the same progressive Abstract queries without a central class authority.
Lower layers resolve identity and then ask for `Type` without knowing whether
the authored query passed through an Alias, Generic construction, or
ISA-specific context. Documentation tools can inspect an authored Alias before
resolution. Any future class, schema, or reflection description is itself
another Abstract in the graph, not a repository attached to the base class.

Each declared native contract owns a stable 128-bit interface UUID and proves
its own direct inheritance chain. `abstract.is<Type>()` performs two word
comparisons per shallow level. `abstract.as<Type>()` checks that proof and
returns `const Type&`. There is no hash, allocation, nullable cast, global class
table, or centrally assigned type number. These UUIDs identify contract schemas
only: Abstract objects still use stable local addresses, resolution still uses
borrowed names, and exports still render reversible named ownership chains.
Shared implementation bases are not query contracts unless they explicitly
declare an identifier. The native v1 contract hierarchy has one semantic
inheritance spine per object, and an implementation may prove only its public
C++ base contracts. Foreign-language adapters may prove the same identifiers by
implementing the corresponding native adapter contract without exposing C++
vtables across the ABI.

The core contracts are deliberately narrow:

| Contract      | Responsibility                                                   |
| ------------- | ---------------------------------------------------------------- |
| `Abstract`    | name, identity redirection, and progressive context resolution   |
| `Alias`       | closed named redirection to another Abstract                     |
| `Type`        | resolved identity with a total Structured Layout query           |
| `Terminal`    | Type leaf with direct byte size and alignment                    |
| `Unsigned`    | Terminal non-negative integer domain                             |
| `Signed`      | Terminal signed integer domain                                   |
| `Real`        | Terminal floating-point domain                                   |
| `Flag`        | Terminal two-value logical domain                                |
| `Generic`     | instruction that creates or finds a concrete Type from arguments |
| `Pack`        | grouped value flow exposing an identity-free fitting Layout        |
| `Expression`  | one value with result Type, input Layout, and fitting queries       |
| `Constant`    | immutable zero-input Expression with value equality                |
| `Projection`  | Expression selecting an Addressable through a receiver              |
| `Binding`     | Expression giving another Expression an authored flow name          |
| `Callable`    | complete parameter/result layouts and an address/linkage query   |
| `Static`      | invocation selected through a Type or package without a receiver |
| `Self`        | invocation on an addressable receiver included as parameter zero |
| `Addressable` | named semantic edge resolving to the addressed Abstract           |
| `Invalid`     | an absorbing failed query                                        |

The initial native query surface is intentionally small and total:

```text
Type::get_layout()             -> const Layouts::Structured&
Terminal::get_size()           -> Count
Terminal::get_alignment()      -> Count
Callable::get_parameters()     -> const Layout&
Callable::get_results()        -> const Layout&
Callable::get_address()        -> const Abstract&
Generic::materialize(args)     -> const Abstract&
Pack::get_layout()             -> const Layout&
Expression::get_type()         -> const Abstract&
Expression::get_inputs()       -> const Layout&
Expression::fits(type)         -> Bool
Constant::equals(constant)     -> Bool
Layout::get_fitted(target, i)  -> const Abstract&
```

Addressable failure remains Invalid or an explicitly unresolved Addressable.
Layout stores no copied Member record. It exposes real Abstracts, and Structured
narrows those entries to the actual Addressables owned by a Type. Names, child
Types, documentation, attributes, defaults, and ISA facts remain on the real
objects or their richer contracts rather than becoming nullable Layout fields.
Contiguous model collections use non-null `Abstraction::Reference<Contract>`
values rather than raw semantic pointers.

There is no generic `Typed` marker. A query asks for the real operation it
needs. The remaining route is a borrowed `View::Bytes`. The public virtual
`Abstract::resolve()` may redirect identity. The public virtual
`resolve_context(route)` lets the object interpret that route inside its own
context. It may treat the route atomically, split it, forward a suffix, or
redirect the unchanged view. Alias redirects both operations to its target, and
a package is simply a top-level Type with an appropriate child index.

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

This is the ground-truth Abstract model from which the other TTX contracts are
refined. Its surface is heavily restricted, not permanently closed. A proposed
addition must be fundamental to every semantic object and must not be a Type,
package, registry, reflection, diagnostic, ownership, or traversal concern in
disguise. As a practical warning boundary, `abstract.hpp` should remain around
150 lines or fewer.

Alias and Invalid are closed concepts. Alias owns only a local name and a
borrowed target, redirects both Abstract queries, and relies on the graph owner
for target lifetime and cycle rejection. Invalid is a stateless absorbing
Abstract. Diagnostics remain with the source-owning query. Neither class grows
Type, Layout, documentation, package, ownership, route-history, or specialized
failure responsibilities.

Resolution policy stays with the owning object. A Type may keep separate static
and self maps, permitting both surfaces to contain `open` without introducing a
global contract discriminator. `Widget -> open()` asks Widget's static surface.
`widget -> open()` resolves the receiver Type and asks Widget's self surface.
Duplicates are rejected within the selected surface.

An object can be reachable through several imports or aliases. Diagnostics keep
the authored input bytes. Public symbols walk an explicitly selected ownership
chain and encode its names reversibly. They do not allocate a semantic path,
hash a signature, or choose a lexicographically preferred alias.

Failure is also an object. A missing name, wrong requested contract, ambiguous
query, rejected alias-cycle construction, invalid archive, or unresolved
linkage produces an `Invalid : Abstract` result rather than `nullptr`.
Continuing a query through Invalid returns the same Invalid, leaving the source
owner's original cause authoritative so one bad name does not become a cascade
of unrelated errors.
Empty child sets are empty views. Absence is never modeled by a null
pseudo-Abstract.

### Stable Construction And Resolution

Real TTX envelopes and body ISAs may require preregistration or more than one
evaluation pass. The Abstract model permits that without imposing one
construction/publication lifecycle on every host.

A host may reserve nonmoving objects and install names that later declarations
can query. Until a requested Type or system has enough facts, it resolves that
query to Invalid. Once resolution reaches the real Type, its Structured Layout
is a total reference. Layout therefore has no Incomplete state. An empty
Structured Layout may belong to a Terminal or a valid empty aggregate, so a
consumer proves Terminal before treating it as a scalar leaf.

The Package, Library, Foreign, Shader, Render, Scene, App, or another active ISA
completes the facts it owns using the pass structure appropriate to that
dialect. A host may enrich the reserved object, replace an enclosing resolver,
or build an immutable snapshot. Abstract, Type, Layout, and Callable prescribe
none of those mutation, snapshot, or pass-management policies.

Abstract determinism applies while the DAG is unchanged. The owner that changes
or replaces the DAG also owns reference lifetime, cache invalidation, and any
revision used by its readers. Process addresses can be local identity while
kept stable, but are never durable names.

Public export is a separate optional concern. A package host may validate and
archive an immutable public surface, while an interpreter or compiler may use
internal and transient Types that are never exported. Resolution, not
publication status, determines whether a semantic fact is currently available.

## Type References

Type references are progressive PascalCase Abstract queries whose final result
must prove the `Type` contract, with optional type arguments:

```ttx
Bits_32
Vec[Bits_8, 4]
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
operators. Numeric size arguments, such as the `4` in `Vec[Bits_8, 4]`, are part
of the type reference.

Parameterized types are not templates and do not generate code by themselves.
Resolution first builds the arguments, proves that the resolved object
implements `Generic`, then asks it for a concrete Type. `View[Bits_8]`,
`Vec[Real_32, 4]`, and `List[Graphics::Sprite]` are all the same operation. The
compiler owns the resulting concrete object and makes it queryable through the
same Type contract as any authored type.

Each Generic is a named formula registered in the current source context. `Vec`
owns Vec argument validation, materialization, and its concrete-Type cache.
`View` owns the corresponding View rules. The context owns name lookup and can
keep Generic formulas separate from concrete Types. There is no global formula
registry and no parser switch on formula names.

The evaluator constructs a closed ordered Argument sequence before calling the
formula. An Abstract argument stores its resolved identity. Bool and unsigned
arguments store their tagged values. Constants stay Abstract values and compare
by domain, resolved Type, and payload instead of collapsing to their result
Type. That complete sequence is the formula's cache key. Repeating the same
formula with an Alias, type-producing expression, or direct Type that resolves
to the same final object returns the same concrete Type identity. Unrelated
objects with the same local name remain distinct. Names, routes, parents, and
hashes do not participate. Missing formula lookup and a found formula rejecting
its argument shape remain distinct source errors, with both represented
semantically by Invalid.

Aliases are closed compile-time Abstract redirects. Alias preserves its local
name while `resolve()` follows the target's represented identity and
`resolve_context(route)` gives the complete route to the resolved target. If
that target is a Type, the consumer proves the Type contract after resolution.
The source declaration owns documentation beside the Alias. Alias cycles are
rejected during graph construction.

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

Core TTX defines `Terminal : Type` with direct byte-size and alignment queries,
then `Unsigned`, `Signed`, `Real`, and `Flag` as its standard semantic families.
It owns neither a prelude nor a supported-width registry. The active toolchain
constructs stable instances for the widths and formats it supports and installs
their names in its own Abstract resolution context. An unsupported name resolves
to Invalid through that context.

The current Perimortem C++ toolchain can register `Bool`, `Bits_8`, `Bits_16`,
`Bits_32`, `Bits_64`, `Signed_8`, `Signed_16`, `Signed_32`, `Signed_64`,
`Real_32`, `Real_64`, and `Real_128`. The `Bits_*` names are instances of
`Unsigned`, not a separate Bits contract. `Count` may Alias the registered
64-bit Unsigned instance, while `CppSize` may Alias the Unsigned instance for
the active C++ interface. `True` and `False` are Flag values rather than Types.

Terminal size and alignment are storage facts, not instruction mandates. A
one-byte Unsigned Type may use a wider register or move when its observable
semantics remain correct. Widths are instance data rather than C++ classes, so
another toolchain may register a different supported set without changing core
TTX or consulting a central type authority.

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
private bytes : Bytes;
```

This is one of the places where TTX is deliberately closer to IR than to a
general source language. The definition keyword tells the parser what kind of
semantic object is being created.

The active ISA may reject otherwise valid builtin forms. `object`
can be a legal builtin in a `Library` package while remaining invalid in a
`Shader` package.

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

Both parameters and returns are layouts. The selected body evaluator owns the
implementation block. The resulting semantic object implements `Callable` and
carries the callable signature, documentation, and dispatch name. A single type
may be written directly:

```ttx
public func size[] -> Count { ... }
```

Callable has two semantic subtypes. `Static` is selected through a type or package
name and does not consume a receiver. `Self` is selected through a runtime value
and requires that receiver as parameter zero:

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
An implementation or resolved external linkage may enrich either callable with
an Addressable object. Until linkage exists, the callable's address query returns an
explicit unresolved or Invalid Abstract, never a null pointer.

A named layout uses fields:

```ttx
public func decode[.source : View[Bytes]] -> [
  .ok : Bool,
  .image : Image,
] {
  ...
}
```

The Foreign ISA accepts exposed function declarations without bodies:

```ttx
private C : foreign {
  expose func inflate[.source : View[Bytes]] -> Bytes;
}
```

This is not a general `external` keyword. Forward declarations are not a TTX
feature. In a `foreign` context, a function without a body is an ABI promise
owned by that ISA.

## Packs

Parentheses always form a pack. Always.

```ttx
()                    // empty pack, Void
(value)               // one-element pack
(one, two)            // positional pack
(.ok = true, .value = 7) // named pack
(.10 = 50, .65 = 14)  // indexed pack
```

There is no separate grouping syntax. A one-element pack behaves as the one
value when the surrounding expression needs a value, so `(a + b) * c` still
works.

Packs are Abstracts for grouped value flow, not Expressions, Types, runtime
storage, or addressable objects. Their semantic identity lets evaluators,
tools, and lowering return and inspect a multi-value result before a receiving
Type exists. `Pack::get_layout()` exposes the identity-free fitting view.

The evaluator flattens nested positional Packs into one borrowed Abstract view
before constructing `Packs::Positional`. The Pack then owns one Fluid fitting
object and allocates no copied list. `Packs::Named` owns one Named fitting
object over actual named Abstracts in authored carrier order and does not
flatten. An authored field uses Binding when its name is a new edge to an
underlying Expression. Binding is not a binder, symbol table, declaration,
storage edge, or resolution phase. The evaluator rejects mixed pack modes
before construction.

The evaluator flattens nested positional packs before it constructs the outer
Pack. Grouping values with another pack does not create a nested runtime tuple:

```ttx
(1, (2, 3)) == (1, 2, 3)
```

Flattening is explicit. A typed object inside a pack remains one value. If code
wants to decompose it into a pack, it must swizzle or slice it:

```ttx
(screen_pos, 0.0, 1.0)       // Vec2D, Real_32, Real_32
(screen_pos.[x, y], 0.0, 1.0) // Real_32, Real_32, Real_32, Real_32
```

The Structured Layout supplies the selected Addressable facts but does not
evaluate the access. The active expression ISA creates Projections that retain
the receiver and selected Addressable. A Projection remains its own Expression
identity and publishes the field Type through `get_type()`. Swizzle and fixed
slice construct Positional Packs over those Projections. Joining constructs a
Positional Pack over already produced values and never implicitly deconstructs
a Structured typed value. Swizzle, Slice, and Join remain evaluator operations
instead of shared model subclasses until a unique query requires one.

That gives TTX three explicit ways to make packs:

- grouping values with `(...)`
- swizzling fields with `value.[x, y]`
- slicing constant-evaluated ranges with `value:[start, count]`.

`Vec`, `Vec2D`, `Vec3D`, `Vec4D`, graphics color values, and user structs are
typed values, not pack aliases. They may be initialized from compatible packs,
but they do not implicitly splat back into packs.

An active toolchain may provide the small vector types as top-level Types with
concrete `Real_32` fields:

```ttx
Vec2D : struct { x : Real_32; y : Real_32; }
Vec3D : struct { x : Real_32; y : Real_32; z : Real_32; }
Vec4D : struct { x : Real_32; y : Real_32; z : Real_32; w : Real_32; }
```

Graphics color lives in `Perimortem.Graphics`, which is explicit:

```ttx
import Graphics : Package = Perimortem.Graphics;

Graphics::Color : struct { r : Real_32; g : Real_32; b : Real_32; a : Real_32; }
```

So `screen_pos.[x, y]` is not special pack syntax over an alias. It is a
swizzle over real fields on a real typed aggregate.

Pack order matters when a pack is passed, returned, or materialized. Names are
authored or boundary-provided facts. They do not erase the carrier order. A
named pack can initialize a struct out of declaration order because type and layout
fitting maps by name, then lowering writes the struct in declaration order:

```ttx
state thing : Thing = (
  .b = source_b,
  .a = source_a,
);
```

To repack values, build the new pack explicitly with grouping, swizzle, slice,
or named fields. Grouping, swizzle, and slice produce positional packs unless a
field explicitly authors a name:

```ttx
state position : Vec3D = (screen_pos.[x, y], z);
state color    : Vec4D = (sample.[r, g, b], alpha);
```

Indexed packs are for sparse data tables, especially fixed-size aggregates like
`Vec[T, N]` where most elements use defaults:

The shared v1 model does not add an Indexed Pack contract. The receiving Type
and evaluator validate the designators and expand them into complete positional
value flow or Invalid before core Layout fitting.

```ttx
private decode_table : Vec[Bits_8, 256] = (
  .43 = 62,
  .47 = 63,
  .65 = 0,
);
```

An indexed designator must be an explicit integer literal. It is not an
expression and does not accept character literals or named constants:

```ttx
(.'A' = 0)   // invalid
(.upper = 0) // invalid for Vec tables because this is a named field
(.40 + 3 = 62) // invalid
```

Pack modes cannot be mixed in the same pack:

```ttx
(.name = value, .other = 1) // valid
(value, other)             // valid
(.10 = value, .20 = other) // valid
(value, .other = 1)        // invalid
(.name = value, .10 = 1)   // invalid
```

This keeps pack fitting deterministic.

The first field determines the pack mode. A named pack starts with `.field`, an
indexed pack starts with `.integer`, and a positional pack starts with a value
expression. Later fields cannot change the mode.

## Layouts

Layouts describe the shape expected by a function, return, or loop value:

```ttx
Count
[]
[Bits_32, Bits_32]
[.x : Bits_32, .y : Bits_32]
[@builtin @slot(0) .source : View[Bytes], .count : Count]
```

The model uses three narrow contracts rather than one tagged record:

- `Fluid` carries ordered positional Abstract values.
- `Named` carries ordered, uniquely named Abstract values and fits by name.
- `Structured` is returned by Type and carries its actual Addressable objects.

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
every ISA repeat name matching. This operation does not allocate or create
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

`Pack` and `Layout` deliberately meet at only one seam. Pack is the queryable
semantic carrier for grouped flow. Layout is its identity-free shape and
fitting view. Concrete Packs own their Layout objects instead of inheriting
them. Expression dependency lists also use identity-free Layouts because an
operand list is not itself an authored multi-value result. This keeps grouped
flow queryable without making every Type Layout into an Abstract value.

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
remain ISA or compiler concerns. A future foldable expression can expose a
Constant without adding evaluation to every Expression.

Executable syntax is owned by the ISA that understands it. An ISA may attach
an owned executable body to a stable Callable, but that representation
does not add body tags or a generic statement hierarchy to the shared TTX
model.

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
Bits_32 -> from(value) -> bit_and(Bits_32 -> from(0xFF));
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
| `.field`          | field or package/type member access       |
| `:[index]`        | constant-evaluated index access           |
| `:[start, count]` | constant-evaluated index slice            |
| `.[a, b, c]`      | swizzle into a positional pack            |
| `-> name(pack)`   | callable dispatch from the left-side base |

Calls take a pack because call arguments are written with `(...)`, and `(...)`
is always a pack. `.` is lookup only. `->` marks every call. A type or package
receiver resolves a `Static` callable. An addressable value resolves a `Self`
callable from its resolved Type and contributes the declared receiver argument.
Function-pointer dispatch is Self dispatch on the callable value, such as
`callback -> invoke(args)`.

The `:[...]` operands must evaluate to Unsigned Constants. This form performs
compile-time selection and produces a projection or positional pack. Runtime
slicing is ordinary Self dispatch such as `value -> slice(start, count)`. It
returns one View-like typed value and will be supported by the receiver's
Indexable contract rather than by weakening the compile-time slice rule.

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
private values : Vec[Bits_32, 4] = (1, 2, 3, 4);

private quad_uvs : Vec[Vec2D, 6] = (
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

The left side must be an assignable access chain. Valid roots are:

```ttx
name
self.field
```

`self` is only an assignment root when it has an access suffix. Bare
`self = value;` is invalid. A package is a top-level Type, not a lowercase
pseudo-root. Package-owned state is reached through an ordinary bound package
or Type context.

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
directives such as `@if` may also spawn scopes when an ISA chooses to own them.
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
private Color : enum[Bits_8] {
  red = 1;
  green = 2;
  blue = 3;
}
```

The compiler treats enum members as exposed compile-time values inside the enum
namespace and checks each value against the declared storage type. The meaning
is equivalent to:

```ttx
private Color : struct {
  expose red   : Bits_8 = 1;
  expose green : Bits_8 = 2;
  expose blue  : Bits_8 = 3;
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
private signature : Vec[Bits_8, 8] = 0x[89 50 4E 47];
```

Documentation is part of the TTX model because the formatter and LSP need it.
It is not part of type identity, layout equivalence, or layout fitting.
It is also not folded into identity resolution. The declaration that introduces
an Alias may document why the target is being used in that context, while the
resolved target keeps its own documentation. An LSP hover can choose source,
resolved, or stacked presentation without storing prose on Alias or changing
type equivalence.

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
