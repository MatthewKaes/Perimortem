# TTX Language Semantics

TTX is Tetrodotoxin's default frontend IR language. It is authored by humans, but
it stays close to the compiler's semantic model. A TTX file makes package
boundaries, storage shape, addressability, layout, and lowering intent visible
in the source. Tetrodotoxin can support other frontend languages, but TTX is the
frontend designed around Perimortem's authoring, tooling, shader, ABI, and build
needs.

For high-level language design and usage, see
[ttx_design.md](ttx_design.md). This document is the single source of truth for
TTX semantics and is sufficient to build a conforming TTX parser from scratch.
It explains how each language concept is tokenized, parsed, represented, and
validated before lowering.

TTX's central design philosophy is to provide as much of a human-editable
surface as possible without giving up the advantages of an IR. The source is
pleasant to read and review, but it is also extremely cheap and reliable to
parse. A practical way to state the design goal is: TTX is an IR-like language
where humans edit semantic structure directly without drowning in compiler
machinery.

The compiler architecture follows **monotonic context layering**. Each stage
enriches the same conceptual program representation with queryable context
instead of throwing that representation away and replacing it with a new private
IR. A tool can stop as soon as it has the context it needs: syntax highlighting
can stop after lexical classification, formatting can stop after syntax,
project navigation can stop after resolution, and code generation continues
through type/layout validation queries and compilation. Fast compilation is part of that goal, not an
afterthought. Every syntax feature must justify its parse cost, recovery cost,
and downstream context cost.

To make that work, the syntax carries a large amount of semantic information
directly. Casing separates addressable names from type names. Sigils encode
visibility and compile-time addressability. Builtin words such as `Struct`,
`Enum`, and `Foreign` are token classes rather than ordinary identifiers. Packs,
layouts, access chains, and assignment statements all have distinct local
shapes.

Each major section below describes the source form, the token or local shape
that starts the parse, the AST meaning, and the validation rules attached to
that shape. The exact C++ implementation can change, but these contracts remain
stable.

## Formal Language Model

TTX has two useful formal layers:

- The token stream and syntactic grammar are deterministic context-free after
  lexical analysis. The grammar is designed for predictive recursive descent
  with bounded fixed lookahead, plus an operator-precedence parser for
  expressions.
- The set of semantically valid TTX programs is context-sensitive. Name
  resolution, import resolution, type canonicalization, package dialect rules,
  pack fitting, and addressability checks all depend on program context.

TTX is not trying to be a maximally expressive context-free grammar. It is a
context-sensitive source IR whose concrete grammar is engineered to be
deterministic and bounded-lookahead. Its main formal trick is moving semantic
category information into lexical and syntactic form so the parser can build a
source-shaped AST without speculative parsing or later reinterpretation.

In practical parser terms, TTX is LL-like rather than a pure LL(1) grammar.
Several local decisions use fixed lookahead, such as distinguishing named and
unnamed layouts after `[`. Expressions are not written as left-recursive grammar
productions. They are parsed by precedence so recursive descent never needs to
expand an expression before consuming input.

The grammar is intentionally left-factored around visible source markers. A
parser consumes the shared prefix of a construct once, then branches on the next
token that actually distinguishes the alternatives. The syntax does not require
backtracking, speculative parsing, speculative diagnostics suppression, or
reinterpretation of an already-parsed subtree.

## Monotonic Context Layering

Tetrodotoxin is a multi-phase IR-to-IR toolchain, but its phases are not
intended to be a chain of unrelated replacement IRs. When the frontend language
is TTX, the durable object is the source program with progressively enriched
context.

```text
authoring surface
-> lexical context
-> syntax and scope context
-> cross-file resolution context
-> type/layout validation context
-> compilation context
-> terminal output
```

The rule is monotonic: a stage can add facts, validate invariants, cache derived
answers, or expose a richer query surface. It does not reinterpret earlier
facts, silently discard source shape, or copy dialect-specific facts into a
parallel primary representation. A new standalone representation is appropriate
only when the pipeline intentionally crosses a terminal boundary, such as
emitting SPIR-V words, LLVM IR, a C header, an object file, an archive, JSON for
an editor, or formatted source text.

The practical rule for moving information left is:

> Push stable, local, syntax-visible semantics left. Keep non-local facts in
> explicit queryable context layers.

The lexer classifies stable source spellings such as type-shaped names,
addressable names, sigils, builtin type forms, and fixed operators. The parser
builds local scope and source shape without needing imports. Resolution adds
package graph and cross-file identity. Type/layout validation queries check how
resolved nodes fit local type and layout expectations. Dialects own
package-specific legality and metadata. Compilation asks those contexts
questions rather than rediscovering source intent.

This model is close to typed AST and attribute-grammar systems, and it also
resembles query-based incremental compilers. The important distinction is
ownership:

- Source and package records own source text and arena lifetime.
- Lexical owns token classification.
- Syntax nodes own source shape, comments, local declarations, scopes, layouts,
  packs, statements, and expressions.
- Resolution owns package records, import graph state, cross-file identity, and
  invalidation when files change.
- Type/layout validation queries expose derived answers such as expression type,
  pack-fit result, selected callable, assignment compatibility, and return
  compatibility. These queries use resolution, syntax, TTX package facts, and
  dialect context rather than a standalone semantic IR.
- Dialects own dialect-specific interpretations such as shader stages,
  descriptor bindings, push constants, builtins, and target legality.
- Compilation owns backend lowering state and terminal-oriented intermediate
  artifacts.

## Layout Facts

TTX uses layout as the shared fact for value shape and storage shape. Scalars,
vectors, colors, structs, function parameters, returns, packs, and swizzles all
bottom out in layout questions: what entries exist, what names they have, what
types they carry, and whether the result has concrete storage.

There are two important layout kinds:

- **Fluid layouts** describe value flow. Packs, grouped values, function
  argument packs, and intermediate return packs are fluid. They can flatten
  nested positional groups, acquire or discard names when fitting rules allow
  it, and be repacked before they materialize. A fluid layout has no stable
  address of its own.
- **Concrete layouts** describe materialized storage. Scalars, structs,
  `Vec2D`, `Vec3D`, `Vec4D`, `Color`, ABI blocks, and other typed values have
  concrete layouts. They have stable size, alignment, entry order, offsets, and
  ABI or wire-format meaning.

A concrete layout may contain entries. Each entry has an optional name, a child
type, and an offset computed under the parent layout policy. A scalar is the
terminal case: a concrete layout with no entries. A struct or vector is the
same fact shape with entries.

Methods are not layout entries. A layout describes shape and storage; a type or
scope owns dispatch. Method signatures may reference layouts for their argument
and result shapes, but method lookup belongs to the concrete typed receiver or
package scope.

The core invariant is:

> Fluid layouts describe value flow. Concrete layouts describe storage. TTX may
> reshape value flow, but it does not silently reshape storage.

This is why a pack can initialize a struct by name while an existing struct
does not implicitly decompose back into a pack. Source must use swizzle or
slice syntax to cross from concrete storage back into fluid value flow.

Callable dispatch also requires a concrete receiver. The `->` operator is
valid on concrete typed values, concrete type names, or concrete package scopes.
It is not valid on a raw fluid pack because a fluid pack has no stable receiver
identity. Its entries can still be mapped, fitted, reordered, or materialized by
type/layout fitting, but they are not addressable storage until a concrete target is known.

Implementations prefer queryable views over copied summaries. For example, a
shader backend asks a shader dialect context for `stage_of(type)`,
`descriptor_of(member)`, and `is_push_constant(type)`, and asks the type/layout
query surface for `type_of(expression)` or `fit_of(pack)`. It does not need a separate
shader-specific clone of the package AST before it starts lowering.

## Compiler Architecture

A complete TTX implementation has an authoring surface followed by six compiler
layers:

```text
authoring surface -> lexical -> syntax -> resolution -> compilation -> generation
```

These pass names are deliberate. TTX does not have a broad semantic pass whose
job is to reinterpret ambiguous syntax after parsing. The source already carries
the semantic category of each construct. The later passes connect,
type-check, and lower already-shaped nodes by enriching the available context.

Type/layout validation is not a separate replacement IR layer. It is a set of
query surfaces, owned by resolution, syntax, standard TTX providers, dialects,
and eventually ABI providers, that validates resolved names, expressions,
packs, layouts, and expected types. These queries may reject a program and may
cache concrete facts on or beside syntax nodes. They do not guess which
namespace a name belongs to, choose between ambiguous parse trees, reinterpret
syntax after the fact, or collect backend-specific metadata into a second source
of truth.

The stages are:

1. **Lexical**: tokenize source into meaningful lexical classes. The tokenizer
   distinguishes addressables, type names, package kinds, builtin type forms,
   sigils, attributes, byte literals, embedded file literals, and operators.
2. **Syntax**: parse each package into a source-shaped AST. The syntax pass owns
   package structure, imports, type bodies, members, functions, attributes,
   statements, expressions, local shape validation, formatting shape, and
   deterministic recovery.
3. **Resolution**: build the package graph and bind references between parsed
   ASTs. The resolver checks whether imported packages exist, whether imported
   dialects match the declared import kind, whether exported names exist in the
   namespace selected by syntax, and whether type aliases canonicalize to
   concrete type references.
4. **Compilation**: lower resolved syntax using type/layout validation and
   dialect contexts into the backend's intermediate and machine-level
   representation. This pass consumes resolved, type/layout, and dialect facts through context queries
   rather than rediscovering them from source text.
5. **Generation**: emit the requested output format. Shader binaries, embedded
   read-only data, foreign ABI tables, generated host constants, relocation
   records, objects, archives, and other final artifacts belong here.

Resolution and type/layout validation are intentionally narrower than a traditional semantic
pass. For example, `Graphics.Image` is syntactically a type path,
`Graphics.image` is a member access, and `Graphics -> image()` is a function
dispatch. The resolver checks existence, visibility, import graph reachability,
and dialect compatibility. It does not decide which namespace the source meant;
the syntax already selected that namespace. Type/layout fitting then checks whether the
resolved symbol can be used in the local type context.

Imports are graph edges between complete package ASTs. A compiler can parse an
imported source into its own AST, continue resolving that package's imports, and
stop when the graph bottoms out or reports a cycle that violates the relevant
resolution rule. This is different from C or C++ header inclusion: TTX does not
splice imported source text into the importing package, and a package can be
parsed into a meaningful local AST before any imported package is resolved.

The resolver owns this graph. Each loaded source file has a package record with
its own arena, source file snapshot, tokenizer output, parsed AST, and resolution
state. If the file changes, the resolver clears only that package record's arena
and reparses that one AST. Packages that import it can keep their syntax ASTs;
their resolution state is invalidated because the graph facts they previously
bound against may have changed. This keeps syntax ownership local while giving
resolution a stable place to manage cross-package lifetime.

Type/layout validation caches are invalidated by resolution changes because they depend on
the concrete targets selected by the resolver. A package can keep its syntax AST
while re-running resolution and validation queries against updated imports. That keeps
the arena model simple: syntax remains owned by the package record, resolution
owns the package graph, and proof caches own only the derived typed facts for
the current graph snapshot.

Validation is successful when every expression has a concrete value type, every
pack has either fitted an expected shape or remains in a context that explicitly
allows a pack, every access chain has a typed result, every call has a selected
callable target, and every statement has the type facts needed for lowering.
After type/layout validation, compilation no longer needs source-level name lookup, pack
interpretation, or expression inference. It asks the enriched context for the
already-proven answers.

## Parser Contract

The parser turns the token stream into the source syntax tree that resolution
and validation queries expect. It preserves source shape, attaches local type and name
facts as soon as they are available, and never asks later stages to reconstruct
source intent from raw text.

The parser does not use rollback, checkpointing, or suppressed diagnostics to
choose between valid grammar paths. It parses each construct once and then
validates the resulting shape.

There are deterministic lookahead decisions, and those are intentional:

- `Layout::parse()` looks one token after `[` to distinguish a named layout
  field, an unnamed layout, an empty layout, or a direct value type.
- `Pack::parse_fields()` selects named-pack mode only from a leading `.` field.
  Any other expression-starting token selects positional-pack mode.
- `Member::parse()` looks for `sigil func` so function declarations do not look
  like ordinary value declarations.
- `Expression::parse_access_chain()` looks after `.` to decide whether there is
  a field access. If the next token is not a valid field name, postfix parsing
  stops.
- The expression parser looks after `+` so `+:` remains a slice operator rather
  than an addition followed by a definition token.
- Statement parsing parses an expression once. If the next token is an
  assignment operator, the parsed expression is checked to see whether it is an
  assignable address chain.

Those decisions are local and do not backtrack. This is important because TTX
is intended to have an authoritative formatter, useful diagnostics, and a
compiler pipeline that does not depend on hidden parser guesses.

A conforming parser follows these rules:

1. Let token classes carry the first layer of meaning.
2. Pick the next production from the current token whenever possible.
3. Treat named and positional aggregate syntax as disjoint modes. Named packs
   start with `.field`; named layouts start with `.field` or attributes followed
   by `.field`.
4. Use bounded lookahead only for local shape decisions, such as distinguishing
   a named layout from an unnamed layout.
5. Parse expressions once. If a surrounding construct needs an assignable
   expression, validate the parsed expression's AST shape.
6. Emit diagnostics at the point where a required delimiter, marker, or shape is
   missing, then advance predictably so parsing can continue.

## Lexical Classes

The tokenizer does more semantic work than a minimal lexer would. This is
intentional. The parser sees classes such as `Addressable`, `Type`, `Func`,
`Enum`, `@if`, `@public`, and `SwizzleOp` directly.

Important lexical distinctions:

| Source shape                | Token class             | Semantic meaning                                    |
| --------------------------- | ----------------------- | --------------------------------------------------- |
| `snake_case`                | `Addressable`           | runtime names, fields, functions, local values      |
| `PascalCase`                | `Type` or builtin token | type names, aliases, package/type paths             |
| `Enum`, `Struct`, `Foreign` | builtin type tokens     | definition forms and type references                |
| `public`, `@public`, etc.   | sigil tokens            | visibility plus runtime/compile-time addressability |
| `@name`                     | `Attribute`             | metadata attached to members, params, fields        |
| `@if`                       | `CompileIf`             | compile-time control-flow keyword                   |
| `break`, `continue`         | control keyword tokens  | loop-control statements                             |
| `0x[...]`                   | `Bytes`                 | byte data literal                                   |
| `$[...]`                    | `Embedded`              | embedded file literal                               |
| `_`                         | `Discard`               | wildcard or ignored value                           |

Fixed source spellings live in `Lexical::Class::get_source_text()`. The lexer
and formatter call into `Class` rather than duplicate strings for
keywords, operators, delimiters, byte literal prefixes, embedded literal
prefixes, and marker tokens.

## Packages And Dialects

A complete file starts with exactly one package declaration:

```ttx
package : Library;
package : Shader;
package : Entity;
```

Parser shape: a full-file parser starts at `Package`, requires `Define`, then
requires one of the package dialect tokens and `EndStatement`. Anything else on
the first line is a package-level syntax error. Subtree parsers used by tests
or tools may start below package level, but a source file is anchored by this
declaration.

The package kind selects the top-level dialect. A dialect controls which
builtins, attributes, types, address spaces, runtime features, and lowering
targets are legal in the package.

| Dialect   | Purpose                                                            |
| --------- | ------------------------------------------------------------------ |
| `Library` | general reusable code, binary formats, data transforms, host logic |
| `Shader`  | shader definitions and shader-specific host glue                   |
| `Entity`  | entity-oriented package data and behavior                          |

`Object`, `Struct`, `Enum`, `Foreign`, and `Shader` can appear as definition
builtins, but they are not all package dialects. For example:

```ttx
package : Library;

@hidden Header : Struct {
  @public width  : Bits_32;
  @public height : Bits_32;
}
```

The active package dialect may reject a construct that is syntactically valid.
A `Shader` package does not accept managed runtime concepts such as `Object` or
`List` unless the shader dialect explicitly defines how they lower.

A dialect is not necessarily a single backend. A shader package may produce GPU
code, such as SPIR-V, and host-side code that loads those constants, builds
pipeline layouts, and bridges them into the Perimortem runtime.

## Imports

Imports introduce explicit local aliases for package dependencies:

```ttx
import Graphics : Library = (.source = "graphics/image.ttx");
import Math     : Library = TTX.Math;
```

Parser shape: imports appear immediately after the package declaration and
before members. An import starts with `Import`, then a PascalCase local name,
`Define`, a package dialect, `Assign`, an import source, and `EndStatement`.
The import source is chosen from the current token: `PackingStart` means a
file-source pack, while `Type` means a package path.

The import shape is:

```ttx
import LocalName : PackageKind = source;
```

The source is either:

- a file-source pack, currently `(.source = "path.ttx")`
- a package/type path such as `TTX.Math`.

The local name participates in type paths and value access after import
resolution. Imports do not erase dialect boundaries. A `Shader` package cannot
make `Object` legal by importing a `Library` that contains objects. Imported
definitions must still be valid in the importing dialect.

There is no `using` or wildcard import syntax. Imports are named aliases so
source reviews and diagnostics can see package boundaries.

## Names And Type Paths

TTX uses casing as a semantic boundary:

- `snake_case` names are addressable runtime values or fields.
- `PascalCase` names are types, aliases, package names, or builtin type-like
  symbols.

Type references are PascalCase paths with optional type arguments:

```ttx
Bits_32
Vec[Bits_8, 4]
Graphics.Image
TTX.Math.Matrix[Real_32, 4, 4]
```

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric type arguments, such as the `4` in `Vec[Bits_8, 4]`, are
part of the type reference and are validated while proving the type reference.

`Alias` is a compile-time type path. After canonicalization, later passes
do not need to know whether a type was written directly or reached through an
alias.

Parser shape: a type reference starts with `Type`, a builtin type-like token
such as `Struct` or `Alias`, or a numeric token when parsing a numeric type
argument. Path segments are separated by `AddressOp`. Type arguments start with
`IndexStart`, contain type references separated by `PackingOp`, and end with
`IndexEnd`.

## Standard Package Aggregate Types

TTX treats the small vector and graphics color types as real typed aggregates,
not pack aliases. The compiler provides them through standard packages with
fixed field names and `Real_32` storage.

`TTX.Core` is the implicit prelude for universal scalar, vector, and memory
forms. Source may use these names without a package qualifier:

```ttx
Vec2D : Struct { x : Real_32; y : Real_32; }
Vec3D : Struct { x : Real_32; y : Real_32; z : Real_32; }
Vec4D : Struct { x : Real_32; y : Real_32; z : Real_32; w : Real_32; }
```

`TTX.Graphics` is explicit. A package that needs graphics-domain types imports
it and refers to those types through the import name:

```ttx
import Graphics : Library = TTX.Graphics;

Graphics.Color : Struct {
  r : Real_32;
  g : Real_32;
  b : Real_32;
  a : Real_32;
}
```

These declarations are conceptual standard-package definitions rather than
source that appears in every file, but all later stages must behave as if the
fields are present. Member lookup, swizzle validation, pack fitting,
host ABI metadata, and shader lowering all use these same field definitions.

`Vec[T, N]` is different. It is a fixed-size homogeneous aggregate indexed by
position, so indexed packs may initialize sparse entries. `Vec2D`, `Vec3D`,
`Vec4D`, and `Graphics.Color` have named component fields and support named
packs and named swizzles through those fields.

## Definitions

The core definition forms are:

```ttx
sigil name : Type::Ref;
sigil name : Type::Ref = value;
```

Parser shape: a definition starts after an optional sigil. The next token must
be `Addressable` or `Type`. `Define` selects explicit type parsing. After the
type reference, `Assign` introduces an initializer. Otherwise the definition
must end with `EndStatement` or open a scoped builtin body.

TTX does not have an inferred declaration operator. Every declaration writes
its type at the declaration site so the parser can attach type information to
the AST before expression analysis.

Definitions introduce either addressable values or type-like names depending
on the name and the right-hand type reference:

```ttx
@hidden Header : Struct { ... }                     // type definition
@hidden header : Header = (.width = 4, .height = 2); // runtime value
@hidden CountAlias : Alias = Count;                 // compile-time alias-like definition
```

Only builtin definition kinds may open a scope:

```ttx
@hidden Data    : Struct  { ... }
@hidden Runtime : Object  { ... }
@hidden C       : Foreign { ... }
@hidden Stage   : Shader  { ... }
```

Other type references define values and must end with `;` or use `=`:

```ttx
@hidden size  : Count = 4;
@hidden bytes : Bytes;
```

The parser reads these shapes and dialect/type validation queries check which builtin kinds are
legal for the active package dialect, along with the compatibility of the
definition name, type, initializer, visibility, and attributes.

## Sigils

Sigils combine visibility with runtime addressability. Runtime sigils create
values that exist at a runtime address. `@` sigils create compile-time values
that the compiler may fold, inline, remove, specialize, or encode as metadata.

Parser shape: sigils are token classes, not attributes. A parser never parses
`@public` as `Attribute("public")`. Anywhere a sigil is allowed, check the
token class with the sigil predicate and store the class on the AST node.

| Sigil     | Visibility and lifetime                 | Runtime addressable? |
| --------- | --------------------------------------- | -------------------- |
| `public`  | public read/write API                   | yes                  |
| `@public` | public compile-time API                 | no                   |
| `expose`  | externally readable/runtime-exposed API | yes                  |
| `@expose` | externally visible compile-time data    | no                   |
| `hidden`  | private implementation data             | yes                  |
| `@hidden` | private compile-time data               | no                   |
| `stack`   | local runtime data                      | yes, within scope    |
| `@stack`  | local compile-time data                 | no                   |

The `@` prefix is not merely style and is not equivalent to an attribute. It is
a semantic bit carried by the token class.

This replaces older ideas such as `frozen`, `detail`, `@const`, and
`@comptime`. The language has fewer words, while `Class::Type` carries the
distinction exactly.

## Attributes And Directives

Attributes attach compiler metadata to members, layout fields, and parameters:

```ttx
@builtin(.slot = 0) .source : View[Bytes]
@stage(.kind = "fragment")
@binding(.set = 0, .slot = 1)
```

Parser shape: an attribute starts with `Attribute`, whose token text includes
the leading `@`. If the next token is `PackingStart`, parse a pack as the
attribute argument list. Otherwise the attribute has no arguments.

Attribute arguments are packs. Named and positional fields must not be mixed,
and the syntax attribute registry validates the expected keys and legal targets
for each known attribute.

Attributes are not runtime values. They are consumed by the compiler or
forwarded into target metadata.

Known shader ABI attributes have fixed local targets. The compiler keeps these
rules in `syntax/attribute` so attribute legality stays separate from package
shape parsing and later name binding:

- `@stage(.kind = "...")` applies to `Shader` definitions in `Shader` packages.
- `@push_constant` applies to exposed `Foreign` ABI blocks in `Shader`
  packages.
- `@binding(.set = N, .slot = N)` applies to resource members inside `Shader`
  scopes in `Shader` packages.
- `@builtin(.name = "...")` and `@builtin(.slot = N)` apply to shader builtin
  input members or layout fields.

Other attributes may remain target-specific metadata until a dialect defines
their validation rules.

`@if` is a compile-time control-flow keyword, not a normal attribute named
`if`:

```ttx
@if(.enabled = true) {
  @stack generated : Count = 1;
}
```

The tokenizer gives `@if` its own class, so the parser treats it as a scope
statement without consulting the attribute registry.

## Disabled Members

The disabled marker `/>` wraps the following member as disabled source:

```ttx
/> @hidden experimental : Count = 1;
```

Parser shape: `Disabled` is accepted before a member's attributes and
declaration. It is a member modifier, not a statement and not an expression.

It is intended for hand-authored experiments and formatter-preserving comments.
Semantically, disabled members do not participate in normal compilation. The
parser stores the disabled flag on the member so later stages can decide
whether to ignore it, preserve it for formatting, or lower it into a
compile-time-false construct.

## Functions

Functions are explicit syntax:

```ttx
@public func name[params] -> returns {
  ...
}
```

Parser shape: a function member starts with either `Func` or `sigil Func`.
`External` may appear before the function only in a `Foreign` scope. After
`Func`, require an `Addressable` function name, parse a parameter layout, require
`CallOp`, parse a return layout, then parse either a block or `EndStatement`.

`func` is a keyword because functions are not merely ordinary values with type
`Func`. They need a stable AST node for ABI lowering, entry-point discovery,
specialization, foreign declarations, and shader compilation.

Both parameters and returns are layouts:

```ttx
@hidden func size[] -> Count;

@public func decode[.source : View[Bytes]] -> [
  .ok : Bool,
  .image : Image,
] {
  ...
}
```

The return side is called `returns` in the AST because it is a layout, not a
separate grammar family.

The return layout is also the return carrier contract. A named return layout
preserves both the field names and their declared order. Callers may bind that
result into another named layout or aggregate if the names and types fit, but
the call boundary itself uses the function's declared return order.

### External Functions

`external` marks a function declaration without a body:

```ttx
@hidden C : Foreign {
  external @hidden func inflate[.source : View[Bytes]] -> Bytes;
}
```

External functions are only valid inside `Foreign` blocks. TTX does not have
ordinary forward declarations. A function without a body means an ABI promise,
and the linker or foreign ABI provider must resolve it.

## Members And Scoped Builtins

Package-level and scoped definitions are represented as members. A member may
have documentation comments, an optional disabled marker, attributes, a
function form, a scoped builtin body, an enum shorthand, or a value definition.

Parser shape: member parsing proceeds in layers: consume documentation comments,
consume an optional disabled marker, consume attributes, handle optional
`External`, then choose between function syntax and definition syntax. If the
definition's type reference starts with `Enum`, parse enum members. If it starts
with a scoped builtin such as `Struct`, parse a member scope. Otherwise parse a
value definition.

Scoped builtins own member lists:

```ttx
@hidden Header : Struct {
  @public width  : Bits_32;
  @public height : Bits_32;
}

@hidden C : Foreign {
  external @hidden func inflate[.source : View[Bytes]] -> Bytes;
}
```

The parser identifies scoped builtins by the first segment of the definition's
type reference. Dialect/type validation queries check that the builtin is legal in the current scope
and dialect.

## Enums

Enums use a storage-typed named-pack declaration shorthand:

```ttx
@hidden Color : Enum[Bits_8](.red = 1, .green = 2, .blue = 3);
```

Parser shape: after `Define Enum`, require exactly one type argument naming the
enum storage type. Then parse a normal pack and require that every field is
named. The enum declaration ends with `EndStatement`. It does not open a brace
scope in source. `Enum[Storage](...)` is only a declaration form, not an
expression form for creating enum values inline.

The pack must be named. Positional enum values are invalid because enum member
names are the purpose of the construct.

Semantically, enum members are compile-time exposed values inside the enum
namespace. Each member value must fit the declared storage type. That makes the
storage width part of the source contract and lets semantic validation reject
out-of-range values before lowering. The compiler may store enum metadata in
whatever internal representation is best, but the meaning is equivalent to:

```ttx
@hidden Color : Enum[Bits_8](
  .red = 1,
  .green = 2,
  .blue = 3,
);
```

producing members that behave like:

```ttx
@expose red   : Bits_8 = 1;
@expose green : Bits_8 = 2;
@expose blue  : Bits_8 = 3;
```

within `Color`.

Enum members lower to constants after type checking. They are not runtime
fields.

Explicit conversion construction is a static dispatch on the destination type:

```ttx
Color -> from(value)
```

That keeps aggregate construction on pack fitting and keeps `Enum[Storage](pack)`
reserved for enum declarations. The parser does not need a special conversion
expression form. It parses conversion as the same explicit call syntax used for
all callable dispatch, and type validation queries check whether the destination type
exposes a valid `from` function for the source value.

## Packs

Parentheses always form a pack. There is no separate grouping expression.

```ttx
()                         // empty pack
(value)                    // one-element pack
(one, two)                 // positional pack
(.ok = true, .value = out) // named pack
(.10 = 50, .65 = 14)       // indexed pack
```

Parser shape: a pack starts with `PackingStart` and ends with `PackingEnd`.
Inside, a leading `AddressOp` selects designated-field parsing. If the token
after `.` is an addressable, parse a named field. If it is an integer literal,
parse an indexed field. Otherwise parse positional expressions. Once a pack has
selected named, indexed, or positional mode, the other modes are errors.

This is a language invariant: named packs start with `.field`, indexed packs
start with `.integer`, and positional packs start with a value expression. The
parser never waits for later fields to decide which kind of pack it is parsing.

A one-element pack can fit where a single value is expected, so `(a + b) * c`
still works. The parser does not need a grouping node. Semantic pack fitting
collapses the one-element pack in value contexts.

Pack modes must not be mixed:

```ttx
(.x = 1, .y = 2) // valid
(1, 2)           // valid
(.10 = 50)       // valid
(1, .y = 2)      // invalid
(.x = 1, .10 = 2) // invalid
```

Packs are in-flight value groups. A pack has a fluid layout: it carries value
order, optional names, and element types, but it has no runtime object identity
or address of its own unless it is fitted into a concrete receiving context
such as a call, return, assignment, attribute, layout, or aggregate type.

Indexed packs are a narrow data-table initialization feature. They are intended
for sparse fixed-size aggregates such as ASCII lookup tables, Base64 decode
tables, opcode tables, and similar cases where most elements use defaults and a
few explicit slots differ.

An indexed designator uses an explicit integer literal after `.`:

```ttx
@hidden decode_table : Vec[Bits_8, 256] = (
  .43 = 62,
  .47 = 63,
  .48 = 52,
  .65 = 0,
);
```

The designator position is not an expression context. It does not accept
character literals, names, imports, enum values, arithmetic, or constants:

```ttx
(.'A' = 0)          // invalid
(.Ascii.upper_a = 0) // invalid
(.40 + 3 = 62)     // invalid
```

Decimal and hexadecimal integer literals are still integer literals, so both
`.65 = 0` and `.0x41 = 0` are explicit indexes if the lexer supports both
literal spellings. Type/layout fitting validates that the target is index-addressable,
that each index is in range, that no index is repeated, and that every value
fits the target element type. Omitted positions use the target element default.

Packs have a carrier order. When a pack is passed, returned, or otherwise
materialized, the pack's field order is the order consumed by the ABI or by the
next compiler stage. Names do not erase that order. Names add semantic mapping
information on top of the ordered fields.

Structs and other typed aggregates have concrete layouts. Their declaration
order is the storage and wire layout of the aggregate value. Type/layout fitting may map a
named fluid pack into a typed aggregate by name, but lowering must still emit
the aggregate in the aggregate's declaration order.

For example, this declaration order is `a, b, c`:

```ttx
@hidden Thing : Struct {
  @expose a : Bits_32;
  @expose b : Bits_32 = 0;
  @expose c : Bits_32;
}
```

This initializer is valid because the names identify the target fields:

```ttx
@stack x : Thing = (
  .c = 1,
  .a = 3,
);
```

The source pack carrier order is `c, a`. The constructed `Thing` stores fields
as `a, b, c`, filling `b` from its default. This is an automatic materialization
shuffle from a named fluid layout into a concrete layout. If a function returns
a named pack whose declared return layout is `c, a`, the return carrier order is
also `c, a`; assigning that result to `Thing` requires lowering to shuffle the
returned values into the target aggregate order.

TTX has three explicit ways to produce packs:

1. grouping values with `(...)`
2. swizzling fields with `value.[a, b, c]`
3. slicing a range with `value.[start +: count]`.

Those forms are the only source-level decomposition operations. A concrete
typed object inside a pack remains one value until source explicitly swizzles or
slices it back into a fluid pack.

Nested positional packs flatten during pack fitting:

```ttx
(1, (2, 3))
```

fits the same positional layout as:

```ttx
(1, 2, 3)
```

Typed values are the boundary. `Vec2D`, `Vec4D`, `Color`, `Struct`, `Object`,
and other aggregate values do not implicitly splat into their fields. Source
must access, swizzle, or slice the fields explicitly:

```ttx
(screen_pos, 0.0, 1.0)        // three values: Vec2D, Real_32, Real_32
(screen_pos.[x, y], 0.0, 1.0) // four Real_32 values
```

### Repacking

Repacking means producing a new pack in the carrier order or named shape
required by the receiving context. TTX does not need a separate repack operator
because pack construction, swizzle, slice, and named fields already express the
operation directly.

Use positional composition when the target wants values in a specific order:

```ttx
@stack position : Vec3D = (screen_pos.[x, y], z);
```

The swizzle decomposes `screen_pos` into a positional pack, and the outer pack
adds `z`. Nested positional packs flatten during fitting, so the target sees
three values.

Use a named pack when names are the contract:

```ttx
@stack thing : Thing = (
  .c = source_c,
  .a = source_a,
);
```

The named pack states that the values are semantically `c` and `a`, regardless
of source order. Type/layout fitting maps those names to the target fields and fills any
valid defaults. This is the clean way to add, drop, or reorder values at a
layout boundary when names matter.

Use swizzle or slice when the source is a typed value:

```ttx
@stack rgba : Color = texture -> sample(uv);
@stack rgb_alpha : Vec4D = (rgba.[r, g, b], alpha);
@stack first_two : Vec2D = rgba.[0 +: 2];
```

Typed values never splat implicitly. The source must say which fields or range
are being repacked.

## Layouts

Layouts describe expected value shape:

```ttx
Count
[]
[Bits_32, Bits_32]
[.x : Bits_32, .y : Bits_32]
[@builtin(.slot = 0) .source : View[Bytes], .count : Count]
```

Parser shape: a layout either starts with a type reference or with
`IndexStart`. After `[`, `AddressOp` or `Attribute` means a named layout
field list. `IndexEnd` means the empty layout. Otherwise parse an unnamed type
reference list. A one-element unnamed layout normalizes to the direct value
layout.

Named layouts follow the same invariant as named packs. The field marker is
visible before the field body: either `.field : Type` or attributes followed by
`.field : Type`. Unnamed layouts start with a type reference. This keeps layout
parsing local and separates expected shape from pack syntax.

The same layout syntax is used for:

- function parameters
- function returns
- `for` bindings.

A named layout field starts with `.` and an addressable field name. Attributes
may decorate layout fields. An unnamed layout is a list of type references.

`[]` is an empty layout. In return position, it represents no returned values.
A single unnamed type in brackets normalizes to the direct value layout:
`[Count]` and `Count` are semantically equivalent.

### For Layouts

`for` binds a layout from an iterable expression:

```ttx
for [.i : Count] in 0...count {
  ...
}

for [.x : Count, .y : Count] in points {
  ...
}
```

The multi-field form consumes values in groups. Conceptually, the layout's
field count defines the stride through the produced value stream.

## Pack Fitting

Pack fitting checks whether produced values match
an expected type or layout. It is used by:

- function calls
- returns
- assignments and declarations
- `if` and `while` conditions
- `for` bindings
- enum named packs
- attributes.

Rules:

1. A one-element pack may fit as its single value.
2. A zero-element pack fits `Void` or an empty layout.
3. Positional packs fit positional layouts by arity and type.
4. Named packs fit named layouts by field name.
5. Indexed packs fit index-addressable aggregate types by literal index.
6. Named, indexed, and positional fields do not mix.
7. Nested positional packs flatten before positional arity is checked.
8. Named packs may fit typed aggregates when the target type has named fields
   and every provided field name and type matches the target. Required target
   fields must be present; fields with defaults may be omitted.
9. Positional packs may fit typed aggregates when the pack order matches the
   target layout exactly. Omitted fields are only valid when the omitted region
   is a trailing region of defaulted fields.
10. Indexed packs may fit fixed-size homogeneous aggregates such as `Vec[T, N]`.
    Omitted indexes use the element default. Indexes must be explicit integer
    literals, in range, and unique.
11. Concrete typed values do not flatten into packs implicitly. Source must use
   swizzle or slice syntax to decompose a typed value into a fluid pack.
12. A named pack fitted into a typed aggregate maps by name, then lowers into
    the aggregate's declaration order. A named pack returned from a function
    keeps the function return layout's carrier order at the call boundary.

`Vec[T, N]`, `Vec2D`, `Vec3D`, `Vec4D`, `Color`, user `Struct`, and other
aggregate types are concrete typed values. They may consume compatible packs,
and they may lower to the same storage representation, but they are not aliases
for packs. The fixed vector and color types have builtin struct fields:
`Vec2D.[x, y]`, `Vec3D.[x, y, z]`, `Vec4D.[x, y, z, w]`, and
`Color.[r, g, b, a]`.

## Expressions

Expressions produce values. Assignment is not an expression.

The expression parser is a precedence parser:

```text
range
or
and
comparison
addition/subtraction
multiplication/division/modulo
unary
postfix access
primary
```

Parser shape: expression parsing starts at range precedence and descends through
the precedence ladder. Primary expressions include literals, addressables,
`self`, `package`, discard, type references, packs, and data expressions.
Postfix parsing then extends the primary with access-chain suffixes until the
current token no longer starts a valid suffix.

Unary `-` is parsed as a prefix operator, not as part of a signed numeric token.
The lexical pass always emits `SubOp` for `-`. In operand position the syntax
parser consumes `SubOp` as unary negation; after an expression it consumes the
same token as binary subtraction. This makes `value-1`, `value - 1`, and
`value - -1` whitespace independent without backtracking or token
reinterpretation.

Logical operators are words:

```ttx
ready and count > 0
failed or !valid
```

`&` and `|` are reserved. Bit operations are explicit methods so width
conversion remains visible:

```ttx
Bits_32 -> from(value) -> bit_and(Bits_32 -> from(0xFF));
```

## Access Chains

Postfix access is the main expression extension point:

```ttx
package.version
self.texture
source[0]
source[0 +: 4]
color.[r, g, b]
source -> get_size()
Image -> from_bytes(bytes)
Color -> from(value)
action -> invoke(args)
```

Parser shape: access-chain parsing loops over postfix suffixes. `AddressOp`
continues only when followed by an addressable or type name. `AddressOp` is
lookup only: package lookup, type lookup, enum member lookup, or value field
lookup. `CallOp` is the only call marker. It requires a callable name and a
pack. The call base may be a value, `self`, `package`, a package/type path, or a
future function-pointer value. `IndexStart` parses index or index-slice content.
`SwizzleOp` parses swizzle fields or a swizzle slice. A swizzle or swizzle slice
produces a positional pack. `PackingStart` after a type expression is invalid;
aggregate construction is pack fitting against an expected type, while explicit
conversion is ordinary `-> from(...)` dispatch.

Access forms:

| Syntax              | Meaning                                      |
| ------------------- | -------------------------------------------- |
| `.field`            | field, package member, or type member access |
| `[index]`           | index access                                 |
| `[start +: count]`  | index slice                                  |
| `.[a, b, c]`        | swizzle that produces a positional pack      |
| `.[start +: count]` | swizzle slice that produces a positional pack |
| `-> name(pack)`     | callable dispatch from the left-side base    |

Calls always take a pack. If no arguments are present, the call still formats
as `receiver -> method()`. Static functions use the same syntax:
`Type -> name(args)` or `package -> name(args)`. Function-pointer invocation is
ordinary dispatch on the function-pointer value, such as
`callback -> invoke(args)`.

The dispatch receiver must be concrete: a typed value, a type name, or a
package scope. A fluid pack is not a receiver because it has no concrete type or
addressable identity. To call through a value carried inside a pack, source or
the validation query must first select the concrete entry that is the receiver.

TTX does not use braced initializers. Braces are scopes and statement blocks.
Aggregate initialization uses packs:

```ttx
@hidden values : Vec[Bits_32, 4] = (1, 2, 3, 4);
@hidden color  : Color = (.r = 1.0, .g = 0.0, .b = 0.0, .a = 1.0);
```

The receiving type supplies the expected shape. Positional packs match by
layout order. Named packs match by field name. Source must use swizzle or slice
syntax when it wants to decompose an existing typed value into a pack:

```ttx
@stack clip_position : Vec4D = (screen_pos.[x, y], 0.0, 1.0);
```

## Assignment Statements

Assignment is a statement:

```ttx
target = value;
target += value;
target -= value;
```

Parser shape: statement parsing does not have a separate assignment pre-parser.
It parses the left expression once. If the next token is `Assign`, `AddAssign`,
or `SubAssign`, the parsed expression must be an assignable address chain. Then
the parser consumes the operator, parses the right expression, and requires
`EndStatement`.

The left side must be an assignable address chain. Valid roots are:

```ttx
name
self.field
package.name
```

Bare `self` and bare `package` are not assignment targets. They are context
roots and require an access suffix.

Valid assignment targets include:

```ttx
value
value.field
value[index]
value[start +: count]
value.[x, y]
self.field
package.setting
```

Invalid examples:

```ttx
self = value;
package = value;
value + other = 1;
receiver -> method() = value;
```

The parser now handles this without speculative parsing: it parses the left
expression once, then checks whether the resulting AST is an assignable address
chain before accepting the assignment operator.

## Statements And Control Flow

A statement starts with one of a small number of shapes:

```ttx
@stack total : Count = 0;     // declaration
return total;                 // return
if (total > 0) { ... }        // scope keyword
source -> copy_to(dest);      // expression statement
total += 1;                   // assignment statement
```

Parser shape: statement parsing first skips comments. `ScopeEnd` and
`EndOfStream` end the current block. A sigil starts a declaration. `Return`
starts a return statement. `Break` and `Continue` start loop-control
statements. Scope keywords start their corresponding structured statement.
Everything else starts expression parsing and may become either an assignment
statement or expression statement based on the next token.

Scope statements are keyword-led:

```ttx
if (condition) { ... }
@if(.enabled = true) { ... }
while (condition) { ... }
for [.i : Count] in values { ... }
match value {
  case pattern: { ... }
  case _: { ... }
}
```

`if` and `while` take condition packs. Condition validation checks that the
condition fits a truthable layout. Numeric truthiness is not implicit. Use
`Bool` or an explicit comparison.

`@if` uses the same statement shape as `if`, but its condition is compile-time
data. It resolves before runtime lowering.

`match` patterns are expressions or `_`. Each case owns an explicit block.
There is no fallthrough.

`break;` and `continue;` are reserved loop-control statements. Dialect
validation decides where they are legal; the syntax layer only preserves the
statement shape for later lowering.

## Literals

Literal classes:

| Source            | Meaning                                     |
| ----------------- | ------------------------------------------- |
| `123`             | decimal numeric literal                     |
| `0xFF`            | hexadecimal numeric literal                 |
| `0.5`             | floating literal                            |
| `"text"`          | string literal, no implicit null terminator |
| `0x[01 02 FF]`    | byte data literal                           |
| `$[path/to/file]` | embedded file data                          |
| `true`, `false`   | `Bool` literals                             |

Integer literals are exact integer values. When no narrower expected type is
present, decimal and hexadecimal integer literals live in the language's
64-bit integer domain, with `Count` serving as the ordinary size/count alias.
Type validation may fit an integer literal into a narrower fixed-width numeric target
only when the value is provably in range. It does not infer among user-defined
types or choose between overloads from a bare numeric literal.

Floating literals follow the same principle: the literal text represents an
exact source value, and type validation either fits it to the expected floating target
or uses the documented default real type when no narrower target exists.

`Bytes` literals and embedded-file literals are tokenized as whole literals, but
their fixed prefixes are still source text entries on `Class`:

```text
Bytes    -> 0x[
Embedded -> $[
IndexEnd -> ]
```

The formatter composes those fixed delimiters through `Class` and preserves the
literal payload from the token.

## Comments And Documentation

Line comments start with `//`. The tokenizer strips the marker and stores the
comment text. Consecutive comments before a member are treated as documentation
for that member by the syntax tree and formatter:

```ttx
// Stored in source order.
// Attached to the following member.
@hidden signature : Vec[Bits_8, 8] = 0x[89 50 4E 47];
```

Comments inside statement bodies are currently skipped by statement parsing
rather than represented as executable statements. Member documentation comments
are preserved.

## Canonicalization

Canonicalization is a resolution query. It happens after parsing and before
validation that depends on resolved types.

It resolves:

- local aliases
- imported package aliases
- builtin type names
- type path segments
- type arguments
- numeric type arguments.

Canonicalization is allowed to lose source spelling. The formatter may preserve
spelling for source output, but compiler analysis uses resolved type identity
rather than repeatedly comparing source text.

## Diagnostics And Recovery

The parser emits diagnostics where the source shape is wrong and then advances
enough to continue finding later errors. `require()` is the main
delimiter/marker helper: it reports the expected token and advances regardless,
which makes recovery predictable.

Diagnostics use human names from `Class::get_name()` and source spellings from
`Class::get_source_text()`. This keeps errors aligned with the token model:

```text
Expected definition `:` but got ttx library package
```

Semantic diagnostics prefer ranges that cover the meaningful construct, not
only the token where the compiler first noticed the problem. For example, an
invalid type argument list highlights the argument range when possible.

## Type Kinds

The semantic type space is intentionally small:

| Kind                    | Runtime model                   | Notes                                            |
| ----------------------- | ------------------------------- | ------------------------------------------------ |
| primitive numeric types | value                           | fixed width, no implicit widening                |
| `Bool`                  | value                           | only `true` and `false` are truthable by default |
| `Vec[T, N]`             | typed aggregate value           | homogeneous fixed-size aggregate                 |
| `Vec2D`, `Vec4D`, `Color` | typed aggregate value         | named component types with concrete storage      |
| layout                  | structural value stream         | anonymous heterogeneous aggregate                |
| `Struct`                | nominal value                   | does not flatten implicitly                      |
| `Object`                | nominal managed value           | heap/reference semantics, dialect-limited        |
| `Foreign`               | external ABI scope              | declarations lower to linked symbols             |
| `Shader`                | shader scope or dialect concept | may lower to GPU module plus host glue           |
| `Enum`                  | compile-time namespace          | members lower to constants                       |
| `Alias`                 | compile-time type path          | erased after canonicalization                    |

`Library`, `Shader`, and `Entity` are package dialects. Some names, especially
`Shader`, can also appear as builtin definition kinds where the dialect permits
them.

## Relationship To LLVM IR And MLIR

TTX is IR-like, but it is not a textual spelling of LLVM IR and it is not MLIR.
The difference is mostly one of layer and audience.

| Area | LLVM IR | MLIR | TTX |
| --- | --- | --- | --- |
| Primary representation | Lowered SSA module/function/block/instruction IR | Extensible operation/SSA IR with regions | Source-shaped AST plus context layers |
| Main extension point | Intrinsics, metadata, passes, and targets | Dialects define operations, types, attributes, and interfaces | Fixed syntax; package dialects define legality and metadata |
| Typical motion | Optimize and transform already-lowered IR | Rewrite and convert operations between dialects | Enrich the same source graph with queryable facts |
| Text form | Debug, test, and serialization form for compiler IR | Debug, test, and serialization form for multi-level IR | Human-authored canonical source surface |
| Dialect granularity | Target and metadata oriented, not dialect-first | Operations from many dialects can coexist freely | Package dialect controls the legal semantic world |
| Source preservation | Mostly lowered away before LLVM IR | Supported through locations and higher-level dialects | Central design constraint |
| Tooling goal | Optimizer and code-generation substrate | Reusable compiler infrastructure | Shared frontend IR for compiler, editor, and build tooling |
| Lowering | Already lowered enough for optimization | Core workflow through dialect conversion | Delayed until terminal artifacts |

LLVM IR is a low-level, strongly typed compiler IR. Its textual form is useful
for debugging and testing, but most source-level structure has already been
lowered away. Control flow is explicit blocks and branches. Aggregate and
address operations are close to machine-level lowering. LLVM IR is excellent as
a target for optimization, but it is not designed as a comfortable authored
surface for package structure, shader metadata, source-level imports, named
packs, or domain-specific declarations.

MLIR is a multi-level IR framework. Its power comes from extensible dialects,
operations, attributes, regions, and rewrite infrastructure. MLIR can represent
many layers of abstraction at once, and dialects can define their own operation
semantics. TTX borrows the idea that dialects matter, but makes a different
tradeoff: the dialect is selected at package level and the syntax remains a
small fixed language rather than an extensible operation syntax.

TTX sits above LLVM IR and beside the lower levels of an MLIR-style pipeline.
It keeps enough human-facing structure to be authored and reviewed directly:
packages, imports, comments, sigils, attributes, named layouts, packs, scoped
builtins, and shader/foreign declarations. At the same time, it avoids the
open-ended grammar surface of a general programming language. The source is
structured so a parser can build semantic shape directly and cheaply.

The architecture also differs in how intermediate state is treated. LLVM IR is
already a lowered representation optimized for analysis and code generation.
MLIR explicitly models many dialect operations and rewrite levels. TTX instead
keeps the authored source tree as the stable carrier for as long as possible and
enriches it with context. Lexical, syntax, resolution, validation queries, dialect, and
compilation contexts are layers over the same program, not permission to create
independent copies of the program's meaning. The backend may eventually emit
LLVM IR, SPIR-V, x86_64 code, or another terminal artifact, but those are output
boundaries, not the organizing principle of the source language.

In short:

- Compared with LLVM IR, TTX is higher level, more source-preserving, and more
  domain aware.
- Compared with MLIR, TTX is less extensible at the syntax level but simpler to
  parse and easier to hand-author. There is no reason Tetrodotoxin could not
  target MLIR, but moving in that direction now would make much of the
  toolchain clone MLIR infrastructure for little benefit to either project at
  this stage.
- Compared with a conventional programming language, TTX exposes more storage,
  layout, type, and lowering information in the syntax itself.
- Compared with a traditional compiler pipeline, TTX emphasizes monotonic
  context layering: later stages answer richer questions about the same program
  rather than replacing the program with a new primary IR at every step.

## Lowering Direction

TTX maps naturally to LLVM-like IR concepts:

| TTX                           | Lowering idea                                            |
| ----------------------------- | -------------------------------------------------------- |
| package                       | module or compilation unit                               |
| import                        | module dependency or package alias                       |
| function                      | function definition or external declaration              |
| block                         | structured region that lowers to basic blocks            |
| `if`, `while`, `for`, `match` | branches, loops, phi/select logic, block graphs          |
| `break`, `continue`           | loop exits and loop-iteration control                    |
| field/index access            | address calculation, often `getelementptr`-like          |
| swizzle                       | vector shuffle, aggregate extract, or aggregate insert   |
| pack fit                      | call ABI shaping, return shaping, aggregate construction |
| `@` compile-time values       | constants, metadata, specialization inputs               |
| `Foreign` functions           | declarations resolved by ABI/linker                      |
| `Shader` package              | shader artifact plus host-side glue                      |

The source-level constructs are frontend contracts. Many disappear during
lowering, but they remain explicit long enough to produce good diagnostics,
validate dialect rules, and encode target metadata.

## Design Invariants

When changing TTX, preserve these invariants:

1. The tokenizer assigns a single class to every source spelling.
2. Fixed source spellings live in `Lexical::Class::get_source_text()`.
3. The parser does not backtrack or suppress diagnostics to choose a parse.
4. Assignment remains a statement, not an expression.
5. Parentheses always mean pack.
6. Function parameters, function returns, and `for` bindings all use layouts.
7. Named packs start with `.field`; named layouts start with `.field` or
   attributes followed by `.field`.
8. Named and positional aggregate fields do not mix.
9. Runtime-vs-compile-time addressability remains visible in sigil token
   classes.
10. Dialect legality belongs to explicit validation passes, not raw parse-shape
    construction.
11. Formatter output derives from the same token source text as the lexer
    whenever the token has fixed source text.

These rules are what keep TTX readable while still letting it behave like a
compiler IR.
