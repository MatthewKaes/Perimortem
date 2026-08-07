# Library

Library owns CPU language semantics and the reusable native CPU compilation
path. The complete folder builds as `//tetrodotoxin:library`.

TTX provides the shared Abstract, Type, Value, Layout, Addressable, Callable,
Attribute, and Documentation contracts. Library adds only the capabilities and
value domains required by CPU executable languages.

## Library language

`Tetrodotoxin::Library::Language` currently declares the following contracts.

1. Expression, Binding, and Projection provide Library value identities.
   Every authored Expression retains one lexical Anchor containing its full
   Span and the independent Token a diagnostic should emphasize. A synthetic
   Expression omits that Anchor.
2. Operation owns ordered input reachability and Constant evaluation for
   executable value operations.
3. Constant provides bytes, flag, real, signed, and unsigned value domains.
   True and False refine Flag so consumers can select either the shared domain
   or one exact logical value without decoding its storage.
4. Generic provides Access, View, and Fixed materializations.
5. Concrete Bool, signed, unsigned, and real Types provide scalar identities.
6. Static and Self distinguish Callable invocation.
7. Structure provides authored inline Types with ordered fields and nested
   Callables.

These classes retain and expose real TTX Type, Layout, Addressable, and Callable
edges. They do not copy those shared contracts into a Library model.

Binding and Projection currently expose synthetic factories for generated
semantic wrappers only. No production parser constructs an authored Anchor
of either contract. Future grammar must supply an Anchor from its real source
facts rather than treating synthetic absence as source provenance.

Expression centralizes those two construction policies with protected
`create_authored<T>` and `create_synthetic<T>` helpers. Each concrete owner
still publishes its exact typed factory and supplies the builder that can call
its private constructor. The shared helper selects the optional Anchor and
uses `Arena::construct_from` to begin the exact object at its final address, so
provenance cannot be selected through a public constructor.

Expression owns `fold()` as a cached query over the original source node. Its
public result is
`Result<Option<Expression&>, Expression::Error>`. A successful projection is
the direct identity of the selected Constant Expression, while Error carries
its category and exact failing Expression identity. The cache stores only a
settled Constant identity or exact Error without replacing the Expression, its
source facts, or any child edge. Its null state represents either an unread
query or dynamic absence, so missing Type information and a dynamic result
remain retryable.

Operation retains immutable Expression inputs in authored order. It queries
only reached children and propagates a reached child's exact Error unchanged.
Every input is reachable by default. A concrete operation may decide whether
the next edge is reachable only after all earlier reached inputs fold. An
earlier dynamic input keeps later inputs reachable because no concrete decision
exists yet. The concrete owner evaluates only after every reached input supplies
a Constant and must preserve the operation's exact linked result Type.
Interpretation performs no fold query and no parser callback replaces a node.
Function finalization may populate each root cache, but an optional fold failure
does not report a diagnostic or reject an otherwise complete Function.

Generic is Library language semantics rather than a universal TTX category.
It owns an immutable formula contract and its inseparable parameter and
argument vocabulary. The independent Materializations transaction owns the
declared construction state for concrete Access, View, and Fixed Type shapes.
It validates canonical formulas and ordered semantic arguments, retains only
successful exact keys, and rejects nested cycles without giving formulas a
mutable cache.

Static identifies a Callable selected without a receiver. Self identifies a
Callable selected through an addressable value and reserves parameter zero for
that receiver. Those invocation distinctions are not required by Package,
Render, or every other TTX host.

The concrete scalar Types live here because their names and native
representations are Library language policy. TTX retains only the common Value,
Flag, Real, Signed, and Unsigned domain contracts.

`Language::Function` is the concrete Library defined Static Callable. Its
Arena stable identity is reserved from the authored visibility, `func`, and
name prefix. Interpretation enriches that same identity with the complete
Function Span, retained signature source facts, and an authored order View of
Expression roots. The current body grammar accepts comments, `Expression;`
roots, and one optional final `return Expression?;`. Blocks, control flow, and
other statement forms remain unsupported rather than being retained as opaque
tokens. Until semantic linking succeeds the Function resolves to shared TTX
Invalid.

`Language::Signature` owns the complete authored parameter and result grammar.
Its private slots retain exact Type routes, entry and Type Anchors, and optional
authored names without demanding that the Types already exist. Signature
linking resolves those routes, constructs real Parameter Addressables and TTX
Layout projections, and publishes every Function signature before Monograph
body linking begins.

`Language::Field` owns one authored member shared by Library composite Type
systems. It retains visibility, Documentation, exact Type spelling, and Anchors
until linking can construct its real TTX Addressable projection with one exact
Type. `Language::Types::Structure` retains Fields in authored order from
`public|private TypeName : struct { ... }`, and its TTX Structured Layout borrows
their Addressable projections without copying field facts. Supported nested
Callable grammar retains the existing Function objects without copying
Signature, body, Static, or Callable policy. Structure lookup is exact, private
Structures remain local, and finalization rejects public fields or Callable
signatures that expose a private local Structure Type.

`Language::Import` owns one complete `using Package::Route;` statement. It
retains the exact contiguous Type shaped Package local route, triggering
`using` Token, and complete statement Span. Package remains the owner of that
route's lookup grammar and semantic edge.

An Embedded operand gives its complete `$[...]` Token spelling to the exact
source Package context. Library never opens or retains Package Storage. A
resolved `Tetrodotoxin::Language::Resource` supplies only stable bytes;
`Language::Parser::Literal` constructs its own concrete
`Language::Constants::Bytes` by borrowing the complete value and infers
`Fixed[Unsigned_8, byte count]`. The Literal domain must not outlive the
Resource dependency domain. Ordinary Package interpretation places both in the
same Workspace Arena, so the complete base needs no second byte allocation.
Scalar spellings construct the canonical binary wide Library Type for their
domain. Literal accepts no expected Type; the complete Expression supplies its
resulting Type to the receiving owner. A resolved
`Tetrodotoxin::Language::Error` contributes its owner-specific failure context
while Library supplies the current Token Span to the textual Report. Invalid or
another Abstract category remains an ordinary expression mismatch. Contextual
resolution has already completed Package acquisition before Literal receives
the Resource, so Literal-local slicing could not avoid the Storage read.

`Language::Operations::Slice` owns the semantic `:[index]` and
`:[start, size]` operation shared by Embedded, quoted Bytes, hexadecimal Bytes,
and later ranged expressions. It retains only the two or three authored
Expression edges. Fixed, View, and Access receivers supply their retained
element Type directly after linking. Indexing yields that exact element. A
directly authored Constant size selects canonical Fixed element and count
identity during linking even while another input is dynamic. A dynamic size
yields canonical View unless the receiver already proves writable contiguous
Access. An Operation size that folds later retains that already linked View or
Access result Type. Slice never manufactures write capability from Fixed, View,
Bytes, or Constant identity.

Bytes is the current Constant ranged payload domain. Fully Constant Bytes index
and range operations evaluate to canonical Unsigned_8 or exact Fixed Bytes
Constants, including empty and chained ranges. Other legal ranged operations
remain Slice identities rather than implying a universal Constant payload
interface.

`Language::Parser::Expression` constructs one complete source shaped tree. Its
primary grammar admits Literal and Identifier primaries, then selects prefix,
postfix, multiplicative, subtraction, comparison, and equality operations in
the preserved acceptance precedence. Each concrete parser callback consumes
only its owned grammar and constructs one Operation with immutable child
identities. No callback links Types, folds a child, reports an
`Expression::Error`, or replaces the authored tree.

Slice binds before Multiply. Multiply owns binary Signed, Unsigned, and Real
legality and checked integer and IEEE Constant evaluation. Divide and Modulo
share that multiplicative precedence. Divide owns selected Type quotient
evaluation and integer zero handling. Modulo accepts only exact Signed or
Unsigned Types and owns remainder sign, zero, selected width, and signed
endpoint failures. Their semantic legality is established during linking and
their Constant behavior is queried through Expression folding.

The multiplicative level binds before Subtract. Subtract owns only binary
subtraction grammar, locked scalar Type selection, selected width overflow and
underflow checks, IEEE Real evaluation, diagnostics, and folding. Literal
remains the owner of a leading negative numeric spelling. Negate owns general
prefix unary `-` for an exact Signed or Real Type. It rejects Unsigned, checks
the selected Signed minimum, and folds complete IEEE Real values while a legal
dynamic operand retains one Negate operation. Slice binds before Negate, and
Negate binds before the multiplicative level.

Not owns prefix unary `!` at that same prefix level. It accepts only the exact
canonical Bool Type. Complete True and False inputs fold to the canonical
opposite Constant, while a legal incomplete Bool retains one Not operation.
Not performs no truthiness conversion or bitwise interpretation.

Subtract binds before the comparison level. Less owns `<` grammar, Greater owns
`>` grammar, LessEqual owns `<=` grammar, and GreaterEqual owns `>=` grammar.
Each owns locked scalar operand Type selection, canonical Bool result identity,
ordered IEEE comparison, diagnostics, and True or False folding. A following
comparison receives that Bool like any other left operand, so ordinary Type
legality rejects comparison chaining.

Equal owns `==` below the ordered comparison level. Exact identical resolved
Signed, Unsigned, Real, or Flag Types admit scalar equality, while complete
Bytes Constants admit their existing exact Type and payload equality. Each
Constant domain owns its payload comparison, including Real NaN equivalence,
so Equal introduces no tagged value or fitting exception. Complete inputs fold
to canonical Bool. A legal dynamic scalar comparison retains its two real
Expression edges.

NotEqual owns `!=` at the same equality level and accepts exactly Equal's
domains. It delegates the inverse comparison to those Constant domains, so the
NaN, signed zero, and Bytes rules remain one semantic value contract. Complete
inputs fold to canonical Bool while legal dynamic scalar operands retain
NotEqual.

Multiply, Divide, Subtract, Less, Greater, LessEqual, and GreaterEqual require
both operands to resolve to the same Signed, Unsigned, or Real Type identity.
Modulo applies the same exact Type rule to Signed and Unsigned only. Constants
keep their declared Type and receive no implicit widening, narrowing, fitting,
or retagging inside these operations. Scalar literals currently use their
canonical binary wide Type, so a narrower receiving owner must construct an
explicitly typed Constant before forming one of these operations.
Signed and Unsigned operations reject host arithmetic overflow first, then use
`Core::Math::is_representable` to prove the result fits the selected byte width.
Concrete operations do not reproduce that representation arithmetic locally.

Grammar failure remains a parser diagnostic over the exact authored Tokens.
Semantic Type failure is published during linking over the complete operation
Span. An `Expression::Error` remains an exact cached query result and creates no
textual report until a later owner requires a Constant. That owner can use the
retained Span of the failing Expression. A receiving declaration, assignment,
invocation, or other typed operation applies fitting only after the complete
Expression has linked its result Type. Later operations plus Projection remain
outside the current parser.

Resource route, Storage, and Package diagnostics do not enter the Library
graph. An unsliced base Constant owns its complete value, and a Slice fold does
not erase that Constant or any authored edge. A future durable consumer may
choose the cached reachable projection without compiling away the source shaped
graph. No Library payload encoder or restorer currently makes that choice.

## Library Dialect and Monograph

The top level Library Dialect installs into `Environment::Workspace` through the
common `Language::Dialect` interface. Each Workspace owns a distinct stateful
Dialect while Bool, integer, real, and Void remain immutable binary wide
identities. Every Library Monograph resolves those same identities through the
Dialect without publishing them as authored declarations. Typed static Dialect
accessors expose their universal addresses to Library machinery and package
consumers that require exact identity while authored name lookup retains its
packed intrinsic table.

The concrete Library Monograph owns exact local Structure and Function lookup
with separate authored order public views. Local lookup also admits private
Structures and Functions. Each declaration occupies its final Arena address,
so linking enriches the identity already visible through the Monograph.
Duplicate declarations fail before any view changes. Missing names resolve to
the shared TTX Invalid identity.

Function context lookup checks its linked Parameter Addressables first. The
parent Monograph then checks local Structures and Functions, delegates to the
source interpretation context for Package or Workspace names, and finally asks
the installed Library Dialect for intrinsic Types. Structure fields link before
Function signatures, so a signature may name any complete local Structure
without depending on declaration order. Raw incomplete declarations occupy
their names before linking, so later publication enriches those exact identities
and shadowing remains a diagnosed collision rather than a second scope model.

Each Monograph also borrows the exact source local interpretation context and
retains authored Imports in order. Its `link()` transaction requires that
context and each selected target to resolve to real Package Monographs. It
traverses only the target Package's ordered direct member Alias view and
consumes only complete public Functions from direct Library member Monographs.
Private Functions, Dependency Aliases, nested Packages, other Dialect members,
and declarations that a provider imported remain excluded.

The complete candidate sequence is staged before lookup changes. Duplicate
Imports, unresolved targets, incomplete provider Functions, and exact local or
imported collisions reject with no imported entry installed. Successful
imports add borrowed provider Function references only to local lookup. The
public Function view remains the source's own authored publication surface, so
imports cannot become transitive exports.

Interpretation admits ordinary public and private Function definitions. It
constructs real TTX and Library Language identities in the Environment Arena.
Functions retain complete source extents, one Signature source owner, and
authored Expression roots. Imports retain their own triggering Tokens and
complete Spans. Each authored Identifier, Literal, and Operation retains one
Anchor containing its focus Token and full Span, while synthetic Expressions
omit it. None of these facts is a Cursor bookmark, replay model, or cloned
semantic graph. Unsupported declarations and malformed, incomplete, bodyless,
duplicate, or trailing syntax reject the complete interpretation transaction.

The current Function body representation is deliberately narrow. Comments,
ordinary Expression statements, and one optional final return are retained.
Nested blocks, `if`, `for`, and other control flow still lack an executable Body
contract and remain grammar errors. The retained roots are enough for source
queries, linking, and optional fold caching without pretending that control
flow or lowering exists.

The intended first application pressure target is
[`../../apps/ttx/echo`](../../apps/ttx/echo/). Its remaining Library pressure
requires inferred byte Constants, dynamic terminal byte input and output, byte
comparison and concatenation, loop and conditional Bodies, and one Static
Callable selected by App. These requirements belong to later Library and App
slices. The fixture's remaining syntax contradictions must be settled before it
becomes an acceptance oracle.

## Shared CPU compilation

Library compilation may consume completed CPU executable facts retained by
Library, App, or Scene Monographs:

```text
Library Callables -> Library compiler
Scene lifecycle and helper Callables -> Library compiler
App entry and lifecycle driver facts -> Library compiler
Library compiler -> Linker input
```

App and Scene remain their own semantic owners. Library does not convert them
into Library source, reopen their Tokens, or clone their Type graph.

Compilation owns CPU target and ABI planning. It derives sizes, alignments,
offsets, and carriers, makes calling convention and register allocation
decisions, lowers completed executable facts, emits native instructions and
relocations, and diagnoses target decisions.

It does not own package acquisition, Environment lifetime, App or Scene policy,
Shader lowering, final object encoding, or archive assembly.

The accepted native output of compilation is one or more
`Linker::Object::Module` values. Each Module owns a coherent set of sections,
symbols, and relocations. The compiler does not place semantic facts or source
identity in that native terminal.

## Assembler and Linker boundary

The active `Library::Assembler::x86_64` encodes source independent instruction
decisions. Its tests cover instruction bytes and relocation slot shape.

The assembler does not decide Type identity, aggregate shape, calling
convention, lifecycle policy, or publication. A future compiler supplies those
decisions.

Linker owns source independent objects, symbols, relocations, ELF encoding, and
native archive construction. Library produces Linker input but does not absorb
that terminal artifact owner.

## Source free payload

Library owns the opaque payload needed to restore its durable declarations,
Types, values, Callables, Bodies, publication facts, and native symbol locators.
It implements the shared Language persistence dispatch without introducing a
second restored model.

Package owns Archive framing and never learns the Library schema. A fresh
Workspace asks the installed Library Dialect to restore real Library and TTX
identities into its Arena. A restored fact may omit its Anchor when no
authored source exists, but authored graphs retain those source facts for
their complete Workspace lifetime. Cursors, process addresses, transient fold
caches, and machine code are excluded from the durable payload.

No Library payload encoder or restorer exists in the current target.

## Current boundary

The current Library target contains the installed Dialect, declaration
Monograph, binary wide scalar and Void Types, authored Structure Types and
fields, Function signature construction, source shaped Function Expression
roots, reusable Layout grammar, exact
authored Package imports, separate linking and finalization, nondestructive
cached folding, and the x86_64 assembler.

It does not yet contain control flow Body ownership, required Constant
validation, CPU target planning, semantic lowering, or App and Scene
integration.
