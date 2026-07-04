# TTX Language Semantics

TTX is a frontend source IR language. It is authored by humans, but it stays
close to the compiler's semantic model. A TTX file makes package boundaries,
storage shape, addressability, layout, and lowering intent visible in the
source. Tetrodotoxin uses TTX as its default frontend, but the TTX source IR and
token bytecode can also serve as an interchange boundary for another host that
agrees to the token and data model.

For high-level language design and usage, see
[ttx_design.md](ttx_design.md). This document is the canonical contract for TTX
semantics and is sufficient to build a conforming TTX evaluator from scratch.
It explains how each language concept is tokenized, evaluated, represented, and
checked by the owner that knows the rule before lowering.

TTX's central design philosophy is to provide as much of a human-editable
surface as possible without giving up the advantages of an IR. The source is
pleasant to read and review, but it is also extremely cheap and reliable to
parse. A practical way to state the design goal is: TTX is an IR-like language
where humans edit semantic structure directly without drowning in compiler
machinery.

TTX is designed for **monotonic context layering**. A host can enrich the same
source IR and token bytecode with queryable context instead of throwing it away
and replacing it with a private IR. A tool can stop as soon as it has the
context it needs: syntax highlighting can stop after lexical classification,
formatting can stop after source shape, project navigation can stop after a host
builds a module graph, and code generation can continue through type, layout,
ISA, ABI, provider, and compilation queries. Fast compilation is part of that
goal, not an afterthought. Every syntax feature must justify its parse cost,
recovery cost, and downstream context cost.

To make that work, the syntax carries a large amount of semantic information
directly. Casing separates addressable names from type names. Sigils encode
visibility and compile-time addressability. Definition words such as `struct`,
`object`, `enum`, `foreign`, and `alias` are lowercase token classes rather than
ordinary identifiers or PascalCase types. Packs, layouts, access chains, and
assignment statements all have distinct local shapes.

Each major section below describes the source form, the token or local shape
that starts evaluation, the TTX facts produced, and the checks owned by that
shape. The exact C++ implementation can change, but these contracts remain
stable.

## Formal Language Model

TTX has two useful formal layers:

- The token bytecode and syntactic grammar are deterministic context-free after
  lexical analysis. The grammar is designed for predictive decoding with
  bounded fixed lookahead, plus operator-precedence decoding for expressions.
- The set of semantically valid TTX programs is context-sensitive. Name binding,
  import binding, type canonicalization, ISA rules, pack fitting, and
  addressability checks all depend on program context.

TTX is not trying to be a maximally expressive context-free grammar. It is a
context-sensitive source IR whose concrete grammar is engineered to be
deterministic and bounded-lookahead. Its main formal trick is moving semantic
category information into lexical and syntactic form so an evaluator can build
source-shaped TTX facts without speculative parsing or later reinterpretation.

In practical decoder terms, TTX is LL-like rather than a pure LL(1) grammar.
Several local decisions use fixed lookahead, such as distinguishing named and
unnamed layouts after `[`. Expressions are not written as left-recursive grammar
productions. They are decoded by precedence so recursive descent never needs to
expand an expression before consuming input.

The grammar is intentionally left-factored around visible source markers. A
decoder consumes the shared prefix of a construct once, then branches on the
next token that actually distinguishes the alternatives. The syntax does not
require backtracking, speculative parsing, speculative diagnostics suppression,
or reinterpretation of an already-decoded subtree.

## Token Bytecode And VM Execution

TTX source text is the human-authored source IR. Lexical analysis lowers that
source IR into TTX token bytecode: a compact instruction stream whose token
classes already carry stable semantic categories such as `Type`, `Addressable`,
`Import`, `Assign`, `TypeAccessOp`, `AddressOp`, `CallOp`, modifiers, attributes,
and fixed operators.

The current token class vocabulary has 70 values: 68 source-facing bytecode
classes plus `Unknown` and `EndOfStream` sentinels. The class value is stored in
8 bits, and payload-bearing tokens keep source text views beside that class.
Those values are arbitrary as byte values, but not arbitrary as language facts:
each token class exists because it gives an envelope evaluator, body evaluator,
or other host-owned decoder a useful intended category before any larger
instruction is decoded.

The bytecode is not a fixed-width instruction stream. A token is the smallest
decoded unit, but an ISA instruction is whatever span the active ISA fetches and
decodes from the cursor. Some instructions consume one token. A package export,
layout, function declaration, or control statement may consume many tokens. A
string or byte literal is a single token whose payload can be large. The useful
hardware analogy is closer to a variable-width instruction stream than a
one-token-per-instruction machine, but the important rule is simpler: Lexical
classifies source into stable tokens, and the active ISA owns the decode length
and execution rule for the bytecode it accepts.

TTX does not define a canonical ISA or canonical ISA set. It defines source IR,
token bytecode, and the shared facts that an interpreter can use. Which ISAs
execute that bytecode is left to the toolchain that hosts TTX.

The Tetrodotoxin reference host uses a small `Boot` ISA to evaluate its source
envelope before dispatching to another ISA. For that concrete host model, see
[`tetrodotoxin_design.md`](../tetrodotoxin/tetrodotoxin_design.md).

The useful mental model is:

```text
TTX source IR
-> lexical token bytecode
-> optional envelope evaluation
-> optional import or module loading
-> optional ISA or evaluator execution
-> lowering, tooling, or interchange output
```

An ISA is a semantic instruction set, not a backend target and not a parse-tree
visitor. It decodes the token bytecode it owns, validates the rules for that
authoring domain, and publishes TTX facts: types, layouts, package exports,
callable facts, ABI facts, shader facts, or other queryable context.

Import loading, package loading, cache validity, and body-ISA dispatch are host
concerns. TTX describes the bytecode and shared data model those systems execute
against.

## Monotonic Context Layering

A host that consumes TTX can be small or large. It might only tokenize source
for highlighting, execute a private ISA for embedded scripting, or run a full
compiler pipeline. The durable object is still the same source program and token
bytecode with progressively enriched context.

```text
TTX source IR
-> lexical token bytecode
-> optional host envelope context
-> optional import or module context
-> optional ISA or evaluator context
-> owned query context
-> terminal output
```

The rule is monotonic: a stage can add facts, check invariants, cache derived
answers, or expose a richer query surface. It does not reinterpret earlier
facts, silently discard source shape, or copy host-specific facts into a
parallel primary representation. A new standalone representation is appropriate
only when the pipeline intentionally crosses a terminal boundary, such as
emitting SPIR-V words, LLVM IR, a C header, an object file, an archive, JSON for
an editor, or formatted source text.

The practical rule for moving information left is:

> Push stable, local, syntax-visible semantics left. Keep non-local facts in
> explicit queryable context layers.

The lexer classifies stable source spellings such as type-shaped names,
addressable names, modifiers, builtin type forms, and fixed operators. A host can
then choose how much more context it wants: an envelope evaluator, an import
graph, one or more ISAs, type and layout queries, ABI facts, or backend
lowering. Compilation asks those contexts questions rather than rediscovering
source intent from raw text.

This model is close to typed AST and attribute-grammar systems, and it also
resembles query-based incremental compilers. The important distinction is
ownership:

- TTX source owns authored bytes and source spans.
- Lexical owns token classification and token payload views.
- Documentation, Type, and Layout own the shared TTX data model.
- Host envelope evaluators own whatever source preamble they choose to execute.
- Import, module, package, cache, and invalidation layers belong to the host
  that needs them.
- ISAs or evaluators own the bytecode spans they understand and the facts they
  publish from those spans.
- ABI, provider, and backend queries expose derived answers without cloning the
  program into a second semantic tree.
- Terminal emitters own output-specific representations such as SPIR-V words,
  LLVM IR, object files, archives, generated headers, editor JSON, or formatted
  source text.

## Layout Facts

TTX uses layout as the shared fact for value shape and storage shape. Scalars,
vectors, colors, structs, function parameters, returns, packs, and swizzles all
bottom out in layout questions: what entries exist, what names they have, what
types they carry, and whether the result has concrete storage.

There are two important layout kinds:

- **Fluid layouts** describe value flow. Packs, grouped values, function
  argument packs, and intermediate return packs are fluid. They can flatten
  nested positional groups, acquire names from authored syntax or receiving
  boundaries, discard names during repacks, and be repacked before they
  materialize. A fluid layout has no stable address of its own.
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

Names are authored or boundary-provided facts. A named pack authors names. A
function parameter layout, function return layout, type layout, or other
receiving boundary can provide names for fitting and later access. A repack
operation produces a positional fluid layout unless the operator explicitly
authors or preserves names. This keeps temporary expression shape from carrying
incidental field labels through grouping, swizzle, slice, calls, or returns.

This is why a pack can initialize a struct by name while an existing struct
does not implicitly decompose back into a pack. Source must use swizzle or
slice syntax to cross from concrete storage back into fluid value flow.

Callable dispatch also requires a concrete receiver. The `->` operator is
valid on concrete typed values, concrete type names, or concrete package scopes.
It is not valid on a raw fluid pack because a fluid pack has no stable receiver
identity. Its entries can still be mapped, fitted, reordered, or materialized by
type and layout fitting, but they are not addressable storage until a concrete
target is known.

Implementations prefer queryable views over copied summaries. For example, a
shader backend asks a shader ISA context for `stage_of(type)`,
`descriptor_of(member)`, and `is_push_constant(type)`, and asks the type and
layout query surface for `type_of(expression)` or `fit_of(pack)`. It does not
need a separate shader-specific clone of the package AST before it starts
lowering.

## Host Architecture

A complete TTX host can be as small as a tokenizer consumer or as large as a
compiler toolchain. TTX itself defines the first durable boundary:

```text
source IR
-> token bytecode
```

After that, the host decides which additional layers exist:

```text
token bytecode
-> optional envelope or entry evaluator
-> optional import or module graph
-> optional ISA or body evaluator
-> optional owned queries
-> optional terminal output
```

These names are examples, not required TTX stages. TTX does not have a broad
semantic pass whose job is to reinterpret ambiguous syntax after parsing. The
source already carries the semantic category of each construct, and lexical
analysis lowers those categories into token bytecode. Later layers, when a host
has them, connect, type-check, cache, and lower already-shaped facts by
enriching the available context.

There is no required replacement IR layer for checks. Type, layout, ISA,
provider, ABI, module, package, and backend owners expose the query surfaces
they understand. These queries may reject a program and may cache concrete
facts beside the source or token bytecode. They do not guess which namespace a
name belongs to, choose between ambiguous parse trees, reinterpret syntax after
the fact, or collect backend-specific metadata into a second authority.

Common host roles are:

1. **Lexical**: lower source text into meaningful token bytecode. The tokenizer
   distinguishes addressables, type names, grammar keywords, modifiers,
   attributes, byte literals, embedded file literals, and operators.
2. **Envelope or entry evaluation**: execute any host-defined preamble. A host
   may use no envelope at all, or it may use a small base ISA to collect
   documentation, selected ISA name, and import requests.
3. **Import or module context**: attach cross-file, package, module, or FFI
   state when the host needs more than one source unit.
4. **ISA or body evaluation**: execute the token spans owned by the selected
   instruction set and publish TTX facts from that execution.
5. **Owned queries**: expose type, layout, ISA, provider, ABI, and
   backend-boundary answers over the evaluated facts.
6. **Terminal output**: emit a requested output format such as shader binaries,
   embedded read-only data, generated host constants, relocation records,
   objects, archives, editor JSON, formatted source text, or another host-owned
   artifact.

Type checks and layout checks are intentionally narrower than a traditional
semantic pass. For example, `Graphics::Image` is a type query,
`Graphics.image` is member access, and `Graphics -> image()` is function
dispatch. The token and operator shape already chose the namespace of the
query. A host only has to prove whether the selected name exists and whether
the result can be used in the local context.

When a host supports imports, imports are graph edges between independently
owned source units. TTX does not require C or C++ style text inclusion, and it
does not require one global package graph. A host can bind token bytecode to
foreign source units, native ABI facts, editor data, generated packages, or
another language runtime as long as the boundary publishes the TTX facts its
queries need.

A program is ready for lowering when the host has proven the facts its output
requires. For a compiler this may mean every expression has a concrete value
type, every pack has fitted an expected shape or remains in a context that
allows a pack, every access chain has a typed result, every call has a selected
callable target, and every statement has the type facts needed for lowering.
Other hosts can stop earlier. The key rule is that the host asks enriched
context for already-proven answers instead of rediscovering source intent from
raw text.

## Evaluation Contract

The evaluator turns token bytecode into the TTX facts that host contexts and
owned queries expect. It preserves source shape, attaches local type and name
facts as soon as they are available, and never asks later stages to reconstruct
source intent from raw text.

The evaluator does not use rollback, checkpointing, or suppressed diagnostics to
choose between valid grammar paths. It decodes each construct once and then
checks the resulting shape.

There are deterministic lookahead decisions, and those are intentional:

- `Layout::evaluate()` looks one token after `[` to distinguish a named layout
  field, an unnamed layout, an empty layout, or a direct value type.
- `Pack::evaluate_fields()` selects named-pack mode only from a leading `.`
  field. Any other expression-starting token selects positional-pack mode.
- `Member::evaluate()` looks for `modifier func` so function declarations do not
  look like ordinary value declarations.
- `Expression::evaluate_access_chain()` looks after `.` to decide whether there
  is a field access. If the next token is not a valid field name, postfix
  evaluation stops.
- The expression evaluator reads `:[` as the value index or slice operator,
  keeping value indexing distinct from layout/type argument brackets.
- Statement evaluation decodes an expression once. If the next token is an
  assignment operator, the decoded expression is checked to see whether it is an
  assignable address chain.

Those decisions are local and do not backtrack. This is important because TTX
is intended to have an authoritative formatter, useful diagnostics, and a
compiler pipeline that does not depend on hidden parser guesses.

A conforming evaluator follows these rules:

1. Let token classes carry the first layer of meaning.
2. Pick the next production from the current token whenever possible.
3. Treat named and positional aggregate syntax as disjoint modes. Named packs
   start with `.field`; named layouts start with `.field` or attributes followed
   by `.field`.
4. Use bounded lookahead only for local shape decisions, such as distinguishing
   a named layout from an unnamed layout.
5. Decode expressions once. If a surrounding construct needs an assignable
   expression, ask the expression owner whether the decoded shape is
   assignable.
6. Emit diagnostics at the point where a required delimiter, marker, or shape is
   missing, then advance predictably so evaluation can continue.

## Lexical Classes

The tokenizer does more semantic work than a minimal lexer would. This is
intentional. The evaluator sees classes such as `Addressable`, `Type`, `Func`,
`public`, `Attribute`, and `SwizzleOp` directly.

Important lexical distinctions:

| Source shape                | Token class            | Semantic meaning                                    |
| --------------------------- | ---------------------- | --------------------------------------------------- |
| `snake_case`                | `Addressable`          | runtime names, fields, functions, local values      |
| `PascalCase`                | `Type`                 | type names, aliases, ISA names, package names       |
| `enum`, `struct`, `foreign` | definition keywords    | definition forms, not type references               |
| `public`, `private`, etc.  | modifier tokens        | ISA-owned visibility, ownership, or storage         |
| `@name`                     | `Attribute`            | metadata attached to members, params, fields        |
| `@if`                       | `Attribute`            | directive owned by an ISA or host                   |
| `break`, `continue`         | control keyword tokens | loop-control statements                             |
| `0x[...]`                   | `Bytes`                | byte data literal                                   |
| `$[...]`                    | `Embedded`             | embedded file literal                               |
| `_`                         | `Discard`              | wildcard or ignored value                           |

Fixed source spellings live in `Lexical::Class::get_source_text()`. The lexer
and formatter call into `Class` rather than duplicate strings for
keywords, operators, delimiters, byte literal prefixes, embedded literal
prefixes, and marker tokens.

## Source Envelopes And ISAs

Many TTX hosts use a source envelope so a complete file can declare which
instruction set should evaluate the remaining bytecode:

```ttx
dialect : Library;
dialect : Render;
dialect : Shader;
dialect : Entity;
```

Envelope shape: full-file evaluation starts at the reserved lowercase `dialect`
keyword, requires `Define`, then requires a PascalCase ISA name and
`EndStatement`. Subtree evaluators, embedded tools, or foreign hosts may start
below this envelope level when they already know which evaluator should execute
the token bytecode.

The source spelling stays `dialect` for now because that is the author-facing
keyword, but semantically it names an evaluator installed in the active host. A
host with different installed ISAs may reject a source another host accepts.

The ISA name is not a globally reserved keyword. `Package`, `Library`,
`Shader`, and other ISA names remain type atoms outside the header, so type
access such as `YourType::Package` is still valid syntax.

The ISA name selects the top-level instruction set for hosts that use this
envelope. An ISA controls which builtins, attributes, types, address spaces,
runtime features, and body instructions are legal in the source.

| ISA       | Purpose                                                            |
| --------- | ------------------------------------------------------------------ |
| `Library` | general reusable code, binary formats, data transforms, host logic |
| `Render`  | stage-oriented render package authoring and host render contracts  |
| `Shader`  | shader definitions and shader-specific host glue                   |
| `Entity`  | entity-oriented package data and behavior                          |

`object`, `struct`, `enum`, `foreign`, and `alias` are core definition
keywords, but they are not ISA names. For example:

```ttx
dialect : Library;

private Header : struct {
  public width  : Bits_32;
  public height : Bits_32;
}
```

The active ISA may reject a construct that is syntactically valid. Once the host
has prepared whatever envelope, imports, or module context it requires, the
selected ISA owns body evaluation and may reject constructs that do not belong
to that authoring space. `Render` and `Shader` packages do not accept managed
runtime concepts such as `object` or `List` unless those ISAs explicitly define
how they lower.

An ISA is not necessarily a single backend. `Render` and `Shader` packages
may produce GPU code, such as SPIR-V, and host-side code that loads those
constants, builds pipeline layouts, and bridges them into the Perimortem
runtime. Backend outputs such as SPIR-V, x86_64, generated headers, or Vulkan
bridge code are compilation targets, not separate ISAs.

## Imports

For hosts that use the common envelope, imports introduce explicit local aliases
for package dependencies:

```ttx
import ImageLibrary : Library = "graphics/image.ttx";
import Graphics     : Package = Perimortem::Graphics;
```

Envelope shape: imports appear immediately after the ISA selection instruction
and before members. An import starts with `Import`, then a PascalCase local
name, `Define`, an expected ISA name, `Assign`, an import source, and
`EndStatement`. The import source is chosen from the current token: `String`
means a file source, while `Type` means a package name.

The import shape is:

```ttx
import LocalName : IsaName = source;
```

The source is either:

- a file-source string, such as `"path.ttx"`
- a package name such as `Perimortem::Graphics`.

The local name participates in type queries and value access after the host has
bound the import. Imports do not erase ISA boundaries. A `Shader` package
cannot make `object` legal by importing a `Library` that contains objects.
Imported definitions must still be valid in the importing ISA.

There is no `using` or wildcard import syntax. Imports are named aliases so
source reviews and diagnostics can see package boundaries.

## Names And Type Queries

TTX uses casing as a semantic boundary:

- `snake_case` names are addressable runtime values or fields.
- `PascalCase` names are types, aliases, package names, or ISA names.

Type references are progressive queries, not stored string paths:

```ttx
Bits_32
Vec[Bits_8, 4]
Graphics::Image
Math::Matrix[Real_32, 4, 4]
```

The first `Type` token is resolved immediately from the active context. Each
operator then dispatches on the current value:

- `:: Name` queries a nested type on the current `Ttx::Type`.
- `.name` queries a layout/member on the current value or `Layout(type)`.
- `-> name(...)` queries callable dispatch.
- `.[...]` dispatches to swizzle/repack semantics.
- `:[...]` dispatches to index or slice semantics.

`::` is therefore not string concatenation. `Graphics::Color` means resolve the
local type/package name `Graphics`, then ask that object for nested type
`Color`. A type query only advances one level per operator, which keeps the
model allocation-free and lets each owner report the error at the exact failed
step.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric type arguments, such as the `4` in `Vec[Bits_8, 4]`, are
part of the type query and are checked while proving that query.

Parameterization is type dispatch over a resolved layout. A type reference such
as `View[Bits_8]` resolves `View`, resolves `[Bits_8]` into the parameter
layout, then asks the `View` type object to produce the concrete type identity
for that layout. A type that does not support the requested layout reports the
error at that point. This keeps parameterized types such as `View[T]`,
`Vec[T, N]`, `List[T]`, and package-owned forms as normal type objects rather
than a template or code-generation system.

The concrete type returned by parameterization is the address used for type
equivalence, member lookup, nested type lookup, function lookup, and
`Layout(type)`. The parameter layout is an input to type construction; it is not
stored as an independent representation.

`alias` creates a compile-time type reference. After canonicalization, later
passes do not need to know whether a type was written directly or reached
through an alias.

Decode shape: a type reference starts with `Type`, or a numeric token when
decoding a numeric type argument. `TypeAccessOp` performs one nested-type query
on the current `Ttx::Type`. Type arguments start with `IndexStart`, contain
type references separated by `PackingOp`, and end with `IndexEnd`.

## Prelude And Package Aggregate Types

TTX treats the small vector and graphics color types as real typed aggregates,
not pack aliases. The core prelude and explicit package manifests provide them
with fixed field names and `Real_32` storage.

The core prelude supplies universal scalar, vector, and memory forms as
top-level names. Source uses these names without a package qualifier:

```ttx
Vec2D : struct { x : Real_32; y : Real_32; }
Vec3D : struct { x : Real_32; y : Real_32; z : Real_32; }
Vec4D : struct { x : Real_32; y : Real_32; z : Real_32; w : Real_32; }
```

`Perimortem::Graphics` is explicit. A package that needs graphics-domain types
imports it and refers to those types through the import name:

```ttx
import Graphics : Package = Perimortem::Graphics;

Graphics::Color : struct {
  r : Real_32;
  g : Real_32;
  b : Real_32;
  a : Real_32;
}
```

The prelude declarations are conceptual definitions rather than source that
appears in every file. Package declarations come from resolved package manifests.
All later stages must behave as if the fields are present. Member lookup,
swizzle checks, pack fitting, future host-boundary metadata, and shader lowering
all use these same field definitions.

`Vec[T, N]` is different. It is a fixed-size homogeneous aggregate indexed by
position, so indexed packs may initialize sparse entries. `Vec2D`, `Vec3D`,
`Vec4D`, and `Graphics::Color` have named component fields and support named
packs and named swizzles through those fields.

## Definitions

The core definition forms are:

```ttx
modifier name : qualifier;
modifier name : qualifier = value;
modifier name : qualifier { ... }
```

Parser shape: a definition starts with a modifier accepted by the active ISA.
The next token must be `Addressable` or `Type`, then `Define`, then a
qualifier token accepted by that ISA. After the qualifier, `Assign` introduces
an initializer. Otherwise the definition must end with `EndStatement` or open a
scoped body.

TTX does not have an inferred declaration operator. Every definition writes its
qualifier at the declaration site so the evaluator knows which ISA-owned rule
to run before expression analysis.

Definitions introduce either addressable values or type-like names depending
on the name and qualifier:

```ttx
private Header : struct { ... }                     // type definition
private header : Header = (.width = 4, .height = 2); // runtime value
private CountAlias : alias = Count;                 // compile-time alias-like definition
```

Only builtin definition kinds may open a scope:

```ttx
private Data    : struct  { ... }
private Runtime : object  { ... }
private C       : foreign { ... }
private Stage   : Shader  { ... }
```

Other type-like qualifiers define values and must end with `;` or use `=`:

```ttx
private size  : Count = 4;
private bytes : Bytes;
```

The evaluator reads these shapes. ISA and type owners then check which builtin
kinds are legal for the active ISA, along with the compatibility of the
definition name, qualifier, initializer, modifier, and attributes.

## Modifiers

Modifiers are fixed keyword tokens that give ISAs a shared access and storage
surface without forcing one language-wide policy. The lexer owns the spelling;
the active ISA owns the meaning.

Parser shape: modifiers are token classes, not attributes. A parser never parses
`public` as `Attribute("public")`. Anywhere a modifier is allowed, pass the
allowed `Class::Type` values and check the current token against that set.

| Modifier  | Intended contract                                      |
| --------- | ------------------------------------------------------ |
| `public`  | visible API that other sources may read or call        |
| `private` | implementation detail owned by the active ISA          |
| `expose`  | externally readable data, written by its owner         |
| `state`   | stateful storage that is not part of the value shape   |
| `const`   | write-once or compile-time data                        |

This replaces older ideas such as `hidden`, `stack`, `@const`, and `@comptime`.
The language has fewer spellings, while `Class::Type` still gives evaluators a
cheap dispatch point.

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
and the attribute owner checks the expected keys and legal targets for each
known attribute.

Attributes are not runtime values. They are consumed by the compiler or
forwarded into target metadata.

Some ISAs also consume directive-style attributes as standalone statements.
For example, a Package manifest declares its package identity with:

```ttx
@package_name = Perimortem::Graphics;
```

The decode shape is `Attribute(package_name) Assign PackageName EndStatement`.
That keeps package identity in the Package ISA body while using the same
authored package-name surface as package imports.

Known shader ABI attributes have fixed local targets. The compiler keeps these
rules in `syntax/attribute` so attribute legality stays separate from package
shape parsing and later name binding:

- `@stage(.kind = "...")` applies to `Shader` definitions in `Shader` packages.
- `@push_constant` applies to exposed `foreign` ABI blocks in `Shader`
  packages.
- `@binding(.set = N, .slot = N)` applies to resource members inside `Shader`
  scopes in `Shader` packages.
- `@builtin(.name = "...")` and `@builtin(.slot = N)` apply to shader builtin
  input members or layout fields.

Other attributes may remain target-specific metadata until an ISA defines
their legality rules.

`@if` is an attribute-shaped directive. A host or ISA may reserve that spelling
for compile-time control flow:

```ttx
@if(.enabled = true) {
  state generated : Count = 1;
}
```

The tokenizer emits `@if` as `Attribute`. The ISA that supports it decides
whether the following bytecode span is a scope instruction.

## Disabled Members

The disabled marker `/>` wraps the following member as disabled source:

```ttx
/> private experimental : Count = 1;
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
public func name[params] -> returns {
  ...
}
```

Parser shape: a function member starts with either `Func` or `modifier Func`.
`External` may appear before the function only in a `foreign` scope. After
`Func`, require an `Addressable` function name, parse a parameter layout, require
`CallOp`, parse a return layout, then parse either a block or `EndStatement`.

`func` is a keyword because functions are not merely ordinary values with type
`Func`. They need a stable AST node for ABI lowering, entry-point discovery,
specialization, foreign declarations, and shader compilation.

Both parameters and returns are layouts:

```ttx
private func size[] -> Count;

public func decode[.source : View[Bytes]] -> [
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
private C : foreign {
  external private func inflate[.source : View[Bytes]] -> Bytes;
}
```

External functions are only valid inside `foreign` blocks. TTX does not have
ordinary forward declarations. A function without a body means an ABI promise,
and the linker or foreign ABI provider must resolve it.

## Members And Scoped Builtins

Package-level and scoped definitions are represented as members. A member may
have documentation comments, an optional disabled marker, attributes, a
function form, a scoped builtin body, an enum shorthand, or a value definition.

Parser shape: member parsing proceeds in layers: consume documentation comments,
consume an optional disabled marker, consume attributes, handle optional
`External`, then choose between function syntax and definition syntax. If the
definition kind is `enum`, parse enum members. If it is a scoped keyword such as
`struct`, parse a member scope. Otherwise parse a value definition.

Scoped builtins own member lists:

```ttx
private Header : struct {
  public width  : Bits_32;
  public height : Bits_32;
}

private C : foreign {
  external private func inflate[.source : View[Bytes]] -> Bytes;
}
```

The evaluator identifies scoped builtins by the first resolved type query in the
definition. ISA and type owners check that the builtin is legal in the current
scope and ISA.

## Enums

Enums use a storage-typed named-pack declaration shorthand:

```ttx
private Color : enum[Bits_8](.red = 1, .green = 2, .blue = 3);
```

Parser shape: after `Define enum`, require exactly one type argument naming the
enum storage type. Then parse a normal pack and require that every field is
named. The enum declaration ends with `EndStatement`. It does not open a brace
scope in source. `enum[Storage](...)` is only a declaration form, not an
expression form for creating enum values inline.

The pack must be named. Positional enum values are invalid because enum member
names are the purpose of the construct.

Semantically, enum members are compile-time exposed values inside the enum
namespace. Each member value must fit the declared storage type. That makes the
storage width part of the source contract and lets the storage type reject
out-of-range values before lowering. The compiler may store enum metadata in
whatever internal representation is best, but the meaning is equivalent to:

```ttx
private Color : enum[Bits_8](
  .red = 1,
  .green = 2,
  .blue = 3,
);
```

producing members that behave like:

```ttx
expose red   : Bits_8 = 1;
expose green : Bits_8 = 2;
expose blue  : Bits_8 = 3;
```

within `Color`.

Enum members lower to constants after type checking. They are not runtime
fields.

Explicit conversion construction is a static dispatch on the destination type:

```ttx
Color -> from(value)
```

That keeps aggregate construction on pack fitting and keeps `enum[Storage](pack)`
reserved for enum declarations. The parser does not need a special conversion
expression form. It parses conversion as the same explicit call syntax used for
all callable dispatch, and the destination type owner checks whether it exposes
a valid `from` function for the source value.

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
private decode_table : Vec[Bits_8, 256] = (
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
literal spellings. Type and layout fitting checks that the target is index-addressable,
that each index is in range, that no index is repeated, and that every value
fits the target element type. Omitted positions use the target element default.

Packs have a carrier order. When a pack is passed, returned, or otherwise
materialized, the pack's field order is the order consumed by the ABI or by the
next compiler stage. Names do not erase that order. Names add semantic mapping
information on top of ordered fields only when they are authored by the pack or
provided by the receiving layout.

Structs and other typed aggregates have concrete layouts. Their declaration
order is the storage and wire layout of the aggregate value. Type and layout fitting may map a
named fluid pack into a typed aggregate by name, but lowering must still emit
the aggregate in the aggregate's declaration order.

For example, this declaration order is `a, b, c`:

```ttx
private Thing : struct {
  expose a : Bits_32;
  expose b : Bits_32 = 0;
  expose c : Bits_32;
}
```

This initializer is valid because the names identify the target fields:

```ttx
state x : Thing = (
  .c = 1,
  .a = 3,
);
```

The source pack carrier order is `c, a`. The constructed `Thing` stores fields
as `a, b, c`, filling `b` from its default. This is an automatic materialization
shuffle from a named fluid layout into a concrete layout. If a function declares
a named return layout ordered as `c, a`, the return carrier order is also
`c, a`; assigning that result to `Thing` requires lowering to shuffle the
returned values into the target aggregate order.

TTX has three explicit ways to produce packs:

1. grouping values with `(...)`
2. swizzling fields with `value.[a, b, c]`
3. slicing a range with `value:[start, count]`.

Those forms are the only source-level decomposition operations. A concrete
typed object inside a pack remains one value until source explicitly swizzles or
slices it back into a fluid pack. Grouping, swizzle, and slice produce
positional packs unless a field in the grouping explicitly authors a name.

Nested positional packs flatten during pack fitting:

```ttx
(1, (2, 3))
```

fits the same positional layout as:

```ttx
(1, 2, 3)
```

Typed values are the boundary. `Vec2D`, `Vec4D`, `Color`, user structs, objects,
and other aggregate values do not implicitly splat into their fields. Source
must access, swizzle, or slice the fields explicitly:

```ttx
(screen_pos, 0.0, 1.0)        // three values: Vec2D, Real_32, Real_32
(screen_pos.[x, y], 0.0, 1.0) // four Real_32 values
```

### Repacking

Repacking means producing a new pack in the carrier order required by the
receiving context. TTX does not need a separate repack operator because pack
construction, swizzle, slice, and named fields already express the operation
directly.

Names are transient during repacking. Swizzle uses names to select fields, but
the selected result is positional. Slice selects by evaluated positions, so its
result is also positional. Grouping positional values keeps positional order.
The only ordinary way to create a named repack result is to author named fields
with `.field = value`, or to fit a produced pack into a declared boundary whose
layout provides names.

Use positional composition when the target wants values in a specific order:

```ttx
state position : Vec3D = (screen_pos.[x, y], z);
```

The swizzle decomposes `screen_pos` into a positional pack, and the outer pack
adds `z`. Nested positional packs flatten during fitting, so the target sees
three values.

Use a named pack when names are the contract:

```ttx
state thing : Thing = (
  .c = source_c,
  .a = source_a,
);
```

The named pack states that the values are semantically `c` and `a`, regardless
of source order. Type and layout fitting maps those names to the target fields and fills any
valid defaults. This is the clean way to add, drop, or reorder values at a
layout boundary when names matter.

Use swizzle or slice when the source is a typed value:

```ttx
state rgba : Color = texture -> sample(uv);
state rgb_alpha : Vec4D = (rgba.[r, g, b], alpha);
state first_two : Vec2D = rgba:[0, 2];
```

Typed values never splat implicitly. The source must say which fields or range
are being repacked.

Returns follow the same rule. A return statement evaluates an expression pack
and fits it to the function's declared return layout. If the return expression
is a swizzle or slice, the expression result is positional. If the declared
return layout is named, that declaration is the boundary that supplies the
returned names visible to callers.

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
   fields must be present; fields with defaults may be omitted only when all
   defaulted fields form a trailing suffix.
9. Positional packs may fit typed aggregates when the pack order matches the
   target layout exactly. Omitted fields are only valid when the omitted region
   is a trailing region of defaulted fields.
10. Indexed packs may fit fixed-size homogeneous aggregates such as `Vec[T, N]`.
    Omitted indexes use the element default. Indexes must be explicit integer
    literals, in range, and unique.
11. Concrete typed values do not flatten into packs implicitly. Source must use
   swizzle or slice syntax to decompose a typed value into a fluid pack.
12. Repack operations produce positional packs unless the syntax explicitly
    authors names or the receiving boundary supplies them.
13. A named pack fitted into a typed aggregate maps by name, then lowers into
    the aggregate's declaration order. A function with a named return layout
    exposes that declared return layout's carrier order at the call boundary.
14. Duplicate field names are representable but should be diagnosed by the
    source, package, or ISA owner that has the useful source location. Name
    lookup reaches the leftmost duplicate, so later duplicates are shadowed.

`Vec[T, N]`, `Vec2D`, `Vec3D`, `Vec4D`, `Color`, user structs, and other
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
source:[0]
source:[0, 4]
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
pack. The call base may be a value, `self`, `package`, a package or type query,
or a future function-pointer value. `IndexStart` parses index or index-slice content.
`SwizzleOp` parses swizzle fields or a swizzle slice. A swizzle or swizzle slice
produces a positional pack. `PackingStart` after a type expression is invalid;
aggregate construction is pack fitting against an expected type, while explicit
conversion is ordinary `-> from(...)` dispatch.

Access forms:

| Syntax            | Meaning                                      |
| ----------------- | -------------------------------------------- |
| `.field`          | field, package member, or type member access |
| `:[index]`        | index access                                 |
| `:[start, count]` | index slice                                  |
| `.[a, b, c]`      | swizzle that produces a positional pack      |
| `-> name(pack)`   | callable dispatch from the left-side base    |

Calls always take a pack. If no arguments are present, the call still formats
as `receiver -> method()`. Static functions use the same syntax:
`Type -> name(args)` or `package -> name(args)`. Function-pointer invocation is
ordinary dispatch on the function-pointer value, such as
`callback -> invoke(args)`.

The dispatch receiver must be concrete: a typed value, a type name, or a
package scope. A fluid pack is not a receiver because it has no concrete type or
addressable identity. To call through a value carried inside a pack, source or
the owning query must first select the concrete entry that is the receiver.

TTX does not use braced initializers. Braces are scopes and statement blocks.
Aggregate initialization uses packs:

```ttx
private values : Vec[Bits_32, 4] = (1, 2, 3, 4);
private color  : Color = (.r = 1.0, .g = 0.0, .b = 0.0, .a = 1.0);
```

The receiving type supplies the expected shape. Positional packs match by
layout order. Named packs match by field name. Source must use swizzle or slice
syntax when it wants to decompose an existing typed value into a pack:

```ttx
state clip_position : Vec4D = (screen_pos.[x, y], 0.0, 1.0);
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
value:[index]
value:[start, count]
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
state total : Count = 0;     // declaration
return total;                 // return
if (total > 0) { ... }        // scope keyword
source -> copy_to(dest);      // expression statement
total += 1;                   // assignment statement
```

Parser shape: statement parsing first skips comments. `ScopeEnd` and
`EndOfStream` end the current block. A modifier starts a declaration. `Return`
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

`if` and `while` take condition packs. The condition owner checks that the
condition fits a truthable layout. Numeric truthiness is not implicit. Use
`Bool` or an explicit comparison.

When an ISA supports `@if`, it can use the same statement shape as `if` while
evaluating the condition as compile-time data.

`match` patterns are expressions or `_`. Each case owns an explicit block.
There is no fallthrough.

`break;` and `continue;` are reserved loop-control statements. The active ISA
decides where they are legal. The syntax layer only preserves the statement
shape for later lowering.

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
The type or layout owner may fit an integer literal into a narrower fixed-width
numeric target only when the value is provably in range. It does not infer among
user-defined types or choose between overloads from a bare numeric literal.

Floating literals follow the same principle: the literal text represents an
exact source value, and the type or layout owner either fits it to the expected
floating target or uses the documented default real type when no narrower target
exists.

`Bytes` literals and embedded-file literals are tokenized as whole literals, but
their fixed prefixes are still source text entries on `Class`:

```text
Bytes    -> 0x[
Embedded -> $[
IndexEnd -> ]
```

The formatter composes those fixed delimiters through `Class` and preserves the
literal payload from the token.

## Documentation

Line comments start with `//`. The tokenizer strips the marker and stores the
comment text as a lexical comment token. Consecutive comment tokens before a
type, member, or function are collected into one `Documentation` object in
source order:

```ttx
// Stored in source order.
// Attached to the following member.
private signature : Vec[Bits_8, 8] = 0x[89 50 4E 47];
```

Comments inside statement bodies are currently skipped by statement parsing
rather than represented as executable statements. Documentation belongs to the
type, member, or function that owns it, and is preserved for formatter and LSP
queries. It does not participate in type identity or layout fitting.

Documentation is intentionally not canonicalized. `AliasName` can carry
documentation for the context that introduced the alias, while
`AliasName.canonical()` reaches the root type and root documentation. A tool may
present the alias name with alias documentation, canonical documentation, or a
stacked documentation view that accumulates each alias layer.

## Canonicalization

Canonicalization is an owned context query. It happens after the relevant source
shape has been evaluated and before queries that depend on resolved types.

It resolves:

- local aliases
- imported package aliases
- builtin type names
- nested type-query steps
- type parameterization
- numeric parameter values.

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
Expected definition `:` but got type
```

Semantic diagnostics prefer ranges that cover the meaningful construct, not
only the token where the compiler first noticed the problem. For example, an
invalid type argument list highlights the argument range when possible.

## Type Kinds

The semantic type space is intentionally small:

| Kind                      | Runtime model               | Notes                                            |
| ------------------------- | --------------------------- | ------------------------------------------------ |
| primitive numeric types   | value                       | fixed width, no implicit widening                |
| `Bool`                    | value                       | only `true` and `false` are truthable by default |
| `Vec[T, N]`               | typed aggregate value       | homogeneous fixed-size aggregate                 |
| `Vec2D`, `Vec4D`, `Color` | typed aggregate value       | named component types with concrete storage      |
| layout                    | structural value stream     | anonymous heterogeneous aggregate                |
| `struct`                  | nominal value               | does not flatten implicitly                      |
| `object`                  | nominal managed value       | heap/reference semantics, ISA-limited            |
| `foreign`                 | external ABI scope          | declarations lower to linked symbols             |
| `Shader`                  | shader scope or ISA concept | may lower to GPU module plus host glue           |
| `enum`                    | compile-time namespace      | members lower to constants                       |
| `alias`                   | compile-time type reference | erased after canonicalization                    |

`Library`, `Shader`, and `Entity` are ISAs. They remain PascalCase type atoms in
source; ISA registration, not the lexer, decides what they mean.

## Relationship To LLVM IR And MLIR

TTX is IR-like, but it is not a textual spelling of LLVM IR and it is not MLIR.
The difference is mostly one of layer and audience.

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
semantics. TTX borrows the idea that semantic domains matter, but makes a
different tradeoff: a host selects an evaluator for a token span, and the syntax
remains a small fixed language rather than an extensible operation syntax.

TTX sits above LLVM IR and beside the lower levels of an MLIR-style pipeline.
It keeps enough human-facing structure to be authored and reviewed directly:
packages, imports, comments, modifiers, attributes, named layouts, packs, scoped
builtins, and shader/foreign declarations. At the same time, it avoids the
open-ended grammar surface of a general programming language. The source is
structured so an evaluator can build semantic shape directly and cheaply.

The architecture also differs in how intermediate state is treated. LLVM IR is
already a lowered representation optimized for analysis and code generation.
MLIR explicitly models many dialect operations and rewrite levels. TTX instead
keeps the authored source tree as the stable carrier for as long as possible and
enriches it with context. Lexical, envelope, module, owned query, ISA, and
compilation contexts are layers over the same program, not permission to create
independent copies of the program's meaning. A host may eventually emit LLVM IR,
SPIR-V, x86_64 code, or another terminal artifact, but those are output
boundaries, not the organizing principle of the source language.

In short:

- Compared with LLVM IR, TTX is higher level, more source-preserving, and more
  domain aware.
- Compared with MLIR, TTX is less extensible at the syntax level but simpler to
  parse and easier to hand-author. A TTX host can still target MLIR if MLIR is
  the right terminal or intermediate artifact for that host.
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
| `const` values                | constants, metadata, specialization inputs               |
| `foreign` functions           | declarations resolved by ABI/linker                      |
| `Shader` package              | shader artifact plus host-side glue                      |

The source-level constructs are frontend contracts. Many disappear during
lowering, but they remain explicit long enough to produce good diagnostics,
check ISA rules, and encode target metadata.

## Design Invariants

When changing TTX, preserve these invariants:

1. The tokenizer assigns a single class to every source spelling.
2. Fixed source spellings live in `Lexical::Class::get_source_text()`.
3. The parser does not backtrack or suppress diagnostics to choose a parse.
4. Assignment remains a statement, not an expression.
5. Parentheses always mean pack.
6. Function parameters, function returns, and `for` bindings all use layouts.
7. Type parameterization is type dispatch over a resolved layout and returns a
   concrete type identity.
8. Named packs start with `.field`; named layouts start with `.field` or
   attributes followed by `.field`.
9. Named and positional aggregate fields do not mix.
10. Visibility and storage intent remain visible in modifier token classes.
11. ISA legality belongs to the ISA/provider that owns the rule, not
    raw parse-shape construction.
12. Formatter output derives from the same token source text as the lexer
    whenever the token has fixed source text.

These rules are what keep TTX readable while still letting it behave like a
compiler IR.
