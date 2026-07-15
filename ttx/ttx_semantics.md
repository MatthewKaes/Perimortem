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

Puffer, Tetrodotoxin's reference CLI host, uses a small `Boot` ISA to evaluate
its source preamble before dispatching to another ISA. For that concrete host
model, see [`tetrodotoxin_design.md`](../tetrodotoxin/tetrodotoxin_design.md).

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
- Abstract objects and their registered contracts own the shared TTX semantic
  graph. Type, Alias, Generic, Callable, Free, Self, and Invalid are contracts
  in that graph. Layout owns value and storage shape.
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

## Semantic Object Model

The evaluated semantic model is a directed graph of `Abstract` objects. An
object may be reached through more than one import or alias edge, so its
ownership graph is not forced into a tree and the object does not store one
authoritative parent path. Resolution produces a Route beside the object it
reached.

The base hierarchy is:

```text
Abstract
├── Invalid
├── Type
│   ├── Alias
│   ├── Generic
│   └── ISA-defined types
├── Callable
│   ├── Free
│   └── Self
└── Address
```

These names describe up-castable semantic contracts:

| Contract | Required meaning |
| -------- | ---------------- |
| `Abstract` | object name, registered class, named child queries, and child enumeration by contract |
| `Type` | total canonicalization to `Type&` and a concrete Layout query |
| `Alias` | a named Type that canonicalizes through another Type while retaining its own route and documentation |
| `Generic` | a Type that resolves arguments to a compiler-owned concrete Type |
| `Callable` | complete parameter and result Layouts plus an address/linkage query |
| `Free` | invocation that does not consume an addressable receiver |
| `Self` | invocation whose addressable receiver is parameter zero |
| `Address` | local, external, interpreted, runtime, or explicitly unresolved invocation endpoint |
| `Invalid` | failed resolution with the failed Route, source, and diagnostic cause |

`Type` is not the root of this model. Callable, package, diagnostic, ISA, and
future runtime objects do not inherit Type merely to become queryable. There is
also no `Typed` marker. A consumer asks for the actual contract it needs:

```text
children<Type>()       objects that define types
children<Callable>()   callable objects
children<Self>()       calls requiring a receiver
resolve<Type>(name)    one named Type child
```

Lower layers can operate on `Type` without knowing that an object is an Alias,
Generic, or an ISA-specific subtype. A documentation or language-binding layer
can query the more specific contract through the same object.

### ClassDB

The hierarchy is described by a language-neutral ClassDB. TTX does not depend
on C++ RTTI, C++ vtable layout, or C++ mangled names. Each registered class has:

- a unique, versioned schema Route
- its parent Class
- required inherited operations
- construction and destruction callbacks
- method, property, and callable Layout descriptions
- language or plugin ownership
- documentation and reflection metadata.

Every Abstract exposes its Class descriptor. `is<Contract>()` asks ClassDB
whether that class derives from the requested schema. After that proof,
`as<Contract>()` returns a reference under a checked precondition; failed casts
do not return nullable pointers. Native implementations may use a static cast
after ClassDB proves ancestry. Foreign implementations travel through opaque
object handles and registered C ABI callbacks. A C++ vtable never crosses the
language boundary.

Durable class identity is its readable schema Route, for example:

```text
Ttx1.Abstract.Type.Generic
Ttx1.Abstract.Callable.Self
Shader1.Abstract.Type.Stage
```

A loaded ClassDB may assign dense local indices for fast ancestry and operation
lookup. Such indices are registry-local acceleration only: they are never
serialized, embedded in public symbols, or assumed equal across processes or
languages.

### Routes And Resolution

A Route is the ordered record of named query steps used to reach an object.
Each step records both the name and the contract layer selected for that query.
Names are unique inside each layer. For example, these are distinct routes:

```text
Widget / Callable.Free / open
Widget / Callable.Self / open
```

The ClassDB schema steps are real resolution facts, not `.Type` or
`.Addressable` suffixes manufactured while exporting a symbol. A type or package
receiver selects `Free`. A runtime value canonicalizes its Type and selects
`Self`. Both layers independently reject duplicate `open` children.

Resolution returns the Route and a reference to the resulting Abstract. The
same object can have several routes. An Alias retains the authored route while
canonicalization supplies the Type used for layout and equivalence. Diagnostics
and generated source normally use the authored route. Publication uses an
explicit selected public route. No phase chooses a lexicographically preferred
alias as hidden identity.

Durable names use a reversible route encoding. A textual representation may
print the schema and object segments directly; a binary or symbol encoding may
length-prefix them. Neither form hashes the route or callable signature. If
incompatible package or ABI versions must coexist, version is an explicit route
segment rather than an input to a hash.

### Invalid And Total References

`Invalid : Abstract` is the semantic failure object. It represents missing
names, a wrong requested contract, ambiguous resolution, alias cycles, invalid
archives, unsupported ISA construction, and unresolved linkage when a semantic
object is required. It records the first failed Route, source range, and useful
diagnostic message.

Queries through Invalid are absorbing: they return the same Invalid, or a more
specific Invalid that retains the original cause. This prevents one bad name
from producing a cascade of unrelated failures.

The semantic interface follows these rules:

| Situation | Representation |
| --------- | -------------- |
| failed name or contract query | `Resolution` containing `Invalid&` |
| no children | empty child view |
| unresolved callable address | explicit unresolved Address or `Invalid&` |
| corrupt restored package | Invalid package root |
| valid Alias canonicalization | `Type&` |
| checked contract conversion | reference after ClassDB ancestry proof |

`nullptr` is not a pseudo-Abstract, a failed cast, or an absent semantic edge.
Invalid does not inherit Type, Callable, and every other contract to satisfy
narrow return signatures. Operations that can fail return Abstract or
Resolution. After validation and ClassDB up-casting, narrow operations such as
`Type::canonicalize()` and `Type::get_layout()` are total and reference-based.

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
Type, and an offset computed under the parent layout policy. The layout records
its concrete size and alignment. A scalar is the terminal case: a concrete
layout with no entries. A struct, vector, view, callable carrier, or other
composite uses the same recursive fact shape with entries. If target choice can
change storage, the Compiler resolves the target's concrete Layout before
terminal lowering; the backend does not replace the Type with an unrelated ABI
enum.

Every Type admitted to a value or lowering position supplies a concrete Layout
after canonicalization. Alias delegates that query to its canonical Type. A
bare Generic is a valid compile-time query receiver but is not admitted to a
value position; parameterization must first produce a concrete compiler-owned
Type. This is the complete interface needed by a lower compiler layer:
canonicalize the validated Type, inspect its Layout, recursively lower each
non-empty entry, and ask the terminal Type contract how an empty layout is
represented.

Callables are not layout entries. A Layout describes shape and storage. An
Abstract scope publishes Callable children. Callable signatures reference
Layouts for argument and result shape, while method lookup resolves a Free or
Self object through the concrete typed receiver or package scope.

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
   start with `.field`. Named layouts start with `.field` or attributes followed
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
dialect : Package;
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
| `Package` | public package export surfaces                                     |
| `Render`  | stage-oriented render package authoring and host render contracts  |
| `Shader`  | shader definitions and shader-specific host glue                   |

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
import Graphics     : Package = Perimortem.Graphics;
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
- a package name such as `Perimortem.Graphics`.

Package names decode as `Type("." Type)*`. Empty segments, lowercase starts,
double dots, and trailing dots fail through the ordinary token cursor because a
dot must always be followed by a `Type`. A successfully decoded package name can
be used directly as a package cache key and package folder name.

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

Type references are progressive Abstract queries constrained to the Type
contract. The object does not own one string path, but each successful
Resolution retains the Route it walked:

```ttx
Bits_32
Vec[Bits_8, 4]
Graphics::Image
Math::Matrix[Real_32, 4, 4]
```

The first `Type` token is resolved immediately from the active context. Each
operator then dispatches on the current object or value:

- `:: Name` queries a nested Type on the current Abstract.
- `.name` queries a layout/member on the current value or `Layout(type)`.
- `-> name(...)` queries callable dispatch.
- `.[...]` dispatches to swizzle/repack semantics.
- `:[...]` dispatches to index or slice semantics.

`::` is therefore not string concatenation. `Graphics::Color` means resolve the
local type/package name `Graphics`, then ask that object for the Type named
`Color`. A type query advances one level per operator. The Route is compact
transaction-owned data rather than a fresh general-purpose string, and it lets
the owner report the error at the exact failed step.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric type arguments, such as the `4` in `Vec[Bits_8, 4]`, are
part of the type query and are checked while proving that query.

Parameterization is Generic dispatch over resolved arguments. A type reference
such as `View[Bits_8]` resolves `View`, proves that it implements `Generic`,
resolves `[Bits_8]`, and asks the Generic to create or find the concrete Type.
A Type that is not Generic produces Invalid at that exact step. This keeps
parameterized types such as `View[T]`, `Vec[T, N]`, `List[T]`, and
package-owned forms in the same up-castable Type hierarchy rather than a
separate template system.

The concrete Type returned by parameterization is compiler-owned. Its stable
handle is used for local equivalence, member lookup, nested Type lookup,
Callable lookup, and Layout queries. Its durable identity is its Route. The
parameter layout is an input to construction, not a second semantic model.

`alias` creates an `Alias : Type`. Lower compiler layers can canonicalize it and
forget the distinction. Documentation and reflection layers can still query
the Alias contract, route, and authored documentation.

A resolved type may also be stored as a compile-time value whose type is the
standard `Type` type:

```ttx
const reflected_type : Type = Graphics::Image;
```

The initializer stores the resolved Type handle and its Route, not source
spelling. This gives reflection, diagnostics, and editor inspection a
value-system foundation while preserving the same canonical identity used by
ordinary type queries. A terminal that materializes Type values uses the
ClassDB-compatible Abstract handle defined by that runtime. Compiler pointers
and C++ vtables are never the public representation.

Decode shape: a type reference starts with `Type`, or a numeric token when
decoding a numeric type argument. `TypeAccessOp` performs one
`resolve<Type>()` step on the current Abstract. Type arguments start with
`IndexStart`, contain type references separated by `PackingOp`, and end with
`IndexEnd`.

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

`Perimortem.Graphics` is explicit. A package that needs graphics-domain types
imports it and refers to those types through the import name:

```ttx
import Graphics : Package = Perimortem.Graphics;

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
surface without forcing one language-wide policy. The lexer owns the spelling.
The active ISA owns the meaning.

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
@builtin @slot(0) .source : View[Bytes]
@stage(fragment)
@binding @set(0) @slot(1)
```

Parser shape: an attribute starts with `Attribute`, whose token text includes
the leading `@`. It may stand alone as a marker or take one scalar value inside
`PackingStart` and `PackingEnd`. Scalar values may be text, unsigned or signed
integers, real numbers, or booleans. Structured values are not part of the
attribute model. Write another attribute when a declaration needs another
fact.

Attributes are not runtime values. They are consumed by the compiler or
forwarded into target metadata.

Some ISAs may consume directive-style attributes as standalone statements, but
package identity is not one of them. Package names are compiler configuration
because build systems such as Bazel require output paths to be declared before
source evaluation runs. The Package ISA body only describes what the package
exports. Package imports still use the authored package-name surface.

Known shader ABI attributes have fixed local targets. The owning ISA checks
their legality while it evaluates the declaration:

- `@stage(name)` applies to `Shader` definitions in `Shader` packages.
- `@push_constant` applies to exposed `foreign` ABI blocks in `Shader`
  packages.
- `@binding`, `@set(N)`, and `@slot(N)` describe separate resource facts on
  members inside `Shader` scopes.
- `@builtin(name)` and `@builtin @slot(N)` apply to shader builtin input members
  or layout fields.
- `@shader_type(Type)` applies to Library types that deliberately lower as a
  named shader ABI type. Backends must use this metadata or canonical type
  identity. A matching member layout does not imply shader ABI identity.

Other attributes may remain target-specific metadata until an ISA defines
their legality rules.

`@if` is an attribute-shaped directive. A host or ISA may reserve that spelling
for compile-time control flow:

```ttx
@if(enabled) {
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

Functions are explicit callable syntax:

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
`Func`. Evaluation publishes a `Callable : Abstract` carrying the stable
signature used for lowering, entry-point discovery, specialization, foreign
declarations, and shader compilation. Source bodies enrich that same object
through ISA-defined contracts owned by the selected body evaluator.

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

Callable has two registered subtypes:

- `Free` is selected by a type or package receiver.
- `Self` is selected by a runtime value and consumes that receiver.

The subtype is the semantic classification. Consumers do not infer it from a
parameter name, a boolean flag, or the table in which a plain Function happened
to be stored.

```ttx
public Counter : struct {
  public func identity[.value : Count] -> Count {
    return value;
  }

  public func identity[self] -> Counter {
    return self;
  }
}

const reflected_type : Type = Counter;
state counter : Counter;
state scalar  : Count   = Counter -> identity(41);
counter = counter -> identity();
```

The first call resolves `Free/identity` beneath Counter. The second resolves
`Self/identity` beneath Counter and passes `counter` as the declared receiver.
Identical names are legal because the receiver selects a real ClassDB contract
layer before name lookup. Duplicates within one layer are rejected.

In the Library dialect, bare `self` is shorthand for an ordinary typed member
whose name is `self` and whose type is the owning Type. It causes the Callable
object to implement `Self`. The member is part of the function signature rather
than an implicit compiler local. The parser requires it first.
`Self::get_parameters()` returns the complete effective layout, including this
receiver at position zero. Reflection, generated bindings, pack fitting,
register allocation, and ABI lowering all consume that layout. A Self call
contributes its receiver as the first value, appends the authored argument pack,
and fits the complete sequence. No consumer prepends or slices a second
call-site layout. Root package functions cannot use this shorthand because a
package is not an addressable runtime value.

A callable placed under a Type is type-owned; that does not make “typed
function” another subtype. `Free` and `Self` describe invocation semantics.
Ownership is carried by the Resolution route.

The split is also a tooling contract. Completion after a type token enumerates
Free children. Completion after an expression result enumerates Self children
of the inferred canonical Type. The tokenizer can choose the first route for a
bare type name, while semantic typing handles values produced by member access,
indexing, or earlier calls.

Callable publishes an address query because a declaration may be unresolved,
interpreted, compiled locally, restored from a package, or supplied by another
language runtime. A resolved call receives an Address Abstract with the
appropriate linkage contract. An unresolved external receives an explicit
unresolved Address or Invalid. It never receives `nullptr` and never derives a
linker name by hashing its signature.

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

Enums use a storage-typed brace scope:

```ttx
private Color : enum[Bits_8] {
  red = 1;
  green = 2;
  blue = 3;
}
```

Parser shape: after `Define enum`, require exactly one type argument naming the
enum storage type. Then require `ScopeStart` and parse named case assignments or
enum-owned function declarations until `ScopeEnd`. Positional values are
invalid because enum member names are the purpose of the construct.

Semantically, enum members are compile-time exposed values inside the enum
namespace. Each member value must fit the declared storage type. That makes the
storage width part of the source contract and lets the storage type reject
out-of-range values before lowering. The compiler may store enum metadata in
whatever internal representation is best, but the meaning is equivalent to:

The cases produce members that behave like:

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

That keeps aggregate construction on pack fitting. The parser does not need a
special conversion expression form. It parses conversion as the same explicit
call syntax used for all callable dispatch, and the destination type owner
checks whether it exposes a valid `from` function for the source value.

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

Packs are source-IR expression nodes for in-flight value groups. A pack has a
fluid layout: it carries value order, optional names, and element types, but it
has no runtime object identity or address of its own unless it is fitted into a
concrete receiving context such as a call, return, assignment, attribute,
layout, or aggregate type.

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
`c, a`. Assigning that result to `Thing` requires lowering to shuffle the
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
[@builtin @slot(0) .source : View[Bytes], .count : Count]
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
   fields must be present. Fields with defaults may be omitted only when all
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

Executable syntax belongs to the ISA that gives it meaning. The shared TTX
Abstract graph supplies stable declarations, layouts, routes, and identities.
Library lowers authored expressions directly into immutable compiler bodies
owned by Callable objects. Shader currently publishes Shader-owned blocks and statements
until it has an equivalent graphics execution interface. Neither path adds body
tags or generic statement nodes to the shared TTX model.

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
parser consumes `SubOp` as unary negation. After an expression it consumes the
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
produces a positional pack. `PackingStart` after a type expression is invalid.
Aggregate construction is pack fitting against an expected type, while explicit
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
as `receiver -> method()`. A type or package receiver resolves a Free callable.
An addressable receiver canonicalizes its inferred Type, resolves a Self
callable, and contributes the declared receiver parameter. Function-pointer
invocation is Self dispatch on the callable value, such as
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
@if(enabled) { ... }
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

| Source                   | Meaning                                     |
| ------------------------ | ------------------------------------------- |
| `123`                    | decimal numeric literal                     |
| `0xFF`                   | hexadecimal numeric literal                 |
| `0.5`                    | floating literal                            |
| `"Raw string"`           | string literal, no implicit null terminator |
| `0x[AA FF 12 45 ACDE]`   | byte data literal                           |
| `$[path/to/file]`        | embedded file data                          |
| `true`, `false`          | `Bool` literals                             |

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

Byte literals ignore whitespace and pair hexadecimal digits from left to right.
Whitespace is a visual delimiter rather than data, so `0x[AA FF 12 45 ACDE]`
produces `AA FF 12 45 AC DE`. Quoted strings decode their escape sequences and
produce the resulting bytes without an implicit null terminator.

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
`AliasName.canonicalize()` reaches the root Type and root documentation. A tool
may present the authored Resolution route with alias documentation, the
canonical route with canonical documentation, or a stacked view that
accumulates each alias layer. There is no `display_name` identity substitute:
the Route already records how the object was reached. Presentation does not
participate in canonicalization, type equivalence, layout equivalence, or layout
fitting.

## Canonicalization

Canonicalization is the total operation on a validated Type. It happens after
the relevant source shape has been evaluated and before queries that depend on
resolved types.

It resolves:

- local aliases
- imported package aliases
- builtin type names
- nested type-query steps
- type parameterization
- numeric parameter values.

Canonicalization may return a different Type object, but it does not mutate or
discard the Resolution route. The formatter and diagnostics use the authored
route; compiler analysis uses the canonical Type's stable local handle and
Layout. Alias cycles are rejected during graph construction, so a valid
`Type::canonicalize()` returns `Type&` rather than a nullable result.

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
| `alias`                   | compile-time Type subtype   | canonicalizes while retaining authored route     |
| `Type`                    | compile-time type value     | stores resolved Type handle and Resolution route |

`Library`, `Package`, `Render`, and `Shader` are ISAs. They remain PascalCase
type atoms in source. ISA registration decides what they mean instead of the
lexer.

## Relationship To LLVM IR And MLIR

TTX is IR-like, but it is not a textual spelling of LLVM IR and it is not MLIR.
The difference is mostly one of layer and audience.

| Area                   | LLVM IR                                             | MLIR                                                          | TTX                                                          |
| ---------------------- | --------------------------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------ |
| Primary representation | Lowered SSA module/function/block/instruction IR    | Extensible operation/SSA IR with regions                      | Source IR plus token bytecode and context layers             |
| Main extension point   | Intrinsics, metadata, passes, and targets           | Dialects define operations, types, attributes, and interfaces | Fixed syntax with ISAs defining evaluation, legality, and metadata |
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

Terminal lowering is driven by Type and Layout facts, not by source attributes
that secretly encode one compiler enum. For every parameter, result, SSA value,
field, or stored value, a lowerer performs the same operation:

```text
canonical Type
-> concrete Layout
-> non-empty Layout: recursively lower entries in layout order
-> empty Layout: query the terminal Type contract for this target
```

A non-empty Layout is never collapsed to an invented scalar carrier merely
because a backend recognizes the outer Type name. A byte view, vector, struct,
render contract, and user aggregate are recursively deconstructed according to
their entries. The source value remains one semantic aggregate; the terminal
representation is its ordered projection. Calls, returns, stack placement,
register classification, generated host declarations, and archive descriptions
must consume the same projection.

An empty Layout alone does not say whether a terminal is an integer, real,
opaque handle, zero-width value, or target-defined resource. The canonical Type
must implement the terminal contract registered by the active ISA or target.
An ISA can enrich the Abstract graph with that target meaning. A lower compiler
only knows the Type and terminal interfaces; it does not need to know whether
the Type was an Alias, Generic, shader scalar, or foreign-language object.

Lowering therefore must not depend on an authored `@abi` number, a global
`Abi::Lowering` switch, a C++ type name, or a pointer-keyed side table. Those
forms duplicate semantic facts outside the Abstract graph and become wrong as
soon as a new ISA or language runtime contributes another terminal contract.

## Design Invariants

When changing TTX, preserve these invariants:

1. The tokenizer assigns a single class to every source spelling.
2. Fixed source spellings live in `Lexical::Class::get_source_text()`.
3. The parser does not backtrack or suppress diagnostics to choose a parse.
4. Assignment remains a statement, not an expression.
5. Parentheses always mean pack.
6. Function parameters, function returns, and `for` bindings all use layouts.
7. Type parameterization proves the Generic contract over resolved arguments
   and returns a concrete compiler-owned Type.
8. Named packs start with `.field`. Named layouts start with `.field` or
   attributes followed by `.field`.
9. Named and positional aggregate fields do not mix.
10. Visibility and storage intent remain visible in modifier token classes.
11. ISA legality belongs to the ISA/provider that owns the rule, not
    raw parse-shape construction.
12. Formatter output derives from the same token source text as the lexer
    whenever the token has fixed source text.
13. Every semantic object implements Abstract and exposes its registered Class;
    Type is a queryable subtype, not the universal base.
14. Failed semantic resolution returns Invalid, never a null pseudo-Abstract.
15. Free and Self are Callable subtypes, and Self's complete parameter Layout
    contains the receiver at position zero.
16. Resolution retains a reversible Route. Canonicalization does not erase the
    authored route, and publication never substitutes a signature hash.
17. Composite terminal lowering recursively deconstructs every non-empty
    concrete Layout before asking terminal Type contracts to lower leaves.

These rules are what keep TTX readable while still letting it behave like a
compiler IR.
