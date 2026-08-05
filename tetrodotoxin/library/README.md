# Library

Library owns CPU language semantics and the reusable native CPU compilation
path. The complete folder builds as `//tetrodotoxin:library`.

TTX provides the shared Abstract, Type, Value, Layout, Addressable, Callable,
Attribute, and Documentation contracts. Library adds only the capabilities and
value domains required by CPU executable languages.

## Library language

`Tetrodotoxin::Library::Language` currently declares the following contracts.

1. Expression, Binding, and Projection provide Library value identities.
2. Operation owns recursive folding for executable value operations.
3. Constant provides bytes, flag, real, signed, and unsigned value domains.
   True and False refine Flag so consumers can select either the shared domain
   or one exact logical value without decoding its storage.
4. Generic provides Access, View, and Fixed materializations.
5. Concrete Bool, signed, unsigned, and real Types provide scalar identities.
6. Static and Self distinguish Callable invocation.

These classes retain and expose real TTX Type, Layout, Addressable, and Callable
edges. They do not copy those shared contracts into a Library model.

Operation retains replaceable ordered Expression edges and owns the public
recursive `attempt_fold` transaction. Ordinary Expressions and Constants remain
successful inputs. A partial fold replaces only completed child edges and
retains the parent Operation. Once every input is Constant, only the concrete
operation evaluates its domain. Successful completed results are retained for
idempotent repeated attempts. FoldError carries the failure category and exact
failing Expression identity without retaining parser Tokens or Spans. Every
completed child and operation result must preserve its already selected Type.
Parser construction may eagerly attempt folding. A later Monograph post pass
will traverse replaceable graph roots and invoke the same transaction.

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
name prefix. Until its signature is complete it resolves to the shared TTX
Invalid object. One successful completion installs real parameter and result
Layouts on that same Function and consumes the required balanced definition
body without retaining a Cursor, Token range, source replay record, or
executable Body.

`Language::Parser::Layout` owns the reusable signature grammar. Empty, direct,
unnamed, and named shapes construct real TTX Layouts in authored order. Direct
and unnamed entries retain resolved Type identities. Named entries retain real
Arena owned Alias edges whose resolution reaches those same Types. Qualified
Type routes are resolved by the source local Abstract context supplied by the
Monograph transaction.

`Language::Import` owns one complete `using Package::Route;` statement. It
retains the exact contiguous Type shaped Package local route and no Token,
Span, Cursor, bookmark, or replay state. Package remains the owner of that
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
element Type directly. Indexing yields that exact element. A Constant size
yields canonical Fixed element and count identity during construction even
while another input is dynamic. A dynamic size yields canonical View unless
the receiver already proves writable contiguous Access. If that size is an
Operation which later folds to a Constant, Slice retains the original result
Type rather than performing Type resolution during folding. Slice never
manufactures write capability from Fixed, View, Bytes, or Constant identity.

Bytes is the current Constant ranged payload domain. Fully Constant Bytes index
and range operations evaluate to canonical Unsigned_8 or exact Fixed Bytes
Constants, including empty and chained ranges. Other legal ranged operations
remain Slice identities rather than implying a universal Constant payload
interface.

`Language::Parser::Expression` delegates primary dispatch to Literal and
selects following operators in precedence order. Each concrete operation owns
its complete grammar builder. `Operations::Slice::parse` recursively asks the
Expression dispatcher for its operands, constructs the semantic Slice, and
eagerly attempts folding. Slice binds before Multiply. Multiply owns binary
Signed, Unsigned, and Real grammar, selected Type legality, checked integer and
IEEE Constant folding, and diagnostics after asking the dispatcher only for a
tighter operand. This keeps later operations on their own owners while
Expression gains only one explicit dispatch case.

Multiply binds before Subtract. Subtract owns only binary subtraction grammar,
locked scalar Type selection, selected width overflow and underflow checks,
IEEE Real evaluation, diagnostics, and folding. Literal remains the owner of a
leading negative numeric spelling, while unary Negate remains a separate
pending operation.

Subtract binds before Less. Less owns `<` grammar, the locked scalar operand
Type selection, canonical Bool result identity, ordered IEEE comparison,
diagnostics, and True or False folding. A following `<` receives that Bool like
any other left operand, so ordinary Type legality rejects comparison chaining.

Multiply, Subtract, and Less require both operands to resolve to the same
Signed, Unsigned, or Real Type identity. Constants keep their declared Type and
receive no implicit widening, narrowing, fitting, or retagging inside these
operations. Scalar literals currently use their canonical binary wide Type, so
a narrower receiving owner must construct an explicitly typed Constant before
forming one of these operations.
Signed and Unsigned operations reject host arithmetic overflow first, then use
`Core::Math::is_representable` to prove the result fits the selected byte width.
Concrete operations do not reproduce that representation arithmetic locally.

Every Slice failure maps to one diagnostic over the complete postfix, while
the caller Cursor joins only once after the complete chain succeeds. A
receiving declaration, assignment, invocation, or other typed operation applies
fitting only after the complete Expression has synthesized its result Type.
Remaining comparisons and Projection remain outside the current parser.

Resource route, Storage, and Package diagnostics do not enter the Library
graph. An unsliced base Constant owns its complete value. A folded slice owns
only the reachable Library bytes, so source-free restoration performs no
resource read and retains no unused base Constant.

## Library Dialect and Monograph

The top level Library Dialect installs into `Environment::Workspace` through the
common `Language::Dialect` interface. Each Workspace owns a distinct stateful
Dialect while Bool, integer, real, and Void remain immutable binary wide
identities. Every Library Monograph resolves those same identities through the
Dialect without publishing them as authored declarations. Typed static Dialect
accessors expose their universal addresses to Library machinery and package
consumers that require exact identity while authored name lookup retains its
packed intrinsic table.

The concrete Library Monograph owns exact local Function lookup and an authored
order view of its public Functions. Local lookup also admits private Functions.
Each declaration is reserved and bound at its final Arena address before its
signature completes, so completion never replaces the identity already visible
through the Monograph. Duplicate declarations fail before either view changes.
Missing names resolve to the shared TTX Invalid identity.

Each Monograph also borrows the exact source local interpretation context and
retains authored Imports in order. Its post pass requires that context and each
selected target to resolve to real Package Monographs. It traverses only the
target Package's ordered direct member Alias view and consumes only complete
public Functions from direct Library member Monographs. Private Functions,
Dependency Aliases, nested Packages, other Dialect members, and declarations
that a provider imported remain excluded.

The complete candidate sequence is staged before lookup changes. Duplicate
Imports, unresolved targets, incomplete provider Functions, and exact local or
imported collisions reject with no imported entry installed. Successful
imports add borrowed provider Function references only to local lookup. The
public Function view remains the source's own authored publication surface, so
imports cannot become transitive exports.

Interpretation admits ordinary public and private Function definitions. It
constructs real TTX and Library Language identities in the Environment Arena
and retains no Cursor, Token range, declaration mirror, signature snapshot, or
source replay state. Unsupported declarations and malformed, incomplete,
bodyless, duplicate, or trailing syntax reject the complete transaction.

The exact executable body representation is not implemented. Function body
consumption proves only that one structurally balanced definition exists. The
current target has no active executable Body parser or semantic compiler
transaction.

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
identities into its Arena. Source bytes, Cursors, process addresses, compiler
caches, and machine code are excluded.

No Library payload encoder or restorer exists in the current target.

## Current boundary

The current Library target contains the installed Dialect, declaration
Monograph, binary wide scalar and Void Types, Function signature construction,
reusable Layout grammar, exact authored Package imports, atomic post pass
expansion, and the x86_64 assembler.

It does not yet contain executable body ownership, CPU target planning,
semantic lowering, or App and Scene integration.
