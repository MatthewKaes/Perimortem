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
token bytecode, and Type and Layout model are intentionally reusable outside
that toolchain.

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

One of the central rules is the difference between fluid value flow and
concrete storage. A pack such as `(a, b, c)` is fluid: it can be flattened,
named, fitted, or repacked before it becomes a real value. A struct, vector,
color, scalar, or ABI block is concrete: it has stable order, offsets,
alignment, and addressable identity. TTX lets fluid packs initialize concrete
values when the shape fits, but it does not silently unpack concrete values
back into packs. Source uses swizzle or slice syntax when it wants to cross that
boundary explicitly.

## Relationship To LLVM IR And MLIR

The short version is that LLVM IR, MLIR, and TTX all live in the IR family, but
they optimize for different moments in the toolchain:

| Area                   | LLVM IR                                             | MLIR                                                          | TTX                                                          |
| ---------------------- | --------------------------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------ |
| Primary representation | Lowered SSA module/function/block/instruction IR    | Extensible operation/SSA IR with regions                      | Source IR plus token bytecode and context layers             |
| Main extension point   | Intrinsics, metadata, passes, and targets           | Dialects define operations, types, attributes, and interfaces | Fixed syntax; ISAs define evaluation, legality, and metadata |
| Typical motion         | Optimize and transform already-lowered IR           | Rewrite and convert operations between dialects               | Evaluate token bytecode and enrich queryable facts           |
| Text form              | Debug, test, and serialization form for compiler IR | Debug, test, and serialization form for multi-level IR        | Human-authored canonical source surface                      |
| Extension granularity  | Target and metadata oriented                        | Operations from many dialects can coexist freely              | One declared ISA controls the legal semantic world           |
| Source preservation    | Mostly lowered away before LLVM IR                  | Supported through locations and higher-level dialects         | Central design constraint                                    |
| Tooling goal           | Optimizer and code-generation substrate             | Reusable compiler infrastructure                              | Shared frontend IR for compiler, editor, and build tooling   |
| Lowering               | Already lowered enough for optimization             | Core workflow through dialect conversion                      | Delayed until terminal artifacts                             |

TTX is closer to a human-editable source IR than to C++ with different
punctuation. It shares LLVM's bias toward explicit values, typed calls, and
visible memory shape, but it keeps source-level concepts alive much longer than
LLVM IR would.

The source format is inspired by LLVM concepts, but TTX does not globally define
every construct that might lower to LLVM-like IR. Lexical lowers authored source
into token bytecode. A host selects an evaluator for the relevant bytecode span.
That evaluator decides which token spans are executable instructions, what facts
they publish, and whether those facts can later lower to LLVM IR, SPIR-V, object
code, editor data, or something else.

That makes the reusable TTX model smaller than a full language tree:

| TTX data or token shape                | Usually becomes                                               |
| -------------------------------------- | ------------------------------------------------------------- |
| `Ttx::Type`                            | type identity, alias identity, namespace-like type lookup     |
| `Ttx::Layout`                          | value shape, field order, pack fitting, member projection     |
| `Documentation`                        | source-authored prose for tools and exported facts            |
| `TypeAccessOp` such as `::`            | nested type query against the current type or import context  |
| `AddressOp` such as `.`                | layout member query or ISA-owned address projection           |
| `CallOp` such as `->`                  | callable dispatch query against the current type or ISA facts |
| modifier and attribute token classes   | visibility, storage, package, or ISA-owned metadata           |
| string, bytes, layout, and pack tokens | source-shaped operands for the active ISA                     |

Library, Package, Shader, Render, and future ISAs then decide what larger source
forms mean. A Library ISA may define functions, control flow, and local storage.
A Shader ISA may define stage metadata and GPU legality. A Package ISA may turn
exports into type facts. The syntax gives those ISAs a shared instruction stream
and a shared Type and Layout model, but the ISA owns the meaning of its body.

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

It should be noted that so far TTX has ***not*** found a use case for significant
whitespace, but it reserves the right to use it in the future. This means
formatted outputs by the LSP should be considered the stable source format.

## Source Envelopes and ISAs

Many TTX hosts use a source envelope so a file can declare which ISA should
evaluate its body:

```ttx
dialect : Library;
dialect : Render;
dialect : Shader;
dialect : Entity;
```

The source keyword remains `dialect`, but semantically this instruction names an
evaluator installed in the active host. Tetrodotoxin implements this convention
with a direct Boot ISA call and an `Isa::Registry`, but that registry is a
Tetrodotoxin toolchain detail. An ISA is not a separate lowered IR stage. It is
the instruction set that owns the next bytecode span and may publish or consume
`Ttx::Type`, `Ttx::Layout`, or other host facts.

The lowercase `dialect` marker is a reserved keyword. The ISA name after
the colon is still a PascalCase type atom, so names such as `Package`,
`Library`, and `Shader` remain valid in type-access expressions like
`YourType::Package`.

In TTX, definition keywords describe how a scoped sub-IR should be treated.
`object` and `struct` are used inside a package:

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

`Library`, `Render`, `Shader`, and `Entity` are ISAs. An ISA is
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
import Graphics : Package = Perimortem::Graphics;
import Math     : Library = "math.ttx";
```

An import reads like a special definition:

```ttx
import Alias : IsaName = source;
```

The left side creates the local name. The ISA name describes what the host
expects the target source to declare. The right side is either a source path,
such as `"math.ttx"`, or a package name such as `Perimortem::Graphics`.

Imports do not import ISA semantics. A `Shader` package does not inherit
managed-library semantics like `object` or `List` by importing a `Library`.
Instead the imported definitions are queried through the importing ISA's rules
after canonicalization.

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
builtin such as `struct`, `alias`, `foreign`, `Package`, or `Namespace`.

The shared definition parser only extracts the shape: modifier, name,
`Define`, and qualifier. The active ISA then decides whether the modifier and
qualifier are legal, whether the body opens a scope, and what TTX facts are
published.

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

| Modifier  | Intended meaning                                      |
| --------- | ----------------------------------------------------- |
| `public`  | visible API that other sources may read or call       |
| `private` | local implementation detail owned by the current ISA  |
| `expose`  | externally readable data, written by its owner        |
| `state`   | stateful storage that is not part of the value shape  |
| `const`   | write-once or compile-time data                       |

The tokenizer only provides the keyword class. Library, Package, Shader, and
future ISAs decide which modifiers are legal at each instruction and what facts
they publish into the TTX data model.

## Attributes And Directives

Attributes are compiler directives attached to the next member, parameter, or
field:

```ttx
@builtin(.slot = 0) .source : View[Bytes]
@packed
@stage(.kind = "fragment")
```

Attributes may take a pack. Since `(...)` is always a pack, attribute arguments
use the same syntax as call arguments and aggregate values.

`@if` is an attribute-shaped directive:

```ttx
@if(.enabled = true) {
  state generated : Count = 1;
}
```

The disabled marker `/>` is source-level sugar for disabling the following
member. It is intended for hand-authored experiments and formatter-preserving
comments, not as a semantic feature.

## Type References

Type references are progressive PascalCase queries with optional type
arguments:

```ttx
Bits_32
Vec[Bits_8, 4]
Graphics::Image
Math::Matrix[Real_32, 4, 4]
```

The first type token resolves from the current context. `::` then asks that
resolved type or package for one nested type at a time. It is not string
concatenation, and a compiler should not allocate a general path object just to
answer `Graphics::Color`.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric size arguments, such as the `4` in `Vec[Bits_8, 4]`, are part
of the type reference.

Parameterized types are not templates and do not generate code by themselves.
Resolution first builds the argument layout, then asks the type object being
parameterized to return the
concrete type identity for that layout. `View[Bits_8]`, `Vec[Real_32, 4]`, and
`List[Graphics::Sprite]` are all the same operation: type dispatch over a
resolved layout. The returned concrete type is what later member, nested type,
function, and layout queries use.

Aliases are compile-time type references. Resolution canonicalizes aliases
inside a package so later compiler stages can reason about resolved types
instead of source spelling.

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
node is being created.

The active ISA may reject otherwise valid builtin forms. `object`
can be a legal builtin in a `Library` package while remaining invalid in a
`Shader` package.

## Functions

Functions are explicit AST nodes, not values declared as `Func`.

```ttx
public func main[.frag_uv : Vec2D] -> Color {
  state s : Color = self.icon_texture -> sample(frag_uv);
  return (s.[r, g, b], s.a * Push.alpha);
}
```

The function syntax is:

```ttx
modifier? func name[params] -> returns block
```

Both parameters and returns are layouts. A single type may be written directly:

```ttx
public func size[] -> Count { ... }
```

A named layout uses fields:

```ttx
public func decode[.source : View[Bytes]] -> [
  .ok : Bool,
  .image : Image,
] {
  ...
}
```

Functions may be declared `external` only inside `foreign` blocks:

```ttx
private C : foreign {
  external private func inflate[.source : View[Bytes]] -> Bytes;
}
```

Forward declarations are not a TTX feature. A function without a body means
external ABI, and that meaning is only valid in `foreign`.

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

Packs are intermediate aggregate expressions, not concrete runtime objects.
Typed values consume packs when the surrounding declaration, call, return, or
assignment supplies a target type.

Nested positional packs flatten when they are fitted to a pack-compatible
target. Grouping values with another pack does not create a nested runtime
tuple:

```ttx
(1, (2, 3)) == (1, 2, 3)
```

Flattening is explicit. A typed object inside a pack remains one value. If code
wants to decompose it into a pack, it must swizzle or slice it:

```ttx
(screen_pos, 0.0, 1.0)       // Vec2D, Real_32, Real_32
(screen_pos.[x, y], 0.0, 1.0) // Real_32, Real_32, Real_32, Real_32
```

That gives TTX three explicit ways to make packs:

- grouping values with `(...)`
- swizzling fields with `value.[x, y]`
- slicing ranges with `value:[start, count]`.

`Vec`, `Vec2D`, `Vec3D`, `Vec4D`, graphics color values, and user structs are
typed values, not pack aliases. They may be initialized from compatible packs,
but they do not implicitly splat back into packs.

The small vector types are provided by the implicit core prelude as top-level
types with concrete `Real_32` fields:

```ttx
Vec2D : struct { x : Real_32; y : Real_32; }
Vec3D : struct { x : Real_32; y : Real_32; z : Real_32; }
Vec4D : struct { x : Real_32; y : Real_32; z : Real_32; w : Real_32; }
```

Graphics color lives in `Perimortem::Graphics`, which is explicit:

```ttx
import Graphics : Package = Perimortem::Graphics;

Graphics::Color : struct { r : Real_32; g : Real_32; b : Real_32; a : Real_32; }
```

So `screen_pos.[x, y]` is not special pack syntax over an alias; it is a
swizzle over real fields on a real typed aggregate.

Pack order matters when a pack is passed, returned, or materialized. Names are
authored or boundary-provided facts; they do not erase the carrier order. A
named pack can initialize a struct out of declaration order because type and layout
fitting maps by name, then lowering writes the struct in declaration order:

```ttx
state thing : Thing = (
  .c = source_c,
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
(.upper = 0) // invalid for Vec tables; named field, not index 65
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
[@builtin(.slot = 0) .source : View[Bytes], .count : Count]
```

A named layout field starts with `.` and an addressable name. Attributes may
decorate layout fields, but the field marker remains visible before the field
body. An unnamed layout is just a list of type references.

`for` uses a layout because iteration can bind more than one value at a time:

```ttx
for [.i : Count] in 0...count {
  ...
}

for [.one : Count, .two : Count] in 0...100 {
  ...
}
```

The second form consumes values two at a time. Conceptually, the layout's size
defines the stride through the packed iterable.

## Expressions

Expressions are values. Assignment is not an expression in TTX because it does
not produce a value.

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
package.version
self.texture
source:[0]
source:[0, 4]
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
| `:[index]`        | index access                              |
| `:[start, count]` | index slice                               |
| `.[a, b, c]`      | swizzle into a positional pack            |
| `-> name(pack)`   | callable dispatch from the left-side base |

Calls take a pack because call arguments are written with `(...)`, and `(...)`
is always a pack. `.` is lookup only. `->` marks every call, including receiver
calls, type/static calls, package calls, and function-pointer dispatch such as
`callback -> invoke(args)`.

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
package.name
```

`self` and `package` are only assignment roots when they have an access suffix.
Bare `self = value;` and bare `package = value;` are invalid.

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

The compiler checks that the first packed value is truthable. Today that means
`Bool`; numeric truthiness is intentionally not implicit.

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

Enums use a concise storage-typed named-pack syntax:

```ttx
private Color : enum[Bits_8](.red = 1, .green = 2, .blue = 3);
```

The syntax is short because enums are common compile-time data. Semantically,
the compiler desugars enum members into exposed compile-time values inside the
enum namespace, and checks each value against the declared storage type:

```ttx
private Color : enum[Bits_8](
  .red = 1,
  .green = 2,
  .blue = 3,
);
```

is treated like:

```ttx
private Color : struct {
  expose red   : Bits_8 = 1;
  expose green : Bits_8 = 2;
  expose blue  : Bits_8 = 3;
}
```

The internal representation is compiler-owned, but the source storage type is
part of the contract. Enum members are named compile-time values, not runtime
fields. `enum[Storage](...)` is an enum declaration shorthand, not a general
expression form for creating enum values inline.

## Literals

TTX literals are intentionally small:

```ttx
42
0xFF
0.5
"text"
0x[89 50 4E 47]
$[path/to/file]
true
false
```

`0x[...]` is a byte literal. `$[...]` embeds a file as data. Strings do not
include an implicit null terminator.

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
It is also not canonicalized with aliases. An alias may document why a type is
being used in a package or context, while the root type keeps its own
documentation. An LSP hover can choose alias documentation, canonical
documentation, or a stacked presentation without changing type equivalence.

## Why The Grammar Is Small

The reference grammar in `syntax/ttx.g4` is compact because many apparent
language features are the same concept:

- Calls, attributes, aggregate values, `if`, and `while` all use packs.
- Function parameters, returns, and `for` bindings all use layouts.
- Field access, swizzle, index, slice, and method call are all postfix access.
- Visibility and storage are modifier tokens.
- Type paths and aliases are type references.
- Assignment is a statement, not an expression.

The result is a language that can grow compiler power without growing many
syntactic special cases.
