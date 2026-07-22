# TTX Language Semantics

TTX is a frontend source IR language. It is authored by humans, but it stays
close to the compiler's semantic model. A TTX file makes package boundaries,
storage shape, addressability, layout, and lowering intent visible in the
source. Tetrodotoxin uses TTX as its default frontend, but the TTX source IR can
also serve as an interchange boundary for another host. Token bytecode can cross
that boundary only when both sides implement the exact concrete Lexer contract
that assigned its Codes.

For high-level language design and usage, see
[ttx_design.md](ttx_design.md). This document is the canonical contract for TTX
semantics. When implementation and this contract disagree, the implementation
is migration work rather than precedent. The document explains how each
language concept is tokenized, evaluated, represented, and checked by the owner
that knows the rule before lowering. Incomplete derived C++ contracts must not
be guessed into the Abstract base.

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
builds a module graph, and code generation can continue through Type, Layout,
Dialect, ABI, provider, and compilation queries. Fast compilation is part of that
goal, not an afterthought. Every syntax feature must justify its parse cost,
recovery cost, and downstream context cost.

To make that work, the syntax carries a large amount of semantic information
directly. Casing separates addressable names from type names. Ordered definition
modifiers keep publication separate from runtime storage and compile-time
evaluation. The fixed `alias` keyword has its own lexical Code. Dialect-owned
definition words such as `struct`, `object`, `enum`, and `foreign` are lowercase
addressable spellings whose meaning belongs to the active evaluator. They are
not PascalCase type references. Packs, layouts, access chains, and assignment
statements all have distinct local shapes.

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
  Environment binding, Abstract identity resolution, Type proofs, Dialect rules, pack
  fitting, and addressability checks all depend on program context.

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

## Token Bytecode And Dialect Evaluation

TTX source text is the human-authored source IR. A concrete Lexer lowers that
source IR into TTX token bytecode. Every emitted token carries one `Lexical::Code`
that identifies the prescribed semantic grouping for its source span. Examples
include `Type`, `Addressable`, `Assign`, `TypeAccessOp`, `AddressOp`,
`CallOp`, modifiers, attributes, and fixed operators.

Each Code is stored in 8 bits. `Terminal` reserves `0x00` and `Unknown` reserves
`0xFF`. Every other numeric value and semantic grouping belongs to the concrete
Lexer contract and may be remapped whenever that contract changes. A Code stream
therefore cannot be serialized, exchanged, or interpreted independently of the
exact Lexer contract that produced it. Payload-bearing tokens retain the source
location needed to inspect their authored text.

The bytecode is not a fixed-width instruction stream. A token is the smallest
decoded unit, but a Dialect instruction is whatever span the active Dialect
fetches and decodes from the cursor. Some instructions consume one token. A package export,
layout, function declaration, or control statement may consume many tokens. A
string or byte literal is a single token whose payload can be large. The useful
hardware analogy is closer to a variable-width instruction stream than a
one-token-per-instruction machine, but the important rule is simpler: a concrete
Lexer emits one exhaustive stream of prescribed semantic slices, and the active
Dialect owns the decode length and evaluation rule for the bytecode it accepts.

TTX does not define a canonical Dialect set. It defines source IR, token
bytecode, and the shared facts that a Dialect can use. Which Dialects evaluate
that bytecode is left to the toolchain that hosts TTX.

Tetrodotoxin's active package Container evaluates the small source envelope
(`dialect : Name;`) before handing the remaining token range to the selected
body Dialect. For that concrete host model, see
[`tetrodotoxin_design.md`](../tetrodotoxin/tetrodotoxin_design.md).

The useful mental model is:

```text
TTX source IR
-> lexical token bytecode
-> optional envelope evaluation
-> optional Environment injection
-> optional Dialect evaluation
-> lowering, tooling, or interchange output
```

A Dialect is a named model, not a backend target, parse-tree visitor, evaluator
callback, or VM record. It decodes the token bytecode it owns, validates the
rules for that authoring domain, and makes real TTX facts queryable: Types,
Layouts, package exports, Callable facts, ABI facts, shader facts, or other
context.

Environment assembly, package loading, cache validity, and Dialect selection are host
concerns. TTX describes the bytecode and shared data model those systems execute
against.

## Monotonic Context Layering

A host that consumes TTX can be small or large. It might only tokenize source
for highlighting, execute a private Dialect for embedded scripting, or run a full
compiler pipeline. The durable object is still the same source program and token
bytecode with progressively enriched context.

```text
TTX source IR
-> lexical token bytecode
-> optional host envelope context
-> optional Environment context
-> optional Dialect context
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
then choose how much more context it wants: an envelope evaluator, an
Environment, one or more Dialects, Type and Layout queries, ABI facts, or backend
lowering. Compilation asks those contexts questions rather than rediscovering
source intent from raw text.

This model is close to attribute-grammar and query-based incremental compiler
systems. The important distinction is ownership:

- TTX source owns authored bytes and source spans.
- Lexical owns Code assignment and token payload views.
- Abstract objects and their virtual contracts form the shared TTX semantic
  graph. Type, Alias, Types::Generic, Expression, Constant, Callable, Static, Self,
  Addressable, Writable, and Invalid are contracts in that graph. Layout is the
  fundamental identity-free fitting contract over an ordered group of those
  real objects. Documentation is the fundamental borrowed authored-prose value.
- Host envelope evaluators own whatever source preamble they choose to execute.
- Environment, module, package, cache, and invalidation layers belong to the host
  that needs them.
- Dialects own the bytecode spans they understand and the facts they
  make queryable from those spans.
- ABI, provider, and backend queries expose derived answers without cloning the
  program into a second semantic tree.
- Terminal emitters own output-specific representations such as SPIR-V words,
  LLVM IR, object files, archives, generated headers, editor JSON, or formatted
  source text.

## Semantic Object Model

The evaluated semantic model is a directed graph of `Abstract` objects. An
object may be reached through more than one Environment binding or Alias edge, so its
ownership graph is not forced into a tree and the object does not store one
authoritative parent path. Resolution passes the remaining borrowed
`View::Bytes` directly through the objects it reaches. It does not allocate or
persist a parallel path graph.

The Concept layer and Abstract hierarchy are:

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
│   │   ├── Ttx::Model::Types::Managed
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
├── Layout
└── Ttx::Model::Body
```

`Ttx::Concept` owns foundational contracts and values that have no narrower
semantic owner. Abstract supplies semantic identity, contextual lookup, and a
stable Documentation query. Layout supplies shape and fitting without identity.
Documentation preserves borrowed authored or generated prose. `Ttx::Model` owns the semantic mechanisms built from those
concepts. Model is a namespace and source-IR ownership boundary, not a central
registry or a second graph.

Only the Abstract branch is queried through `is()` and `assume()`. Layout and
Documentation remain ordinary identity-free contracts or values:

| Contract        | Required meaning                                                         |
| --------------- | ------------------------------------------------------------------------ |
| `Abstract`      | object name, documentation, identity redirection, and context resolution |
| `Alias`         | closed local name, accumulated documentation, and redirection            |
| `Exports`       | ordered public definition edges paired with contextual name resolution   |
| `Documentation` | borrowed ordered prose with no semantic identity                         |
| `Layout`        | identity-free ordered shape, directional fitting, and fitting evidence   |
| `Type`          | resolved semantic identity with a total Layout query                     |
| `Managed`       | Type proof that values are runtime-managed object references              |
| `Generic`       | instruction contract that resolves arguments to a concrete Type          |
| `Expression`    | one evaluatable value with a result Type query and ordered input Layout  |
| `Constant`      | immutable Expression already in normal form with value equality          |
| `Projection`    | Expression selecting one Addressable from one receiver Expression        |
| `Binding`       | Expression giving one underlying Expression an authored flow name        |
| `Callable`      | complete parameter and result Layouts                                    |
| `Static`        | invocation selected without a runtime receiver                           |
| `Self`          | invocation whose addressable receiver is parameter zero                  |
| `Addressable`   | named address to typed data whose `get_type()` supplies Type or Invalid  |
| `Writable`      | Addressable assignment capability with a stable non-writable projection  |
| `Invalid`       | absorbing failed resolution                                              |
| `Body`          | identity-free executable blocks, values, and operations                  |

Terminal Types add narrow storage and domain contracts:

| Contract   | Required meaning                                              |
| ---------- | ------------------------------------------------------------- |
| `Terminal` | Layout leaf with value width, byte size, alignment, and prose |
| `Unsigned` | non-negative integer domain                                   |
| `Signed`   | signed integer domain                                         |
| `Real`     | floating-point domain                                         |
| `Flag`     | two-value logical domain                                      |

`Unsigned_8` is the common semantic byte element. TTX has no separate `Byte`
or scalar `Bytes` Type alias because that would give the same stored value two
semantic identities and duplicate every consumer's numeric handling. The
`Constants::Bytes` contract instead names a byte-array literal domain. Its
resolved collection Type is distinct from its `Unsigned_8` element Type.

The first narrow native contracts are deliberately reference based:

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

The Type queries may return Type or Invalid. Layout itself stores no
member record. It exposes ordered Abstract references. Structured narrows those
entries to the actual Addressable objects in a Type. Their names, queried child
Types, documentation, attributes, defaults, and Dialect facts remain on those real
objects or on richer contracts they implement. Consumers ask an Addressable
for its Type; address identity does not resolve away into Type identity. No
nullable reference is part of the Layout interface.

`Types::Managed : Type` is the narrow semantic proof that a value is a managed
object reference rather than an inline value. It says nothing about tracing
algorithm, pointer width, storage class, target address space, allocator,
moving policy, or object header. Those are runtime and target Representation
decisions. A concrete Library `object` Type proves Managed; an inline struct,
vector, choice, or range Type does not. A Dialect such as Shader rejects a
Managed Type unless an explicit representation contract supplies a legal
non-managed target Type.

`Types::Void : Type` is the result Type of invoking a Callable whose result
Layout is empty. It keeps the invocation represented in an executable Body
when the Callable produces no values but may still mutate state, invoke Foreign
code, or perform another observable side effect. Void is not inserted into the
Callable's result Layout and is not storage, an empty aggregate, runtime
absence, or semantic failure. An optimizer may remove a Void invocation only
after independently proving that the invocation has no observable effects.

`Addressables::Writable : Addressable` is the narrower proof that assignment
may target an address. It does not add a mutability boolean to every
Addressable, prescribe a store instruction, or confuse address access with the
Type of the loaded value. The active Dialect still owns evaluation and lowering
of a successful write.

`Writable::get_read_only()` returns a stable real Addressable edge with the same
name, documentation, and resolved Type that does not prove Writable. The
concrete producer owns that projection and its connection to underlying
storage. It cannot be an Alias resolving back to the writable identity because
that would restore the capability the projection exists to withhold.

Contiguous semantic collections store `Concept::Reference<Contract>`, a
non-null borrowed reference value. It preserves the object it receives.
Consumers call `resolve()` explicitly when they need represented identity, so a
Structured Layout retains its real Addressables while a Generic argument keeps
the resolved `const Type&` identity selected by the parser.

### Contract Proof And Narrowing

TTX contract inheritance is queryable without C++ RTTI, a ClassDB, a global
type-number allocator, or a hash. Every declared semantic contract owns a
stable 128-bit `Perimortem::System::Uuid`. Its implementation recognizes that
identifier and delegates unrecognized identifiers to its direct base contract.
The work is therefore two word comparisons per shallow inheritance level and
requires no allocation or dynamic registry lookup.

Pure construction and query paths remain `constexpr` when every participating
object is available during constant evaluation. This includes contract proof,
alias resolution, fixed terminal facts, borrowed layouts, and dependency-free
fit rules. Runtime-owned caches and evaluator logic remain ordinary functions
when exposing them would move responsibilities into the public contract. The
binary-wide Invalid and empty Comment accessors also remain runtime boundaries
so every linked consumer observes the same canonical objects.

Contract identifiers identify interfaces only. They never identify an Abstract
object, replace its name, form a resolution route, select an export symbol,
version a package, or become a serialized object handle. Object identity inside
one stable DAG remains its address. Durable identity remains a reversible chain
of names. An incompatible change to a contract's required operations receives a
new contract identifier rather than silently changing the old meaning.

Native consumers use a predicate and an invariant-only narrowing operation:

```text
abstract.is<Type>()     -> Bool
abstract.assume<Type>() -> const Type&
```

`is<Contract>()` asks the object to prove the declared contract and its base
chain. `assume<Contract>()` checks that proof and returns a reference. Its name
states that the caller expects the contract proof to succeed; asking it to
convert an unproven contract is a caller invariant violation. Fallible semantic
work first returns `Abstract&`, checks the desired contract, and returns Invalid
from its own query boundary on failure. It never uses a nullable cast result.
Every native `implements()` override may report only contracts that are public
C++ bases of that object, which keeps the checked reference conversion valid.

The first native model has one semantic inheritance spine per object, matching
the hierarchy above. A scalar can therefore be proven and viewed as its full
`Type`, as `Terminal`, and as exactly one of `Unsigned`, `Signed`, `Real`, or
`Flag`. C++ implementation inheritance manufactures no additional facts beyond
the contracts explicitly declared on that spine.
Foreign-language bridges may expose the same stable contract identifiers
through an adapter that implements the corresponding native contract, but C++
vtables and foreign object layouts do not cross that ABI boundary.

`Type` is not the root of this model. Callable, diagnostic, Dialect, and future
runtime objects do not inherit Type merely to become queryable. There is no
`Typed` marker. A named context inherits Type only when it represents a
type-like semantic surface, not merely because it participates in lookup. The
common Abstract resolution operations remain:

```text
abstract.resolve()             resolve represented identity
abstract.resolve_context(route) resolve borrowed bytes in this context
```

Every Type answers its named context through the ordinary Abstract query.
`Type -> name` requires the selected Callable to prove Static. `value -> name`
resolves the value's Type and requires the selected Callable to prove Self.
Receiver form does not create a second TTX lookup interface. A missing name or
wrong Callable contract returns Invalid.

An Alias is resolved before a consumer proves the resulting contract. Lower
layers can therefore operate on `Type` without knowing whether the authored
query passed through an Alias, Generic construction, or a Dialect-specific
context. Tooling that cares about the authored Alias inspects it before
resolution. Reflection, schema description, and cross-language configuration
must also be represented as Abstracts in the graph. They are not a repository
or a mandatory field on every Abstract.

### Progressive Resolution

Resolution passes borrowed `View::Bytes` directly to the queried Abstract. That
view is the route. An Abstract may interpret it atomically, consume a prefix,
pass a sliced suffix, or forward the unchanged view into a different context.
There is no separate route or resolution-state object in the semantic model.

The common protocol preserves the useful split from the original TTX model:

```text
resolve()                redirect represented identity
resolve_context(route)   resolve remaining bytes in this object's context
```

Both operations are public virtual queries. The receiving Abstract owns all
lookup and slicing policy. A small context may compare names directly. A named
Source model may use a table for its definitions. A Type may index its named
Callables directly. These are object-owned lookup policies rather than modes
hard-coded into a central lookup layer.

The following rules are the ground truth for this contract:

| Query rule          | Required behavior                                                                                           |
| ------------------- | ----------------------------------------------------------------------------------------------------------- |
| local name          | `get_name()` names the current Abstract, including an authored Alias                                        |
| canonical name      | call `resolve().get_name()` when the represented identity's name is required                                |
| identity            | `resolve()` returns a real Abstract reference and is idempotent for an unchanged valid DAG                  |
| route ownership     | `resolve_context(route)` receives the entire borrowed view and the current Abstract owns its interpretation |
| route partitioning  | chained queries and one combined route are not required to be equivalent                                    |
| context redirection | a context may forward an unchanged route while selecting a different context                                |
| empty route         | `resolve_context("")` has no base-defined relationship to `resolve()`                                       |
| determinism         | the same ordered query chain from the same Abstract returns the same final identity until the DAG changes   |
| termination         | a valid constructed DAG cannot redirect a resolution chain forever                                          |
| failure             | failure returns an Invalid Abstract reference, never a null pseudo-Abstract                                 |

These rules permit a context to select the lookup representation appropriate to
its domain without weakening reproducibility. They do not require a universal
separator, traversal algorithm, child container, or resolution cache.

Alias overrides `resolve()` to return its target's resolved identity and
`resolve_context(route)` to continue through that resolved target.
`Palette::Color` can therefore consume
`Palette` in a source context and `Color` in the aliased Type without
manufacturing or rewriting an intermediate path object.

Alias is closed around local redirection. It owns its local name, a stable
Documentation view, and a borrowed target reference. That view presents local
lines first and then the target's visible lines. Alias chains therefore
accumulate authored context without copying or changing target Documentation.
Documentation does not participate in either resolution query. Alias does not own Type behavior, layout, attributes,
diagnostics, package membership, or route history. The graph owner guarantees
target lifetime and rejects alias cycles before an Alias becomes queryable.

Names remain unique inside each lookup surface. Static and self callables may
share a name because the receiver chooses the Type-owned surface before lookup.
An export owner walks its selected public ownership chain and writes each name
reversibly. Resolution itself does not store an allocated path, hash a
signature, or choose a lexicographically preferred alias.

### Exported Definition Surfaces

`Exports : Abstract` is the optional contract for a durable public definition
surface. It makes the same graph both resolvable and enumerable without
prescribing a Namespace, Source, Package, Type, container, or route grammar.
The contract supplies `get_export_count()` and total `get_export(index)`
queries. Every valid index returns the real exported Abstract edge in authored
publication order; an out-of-range index returns Invalid.

Every exported edge has a unique non-empty local name, and resolving that name
directly in the export owner's context returns the same edge. Alias remains
visible at that boundary so documentation and authored naming survive;
consumers call `resolve()` only when they need canonical identity. Imports,
private definitions, storage Layout, and locator records are not exported
unless the owner deliberately publishes a real edge for them.

Direct lookup is closed over the enumerated surface. A name resolves from an
Exports context exactly when one indexed edge owns that name. Nested lookup
begins only after selecting an exported edge. A dependency can therefore bind
one local Alias without searching or copying the producer's private source
closure.

This contract is the dependency product boundary. A source dependency binds an
Alias to the Exports object produced by its selected root Dialect. A package
dependency binds the same Alias shape to a Package that also proves Exports.
Nested package groups, interpreted packages, restored packages, archivers,
reflection, and tools consume that contract without learning the producer's
backing mechanism. Resolver identity and dependency location remain on their
actual higher-layer owners.

### Durable And Evaluation Contexts

Core TTX defines no universal Group, Namespace, Source, module, or package
contract. Durable containment is a host domain, not an operation every TTX
consumer needs. A host may define an Abstract that uses its own indexed
resolution policy and optionally proves Exports when its public roots are
enumerable. Consumers walk that object through `resolve_context()` and prove
the narrower contract only when they need enumeration. Containment therefore
does not acquire a fabricated Type or Layout.

Tetrodotoxin, for example, defines Namespace as one concrete Exports owner for
authored package grouping and Source as a top-level evaluated source unit.
Those contracts live in Tetrodotoxin because source files, archives, and
package surfaces earn them. Other TTX hosts are not required to use either
representation.

Evaluation state may retain a private index of parameters, loop bindings, and
local definitions as a block advances. That index is an evaluator detail, not a
TTX concept or semantic node. Before inserting a declaration, evaluation checks
the complete active context. If the authored name already resolves, the
declaration is rejected. Nested blocks may add names and remove them on exit,
but they never shadow a name that remains active.

Package and owner state are not implicit lexical bindings. Source addresses them
through an explicit root such as `package.value`, `self.value`, or another named
Abstract context. This keeps local lookup unambiguous without manufacturing a
Scope object or a second resolution interface beside Abstract.

### Invalid And Total References

`Invalid : Abstract` is the semantic failure object. It represents missing
names, a wrong requested contract, ambiguous resolution, rejected alias-cycle
construction, invalid archives, unsupported Dialect construction, and unresolved
linkage when a semantic object is required. The source-owning caller retains the
authored route, source range, and presentation context. Invalid does not grow a
second diagnostic state model. A successfully constructed Alias is therefore
always acyclic.

Queries through Invalid are absorbing and always return the binary-wide Invalid
object. Invalid construction is private, so semantic owners cannot create or
store local failure sentinels. This prevents one bad name from producing a
cascade of unrelated failures. Invalid is closed and stateless. It never owns
the failed route, source range, message, or a specialized failure subtype. The
source-owning query retains those facts.

The semantic interface follows these rules:

| Situation                       | Representation                                          |
| ------------------------------- | ------------------------------------------------------- |
| failed name or contract query   | `Invalid&`                                              |
| no children                     | empty child view                                        |
| unresolved ABI linkage          | representation owned by the ABI or execution contract   |
| corrupt restored package        | Invalid Abstract in place of the archived Source root   |
| Type-targeting Alias resolution | resolved `Abstract&`, followed by a Type contract proof |
| checked contract conversion     | reference after semantic contract proof                 |

`nullptr` is not a pseudo-Abstract, a failed cast, or an absent semantic edge.
Invalid does not inherit Type, Callable, and every other contract to satisfy
narrow return signatures. Operations that can fail return `Abstract&`, which
may refer to Invalid. After identity resolution and semantic validation, narrow
operations such as `Type::get_layout()` are total and reference-based.

The Abstract contract is heavily restricted but not declared closed. A new
operation belongs on Abstract only when it is fundamental to every semantic
object and cannot be expressed by another Abstract contract. `abstract.hpp` is
the heart of TTX and should remain roughly 150 lines or fewer. Growth toward a
registry, type container, package model, diagnostic store, traversal state, or
other derived concept is an architectural smell.

### Expression And Constant Facts

Expression is the shared contract for one evaluatable value. Its identity is
not its result Type. `resolve()` therefore remains ordinary Abstract identity
unless a specific type-producing expression deliberately redirects to a
materialized object. `get_type()` returns the Type currently proven for the
value or Invalid, and `get_inputs()` returns the Layout of values needed to
evaluate it. The active Dialect owns operator legality, executable bodies, parsing,
evaluation, and diagnostics.

Pack syntax produces grouped value flow directly through Layout. It does not
earn an Abstract identity, carrier object, or second `get_layout()` contract.
An evaluator returns one Expression, one identity-free Fluid, Named, or
Composite Layout, or Invalid according to the consuming context. The Layout
retains the real Abstract entries, so grouping never turns every Type Layout
into a value object and never creates a parallel list of values. Expression
dependencies are also an identity-free Layout because an operand list is not
itself an authored multi-value result.

Projection and Binding are concrete Expression facts. Projection retains one
receiver Expression and one selected, durable Addressable, then publishes the
Addressable's Type through `get_type()`. It is therefore the right expression
for a named field or constant structured position, but not a placeholder for a
runtime array position that has no distinct Addressable in the semantic DAG.
A Dialect that supports uniform indexed values owns that indexed Expression and
retains its receiver and index operands there. Binding retains one authored
name and one underlying Expression. It exists so Named flow contains real
named Abstracts instead of parallel name and value arrays. It is not a symbol
table, binding phase, declaration, storage edge, or resolution context.

`Expression::fits(target)` is the value-to-Type seam used by Layout fitting. An
ordinary expression fits only the exact resolved Type returned by `get_type()`.
A narrower expression contract may prove a safe contextual fit from additional
facts. This keeps conversion knowledge on the value domain without turning
Layout into a numeric conversion table.

Constant is an immutable Expression already in normal form. It has an empty
input Layout and defines value equality, but it does not define a parser,
operator set, evaluator, folding pass, or lowering representation. The shared
constant domains are open up-castable contracts:

| Contract              | Payload       | Contextual fitting rule                        |
| --------------------- | ------------- | ---------------------------------------------- |
| `Constants::Unsigned` | `Unsigned_64` | any Unsigned Type that can represent the value |
| `Constants::Signed`   | `Signed_64`   | any Signed Type that can represent the value   |
| `Constants::Real`     | `Real_128`    | exact resolved Type in the first slice         |
| `Constants::Flag`     | `Bool`        | any Flag Type                                  |
| `Constants::Bytes`    | `View::Bytes` | exact resolved Type in the first slice         |

Constant equality requires the same domain contract, the same resolved Type,
and the same payload. Two independently allocated constants with those facts
are equal. Real NaNs compare as one semantic value so equality remains a valid
cache equivalence relation. Constant identity is never collapsed to Type
identity. TTX has no native String constant. Quoted source text is decoded to
bytes, and a language that wants String semantics builds its own Type and
operations from that data.

Exact decimal source text may remain a Dialect-owned literal Expression until an
expected Type chooses a floating format. `Constants::Real` represents a value
that has already been evaluated into `Real_128`. A future folding facility can
be another narrow contract over expressions that can prove a Constant. It does
not require an evaluation method on every Expression.

## Executable Body

`Model::Body` is the common identity-free executable value produced after a
Dialect has consumed authored source. It is not an Abstract, AST, parse tree,
scope, cursor snapshot, or second Type graph. The concrete Callable, Shader
Stage, or App lifecycle owner retains the complete Body and supplies the
semantic identity around it.

A Body is an immutable set of compact tables:

- blocks name ordered operation ranges and their explicit terminators;
- body-local value IDs name parameters, locals, computed values, and
  projections;
- operations retain body-local IDs and only the graph edges required by their
  shape. Operand and result Types live once in the Body value table;
- aggregate construction and projection preserve semantic Layout order;
- loads and stores refer to real Addressable identities, and stores require a
  Writable proof;
- calls refer to real Callable or explicit intrinsic owners rather than copied
  signatures or domain-specific universal opcodes;
- branches, merge structure, loops, and returns use block IDs rather than
  replayable source positions.

The common alternatives cover structural operation shapes such as constants,
aggregate construction, calls, loads, stores, unary and binary values,
branches, and returns. A binary operation records one Dialect-owned bytecode
and exactly two body-local operands. TTX does not define what `+` means or
manufacture an Abstract for each operand Type pair. The active Dialect proves
the ordered `(left Type, right Type) -> result Type` rule before publishing the
Body. Texture sampling, managed allocation, and Foreign linkage remain calls
to their real semantic owners. Adding one of those domains does not add a
central semantic Kind or a parallel expression tree.

Every result-producing operation declares its result Type and produces one
fresh body-local value ID. All operand, value, block, and graph edges are
validated before the Body is published. IDs are dense within the Body, have no
meaning outside it, and are the only Body facts an archive stores directly;
semantic graph edges use Package definition IDs. Source tokens and Cursor
state are consumed construction inputs and never survive as executable
authority.

## Layout Facts

`Concept::Layout` is the shared fitting contract over an ordered group of real Abstracts.
It answers how many objects are present, which Abstract is at an index, whether
this source shape fits a target shape, and which source Abstract supplies each
target slot. An invalid index or fit returns Invalid. Layout is not an Abstract,
a member container, a Type registry, a storage record, or a lifecycle state.

The five v1 contracts are deliberately separate classes:

- **Fluid** is the positional fitting view exposed by grouped values, swizzles,
  slices, argument packs, and intermediate returns. It exposes the actual
  Abstract values in production order. Fluid fitting compares resolved
  identity in that order.
  When a source entry implements Expression, it instead asks that value whether
  it fits the resolved target Type.
- **Named** is reshapeable value flow whose actual Abstracts author non-empty
  names. Named fitting requires unique source names and matches each name and
  resolved identity exactly once in the target. An Expression may supply the
  same value-to-Type fitting proof after its name matches. Target order is
  independent.
- **Structured** is the stable shape supplied by a resolved Type. Its entries
  are the actual Addressable objects in the semantic DAG. Structured fitting
  preserves those identities. Two separately authored fields do not become the
  same field merely because their spellings and child Types happen to match.
- **Ranged** is a compact homogeneous shape. It stores one real Abstract and a
  count, then returns that same answer for every index inside the interval and
  Invalid outside it. Fixed `Unsigned_8[N]` and `Static::Vector<T, N>` Types can
  therefore participate in recursive projection without allocating N copied
  edges.
- **Composite** is positional composition over two complete Layouts. It routes
  indexes and target segments to the owning child, preserving each child's
  fitting rules without copying its Abstract entries. Composite may recursively
  contain another Composite, so composition does not require a flattened Fluid.

Fitting is directional: `source.fits(target)`. Fluid and Named values may fit a
Structured target without acquiring Type identity themselves. A Structured
value does not silently decompose into value flow. Source uses swizzle or slice
syntax to make that transition explicit.

`source.get_fitted(target, target_index)` returns the original source Abstract
that supplies that target slot. A failed fit, invalid index, or missing mapping
returns Invalid so the source owner can retain the diagnostic. Fluid,
Structured, Ranged, and Composite preserve positional order. Named exposes the
permutation it proved by name and resolved identity. Composite delegates this
evidence to the child that owns the requested position. This is fitting
evidence, not a copied member or an allocated mapping. A caller can construct
target-ordered value flow without repeating Named matching.

Layout does not copy a field's name, Type, documentation, attributes, default,
storage, or Dialect metadata into a generic entry. The Abstract supplies its own
name and represented identity. Structured supplies the stronger guarantee that
the object is the real Addressable edge owned by the Type. Additional facts stay
on that object, its source owner, or a narrower derived contract. There is no
`Member` model object and no optional-name/default bit embedded in Layout.

`Type::get_layout()` returns `const Concept::Layout&`. A Terminal has an empty
Structured Layout and directly publishes its target byte size and alignment.
An authored aggregate usually exposes Structured Addressables; lowering asks
each Addressable for its Type before recursing. A fixed homogeneous Type may
instead expose Ranged and answer every valid index from one repeated Type.
Storage offsets and aggregate size and alignment are derived from those facts.
Carrier, register class, calling convention, and wire policy remain later
compiler queries. None of these are Layout fields.

Target Representation is a derived compiler product over real Type, Layout,
Addressable, Callable, Body, and Dialect facts. It may cache target-local IDs,
offsets, alignments, storage classes, descriptor bindings, register classes,
and ABI carriers for one compilation. Those records are neither Abstract
identities nor Layout entries, and an archive never treats them as the
semantic source of truth. Terminal bytes may be archived as derived products
only together with the semantic and versioned Dialect facts required to prove
their meaning.

A Type-targeting Alias is resolved before the Type contract is proved. A
Generic is a compile-time instruction rather than a Type or value.
Parameterization must produce a resolved Type whose Layout can be queried.
Layout identity alone never substitutes for Type identity.

Callable parameters and results use the base Layout contract because signatures
may use any concrete Layout. Static and Self are Callable objects found through
their owning contexts, not Layout entries. Self includes its receiver at
parameter zero, so fitting, reflection, invocation, and lowering all consume the
same complete signature.

Core fitting requires the objects presented to it. A source language or Dialect
that supports omitted defaults resolves those defaults on the real Addressable
objects and constructs the complete value flow before asking Layout to fit.
This keeps default policy out of every layout consumer.

### Resolution, Completion, And Invalidation

Source loading may require preregistration for recursive Types, aliases,
mutually visible Callables, Environment bindings, or Dialect-specific declarations. That is a
host construction technique, not a universal publication lifecycle.

A host may reserve a stable, nonmoving Type or containing context before all
facts are known. While the requested fact is incomplete, the object or system
resolves that query to Invalid. Once the owner has enough information,
resolution reaches the real Type and `get_layout()` is a total Layout
reference. Layout has no `Incomplete` kind, and an empty Structured Layout is
never overloaded to mean "not ready."

Different hosts may complete the graph differently. One may enrich a reserved
object after a declaration pass. Another may replace an enclosing context or
source system. A third may build an immutable snapshot. Abstract, Type, Layout,
and Callable prescribe none of those policies. They prescribe only that real
query results are references and failed or premature queries resolve to
Invalid.

The Abstract determinism rule is scoped to an unchanged DAG. The owner that
changes or replaces a DAG also owns reference lifetime, cache invalidation, and
any revision or epoch used by its readers. Process addresses may be local
identity while that owner keeps them stable, but they are never durable names.

Exporting a public symbol, archiving a package, and presenting an editor
snapshot are optional host concerns. They may require their own validation and
immutable product, but a Type does not become semantically real by being
published. Internal Types, transient compiler Types, runtime-provided Types,
and partially loaded systems all participate through the same resolution
contract. Unavailable facts resolve to Invalid.

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
-> optional Environment or module context
-> optional Dialect evaluation
-> optional owned queries
-> optional terminal output
```

These names are examples, not required TTX stages. TTX does not have a broad
semantic pass whose job is to reinterpret ambiguous syntax after parsing. The
source already carries the semantic category of each construct, and lexical
analysis lowers those categories into token bytecode. Later layers, when a host
has them, connect, type-check, cache, and lower already-shaped facts by
enriching the available context.

There is no required replacement IR layer for checks. Type, Layout, Dialect,
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
   may use no envelope at all, or it may use a small Boot evaluator to collect
   documentation and the selected Dialect name.
3. **Environment or module context**: attach package, local-product, module, or
   FFI state when the host needs more than one source unit.
4. **Dialect evaluation**: execute the token spans owned by the selected
   Dialect and expose TTX facts from that evaluation.
5. **Owned queries**: expose Type, Layout, Dialect, provider, ABI, and
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

When a host needs multiple source units, its container selects them explicitly
and injects their completed products through Environment. TTX does not require
C or C++ style text inclusion or one global package graph. A host can bind
native ABI facts, editor data, generated packages, or another language runtime
as long as the boundary makes the required facts available through resolution.

A program is ready for lowering when the host has proven the facts its output
requires. For a compiler this may mean every expression has a concrete value
type, every grouped value Layout has fitted an expected shape or remains in a
context that permits grouped flow, every access chain has a typed result, every
call has a selected callable target, and every statement has the type facts
needed for lowering.
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

- Layout decoding looks one token after `[` to distinguish a named layout field,
  an unnamed layout, an empty layout, or a direct value type.
- Pack decoding selects named-pack mode only from a leading `.` field. Any other
  expression-starting token selects positional-pack mode.
- Member decoding recognizes `modifier func` so function declarations do not
  look like ordinary value declarations.
- Access-chain decoding looks after `.` to decide whether there is a field
  access. If the next token is not a valid field name, postfix evaluation stops.
- The expression evaluator reads `:[` as the value index or slice operator,
  keeping value indexing distinct from layout/type argument brackets.
- Statement evaluation decodes an expression once. If the next token is an
  assignment operator, the decoded expression is checked to see whether it is an
  assignable address chain.

Those decisions are local and do not backtrack. This is important because TTX
is intended to have an authoritative formatter, useful diagnostics, and a
compiler pipeline that does not depend on hidden parser guesses.

A conforming evaluator follows these rules:

1. Let token Codes carry the first layer of meaning.
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

## Lexical Codes

The tokenizer does more semantic work than a minimal lexer would. This is
intentional. The evaluator sees Codes such as `Addressable`, `Type`, `Func`,
`public`, `Attribute`, and `SwizzleOp` directly.

Important lexical distinctions:

| Source shape                | Token Code             | Semantic meaning                                    |
| --------------------------- | ---------------------- | --------------------------------------------------- |
| `snake_case`                | `Addressable`          | runtime names, fields, functions, local values      |
| `PascalCase`                | `Type`                 | type names, aliases, Dialect names, package names   |
| `enum`, `struct`, `foreign` | `Addressable`          | Dialect-owned definition forms, not type references |
| `alias`                     | `Alias`                | fixed Abstract-redirection definition               |
| `public`, `private`, etc.   | modifier tokens        | Dialect-owned publication or evaluation intent      |
| `@name`                     | `Attribute`            | metadata attached to members, params, fields        |
| `@if`                       | `Attribute`            | directive owned by a Dialect or host                |
| `break`, `continue`         | control keyword tokens | loop-control statements                             |
| `0x[...]`                   | `Bytes`                | byte data literal                                   |
| `$[...]`                    | `Embedded`             | embedded file literal                               |
| `_`                         | `Discard`              | wildcard or ignored value                           |

`Lexical::Lexicon::get_spelling()` owns fixed source spellings separately from
Code semantics. The Lexer and source emitters share that mapping rather than
duplicate strings for keywords, operators, delimiters, byte literal prefixes,
embedded literal prefixes, and marker tokens.

## Source Envelopes And Dialects

Many TTX hosts use a source envelope so a complete file can declare which
instruction set should evaluate the remaining bytecode:

```ttx
dialect : Library;
dialect : Render;
dialect : Shader;
dialect : Package;
```

Envelope shape: full-file evaluation starts at the reserved lowercase `dialect`
keyword, requires `Define`, then requires a PascalCase Dialect name and
`EndStatement`. Subtree evaluators, embedded tools, or foreign hosts may start
below this envelope level when they already know which evaluator should execute
the token bytecode.

The source spelling names a `Tetrodotoxin::Model::Dialect` resolved from the
host's ordinary `Dialects` Abstract context. A host with a different context may
reject a source another host accepts. No registry or process callback becomes
the semantic identity.

The Dialect name is not a globally reserved keyword. `Package`, `Library`,
`Shader`, and other Dialect names remain Type atoms outside the header, so Type
access such as `YourType::Package` is still valid syntax.

The Dialect controls which builtins, attributes, Types, address spaces, runtime
features, and body instructions are legal in the source.

A Dialect controls source presentation, accepted builtins, evaluation,
legality, and its additional versioned facts. Its durable result participates
in the one shared semantic graph. A Dialect may reject or narrow a common
construct, but it may not reinterpret an existing common contract. In
particular, Environment binding makes another identity available without
importing that identity's Dialect builtins or legality rules.

| Dialect   | Purpose                                                            |
| --------- | ------------------------------------------------------------------ |
| `Library` | general reusable code, binary formats, data transforms, host logic |
| `Package` | public package export surfaces                                     |
| `Render`  | stage-oriented render package authoring and host render contracts  |
| `Shader`  | shader definitions and shader-specific host glue                   |
| `Scene`   | reusable managed state, lifecycle, render roots, and typed outcomes |
| `App`     | process lifecycle, Scene composition, and target selection policy  |

`alias` is the fixed definition keyword. Dialect-owned lowercase spellings such as
`object`, `struct`, `enum`, and `foreign` are definition forms only when the
active evaluator assigns them that meaning. None of them are Dialect names. For
example:

```ttx
dialect : Library;

private Header : struct {
  public width  : Unsigned_32;
  public height : Unsigned_32;
}
```

The active Dialect may reject a construct that is syntactically valid. Once the host
has prepared whatever envelope, Environment, or module context it requires, the
selected Dialect owns body evaluation and may reject constructs that do not belong
to that authoring space. `Render` and `Shader` packages do not accept managed
runtime concepts such as `object` or `List` unless those Dialects explicitly define
how they lower.

A Dialect is not necessarily a single backend. `Render` and `Shader` packages
may produce GPU code, such as SPIR-V, and host-side code that loads those
constants, builds pipeline layouts, and bridges them into the Perimortem
runtime. Backend outputs such as SPIR-V, x86_64, generated headers, or Vulkan
bridge code are compilation targets, not separate Dialects.

### Scene State And App Composition

A Scene is a reusable managed state owner inside an App. It owns its state
Layout, ordinary lifecycle Callables, render-root Addressables, and declared
typed signals. The lifecycle roles are `enter`, `frame`, and `exit`. Their
meaning is recorded as direct Callable edges; a runtime never discovers them
by searching for a conventional function name.

A Scene does not select the next Scene and does not terminate the containing
App directly. Its frame Callable returns a real `Scene::Flow` value which
either keeps the current Scene active or emits one signal owned by that Scene.
The App owns the initial Scene and the complete transition table from
`(Scene, signal)` to `replace`, `push`, `pop`, or App exit behavior. Transition
targets are direct Scene identity edges after evaluation, never source paths or
runtime strings.

A signal is a Scene-owned semantic identity with a complete payload Layout.
`signal finished;` has an empty payload. A future declaration such as
`signal selected[.item : ItemId];` carries one fitted value without creating a
second event Type system. `Scene::Flow` is an ordinary closed runtime value
containing Stay or an emitted signal coordinate and its fitted payload. A
compiled runtime may use owner-local ordinals, but the semantic and durable
edges remain the real Scene and signal identities.

This ownership permits a state-machine cycle without a Source dependency
cycle. If Splash emits `finished` and Title emits `shift_pressed`, the App may
map Splash to Title and Title back to Splash. Neither Scene queries or imports
the other. Both real Scene owners exist before the App composition is
evaluated, and the App contains the transition edges. The archive graph is
therefore an ordinary finite owner graph rather than mutually recursive export
surfaces or forward declarations.

A replacement transition is transactional. Runtime completes the current
frame, calls the old Scene's `exit`, releases its external resources,
constructs and roots the new Scene state in the worker Realm, calls its
`enter`, and only then publishes it as current. Failure follows the App's
normal cleanup guarantee and does not expose a partially entered Scene.

Scene render roots use the same Render contracts and explicit App-owned
Render-to-Shader bindings as App roots. Graphics receives evaluated Render
values from the active Scene set; it never acquires Scene, Source, or lifecycle
concepts. A stacked Scene policy may keep lower Scene state alive while
choosing independently whether those roots update or render.

## Container Environments

For hosts that use the common package container, exact external Packages and
explicit member Sources are selected before any Source is evaluated:

```ttx
resolve Graphics : Perimortem.Graphics = "2.2";
source ImageLibrary : Library = "graphics/image.ttx";
```

The canonical `package.ttx` order is fixed:

```text
optional descriptor Documentation
dialect : Package;
zero or more resolve declarations
zero or more source declarations
Package-Dialect export body
```

Puffer's package Descriptor parses this prefix, resolves the named Dialect
objects, and records the exact resolutions, ordered members, Documentation, and
remaining body token. The package container uses those facts to complete the
shared Environment and declared members. Descriptor evaluation then hands the
verified same Source and remaining token position to the real Package Dialect.
The descriptor root and every declared member form the explicit Source vector
used to construct the source-backed Package.

The resolution belongs to the package container rather than a Source preamble.
It starts with `Resolve`, then a PascalCase local name, `Define`, a
package name, `Assign`, a quoted canonical Major.Minor version, and
`EndStatement`:

```ttx
resolve LocalName : Package.Name = "Major.Minor";
```

The version is a `String` token decoded directly as two independent unsigned
components. It never enters the Float or Real model. `"0.1"` is valid and
`"0.0"` is the unset version. Leading zeroes are noncanonical and rejected so
accepted text has one stable round trip. Package names decode as
`Type("." Type)*`. Empty segments, lowercase starts, double dots, and trailing
dots fail through the ordinary token cursor because a dot must always be
followed by a `Type`.

The host resolves every declaration to one exact anonymous Package and builds a
shared Environment. Repeating the same exact dependency under another Alias
does not duplicate the Package edge. Reusing an Alias for another resolution or
claiming the same name and version for a different Package is invalid.

The container also assigns local names and expected Dialects to member files:

```ttx
source LocalName : DialectName = "path.ttx";
```

The expected Dialect checks the selected file before its body is evaluated.
Every selected Source borrows the same Environment. Source owns no membership,
file, or Package edge; the container passes the complete member vector to the
source-backed Package explicitly.

Environment names participate in type queries and value access after the host
has bound them. Availability does not import Dialect semantics. A `Shader`
Source cannot make `object` legal merely because a Library product is present.
Resolved definitions must still be valid in the active Dialect.

There is no `using` or wildcard binding syntax. Names remain explicit in the
container so reviews and diagnostics can see package and file boundaries.

## Names And Type Queries

TTX uses casing as a semantic boundary:

- `snake_case` names are addressable runtime values or fields.
- `PascalCase` names are Types, aliases, package names, or Dialect names.

Type references are progressive Abstract queries whose final result must prove
the Type contract. The object does not own one string path. Each queried Abstract
receives borrowed `View::Bytes` and may interpret them atomically, pass a sliced
suffix, or redirect them unchanged without allocating a second representation:

```ttx
Unsigned_32
Vec[Unsigned_8, 4]
Graphics::Image
Math::Matrix[Real_32, 4, 4]
```

The first `Type` token is resolved immediately from the active context. Each
operator then dispatches on the current object or value:

- `:: Name` queries a nested Type on the current Abstract.
- `.name` queries a layout/member on the current value or the resolved Type's
  `get_layout()` result.
- `-> name(...)` queries callable dispatch.
- `.[...]` dispatches to swizzle/repack semantics.
- `:[...]` dispatches to fixed-shape index or slice semantics.

`::` is therefore not string concatenation. It establishes a nested context
query whose receiving Abstract owns the lookup grammar. An evaluator may offer
the remaining `Graphics::Color` bytes as one view or issue ordered queries at
the source operators. Those forms are not required to be equivalent. The final
result must prove Type. The source owner retains the authored input and the
operation being resolved so it can report the exact failed boundary.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric type arguments, such as the `4` in `Vec[Unsigned_8, 4]`, are
part of the type query and are checked while proving that query.

Parameterization is Generic dispatch over resolved arguments. A type reference
such as `View[Unsigned_8]` resolves `View`, proves that it implements `Generic`,
resolves `[Unsigned_8]`, and asks the Generic to find the concrete Type. An
Abstract that is not Generic fails at that exact parser step. The parser
validates the declared argument kinds and proves the returned Type contract.
Generic is not a placeholder Type and does not have a Layout of its own.

Each named Generic is a formula such as `Vec`, `View`, or `Dict`. The current
source context owns formula lookup; a toolchain may also expose a closed
immutable builtin table. The Generic object owns its ordered parameter
signature, materialization rule, and concrete-Type cache. TTX does not define a
mutable Generic registry or switch on formula names.

`get_parameterization()` returns the complete ordered signature before any
argument is consumed. Every entry is `Type`, `Unsigned_64`, or `Bool`. The
parser validates the authored tokens against that signature and supplies
`find()` a borrowed ordered view of
`Union<const Type&, Unsigned_64, Bool>`. This union is a compact call carrier,
not an Abstract or a second semantic graph. A Type alternative preserves the
resolved semantic identity as a const reference. Unsigned and boolean
alternatives are direct compile-time values.

The formula compares Type arguments by resolved identity and scalar arguments
by value. Unrelated Types with the same local name remain distinct. The ordered
argument view is the complete formula-local cache key; it never includes a
parent pointer, authored route, formatted Type name, or hash.

Missing formula lookup returns Invalid at the owning Abstract context. Once
parsing begins, a wrong contract, malformed list, rejected argument shape, or
rejected value produces a source diagnostic and `Utility::None`; parse failure
is not inserted into the Abstract graph. The parser can therefore recover at a
statement or scope sequence point without manufacturing an Invalid Type.

The concrete Type returned by parameterization is compiler-owned. Its stable
handle is used for local equivalence, member lookup, nested Type lookup,
Callable lookup, and Layout queries. A host that exports it derives a durable
name by walking a selected named ownership chain. The compact argument view is
an input to construction, not another semantic model. The source owner
retains the authored route for diagnostics. A materialized Type may add a
formula-specific contract such as `View::Type`, allowing consumers to ask
`is<View::Type>()` without treating the `View` generator as a Type. A
publication owner selects and renders a public ownership chain independently of
local cache identity.

`alias` creates an Alias Abstract that preserves the authored local name and
documentation while redirecting to its resolved target. When that target is a
Type, a Type consumer calls `resolve()`, proves the Type contract, and then
forgets the Alias. Documentation tools may inspect the Alias first to retain
that local source context.

A resolved type may also be stored as a compile-time value whose type is the
standard `Type` type:

```ttx
const reflected_type : Type = Graphics::Image;
```

The initializer stores the resolved Type reference, not source spelling or an
allocated path. Diagnostics retain the authored source query separately.
Publication walks an explicitly selected named ownership chain. A terminal that
materializes Type values uses the explicit Abstract contract defined by that
runtime. Compiler pointers and C++ vtables are never the public representation.

Decode shape: a type reference starts with `Type`. `TypeAccessOp` continues
progressive context resolution. The receiving Abstract owns route
interpretation. When `LayoutStart` follows a Generic, the parser walks the
formula's signature and accepts a nested Type reference, decimal or hexadecimal
Unsigned_64, or `true`/`false` for each corresponding parameter. `PackingOp`
separates arguments and `LayoutEnd` closes the list. The completed query proves
the materialized object's Type contract.

## Terminal Registration And Source Contexts

Core TTX defines no package contract. A host may expose a Source, module,
package, or another Abstract context whose ordinary query indexes Types,
values, callables, and extended facts. Such a context has no Layout unless it
independently implements Type for a real value-domain reason. Merely being an
Environment or publication boundary never supplies that contract.

TTX treats the small vector and graphics color types as real typed aggregates,
not pack aliases. Active toolchain contexts and explicit package manifests
provide them with fixed field names and `Real_32` component Types.

An active toolchain may supply scalar, vector, and memory forms as top-level
names. Source can use the names installed in that context without a package
qualifier:

```ttx
Vec2D : struct { x : Real_32; y : Real_32; }
Vec3D : struct { x : Real_32; y : Real_32; z : Real_32; }
Vec4D : struct { x : Real_32; y : Real_32; z : Real_32; w : Real_32; }
```

Core TTX exposes `Unsigned`, `Signed`, `Real`, and `Flag` as Terminal contracts.
Width-specific Perimortem Types such as `Unsigned_8`, `Signed_32`, and
`Real_64` return their literal name, rich Documentation, value width in bits,
byte size, and byte alignment directly. TTX does not own a prelude, singleton
catalogue, or supported-width registry. A toolchain constructs and installs the
concrete Types it supports in its own resolution context. If that context does
not install a width, the ordinary Abstract query returns Invalid.

The standard width Types are stateless apart from their C++ virtual identity.
Their fixed queries are:

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

The current Perimortem C++ surface can register `Bool`, `Unsigned_8`,
`Unsigned_16`, `Unsigned_32`, `Unsigned_64`, `Signed_8`, `Signed_16`,
`Signed_32`, `Signed_64`, `Real_32`, `Real_64`, and `Real_128`. `Count` can be an
Alias to the registered 64-bit Unsigned instance. `CppSize` can be an Alias to
the Unsigned instance matching the active C++ interface. `True` and `False` are
Flag values, not additional Types.

Value width describes the terminal domain. Size and alignment describe storage.
None of them dictate register or instruction width. An eight-bit, one-byte
Unsigned Type may correctly use a 32-bit carrier or move when observable stores
and arithmetic preserve its value domain. Aggregates such as `Vec`, `View`, and a language-defined String Type
instead expose their real Addressable structure unless a toolchain deliberately
registers them as another Terminal contract. A byte-array Constant can use an
aggregate Type without creating a native TTX String concept.

`Perimortem.Graphics` is explicit. A package that needs graphics-domain types
receives it through Environment and refers to those types through the bound name:

```ttx
resolve Graphics : Perimortem.Graphics = "2.2";

Graphics::Color : struct {
  r : Real_32;
  g : Real_32;
  b : Real_32;
  a : Real_32;
}
```

Toolchain-provided declarations are semantic objects rather than source copied
into every file. Package declarations come from resolved package manifests. All
later stages use the same objects for member lookup, swizzle checks, pack
fitting, future host-boundary metadata, and shader lowering.

`Vec[T, N]` is different. It is a fixed-size homogeneous aggregate indexed by
position, so indexed packs may initialize sparse entries. `Vec2D`, `Vec3D`,
`Vec4D`, and `Graphics::Color` have named component fields and support named
packs and named swizzles through those fields.

## Definitions

The core definition forms are:

```ttx
[publication] [evaluation] name : dialect;
[publication] [evaluation] name : dialect = value;
[publication] [evaluation] name : dialect { ... }
```

`publication` is one of `public`, `expose`, or `private`. `evaluation` is one of
`state` or `const`. A definition prefix contains at least one of those ordered
slots; when both are present, publication comes first. A local definition may
therefore begin with `state` or `const`, while an owner member may say
`public state`, `expose state`, `private state`, `public const`, or
`private const`.

Evaluation shape: zero or more Documentation comments precede the modifier
prefix. The next token must be `Addressable` or `Type`, then `Define`, then the
name of a continuation Dialect accepted by the parent. The shared Definitions
grammar passes the consumed facts and owning Abstract context directly to that
selected Dialect. After the Dialect name, `Assign` may introduce an initializer.
Otherwise the selected Dialect must end the definition with `EndStatement` or
consume its scoped body.

The parent supplies that grammar as a compile-time set of Definition mappings.
Each mapping carries its Dialect and the ordered modifier prefixes that Dialect
accepts. Each Dialect name appears exactly once; duplicate registrations are an
invalid program rather than a runtime source error.

TTX does not have an inferred declaration operator. Every definition writes its
continuation Dialect at the declaration site so evaluation knows which rule to
run before expression analysis.

Definitions introduce either addressable values or type-like names depending
on the name and selected Dialect:

```ttx
private Header : struct { ... }                     // type definition
private header : Header = (.width = 4, .height = 2); // runtime value
private CountAlias : alias = Count;                 // compile-time Abstract redirect
```

Only builtin definition kinds may open a scope:

```ttx
private Data    : struct  { ... }
private Runtime : object  { ... }
private C       : foreign { ... }
private Stage   : Shader  { ... }
```

Other type-like Dialects define values and must end with `;` or use `=`:

```ttx
private size  : Count = 4;
private octet : Unsigned_8;
```

The parent supplies constexpr Definition mappings to one `Definitions<...>`
grammar. Each mapping carries its Dialect and the modifier token Codes that
parent grammar permits; no evaluator class, runtime dispatch record, or
transient Abstract is constructed. Definitions consumes at most one
publication token followed by at most one evaluation token, rejects malformed
order and names already visible in context, and passes the authored tokens to
the selected definition Dialect. That Dialect validates combination-specific
semantics and constructs the real TTX fact. Definitions then roots that object
and publishes the selected edge when the prefix requests publication. A
“subdialect” is only a Dialect selected by another Dialect, never a separate
contract. The parent Dialect and Type owners check which continuations are legal
along with the compatibility of the definition name, initializer, modifiers,
and attributes.

### Member Order And Late Binding

Library member order is a source invariant:

1. published addressable definitions, with `public` and `expose` retained in
   authored order;
2. public functions;
3. private addressable definitions;
4. private functions.

Type-like definitions are ordered with the corresponding public or private
addressables. Unpublished Library-owned member `state` storage is ordered with
private addressables. `state` describes mutable storage rather than a third
publication block.

The same order applies recursively inside Library-owned Types. The Definitions
evaluator roots each real Addressable, Type, or Callable as it is consumed and
rejects a name that already resolves in the active context. TTX never creates a
forward declaration, incomplete proxy, or second placeholder Abstract.

Function bodies and value initializers are consumed once by the selected
Dialect. A declaration pass may retain a bounded token range temporarily while
the package transaction reserves real owners. Its name queries run against the
completed owning Type or source Abstract. Successful evaluation attaches a
complete identity-free Body to the concrete owner; Cursor state and token
ranges do not remain the executable program. This permits an earlier public
body or initializer to use a later private implementation member while
preserving ordered declaration, collision, and no-shadowing checks. The
declaration header itself is checked when consumed, so a public signature
cannot depend on a private implementation type that has not been declared.

A callable without an authored body is complete only when its active Dialect
supplies its implementation, such as Foreign linkage or a Library `new`
intrinsic. It is not a prototype for a later authored definition.

## Definition Modifiers

Definition modifiers are fixed keyword tokens that give Dialects a shared
publication and evaluation surface without storing a visibility or storage Kind
on the resulting Abstract. The lexer owns spelling and slot classification.
Definitions owns ordered consumption, rooting, and publication. The selected
Dialect receives the authored tokens, validates its combination-specific
legality, and constructs the real definition.

Modifiers occupy two independent ordered slots:

| Publication | Contract                                                                                               |
| ----------- | ------------------------------------------------------------------------------------------------------ |
| `public`    | publish the resulting Abstract with every capability it actually proves                                |
| `expose`    | publish owner-written `state` through an Addressable that does not grant the consumer write capability |
| `private`   | retain the name in its owner context without adding it to the exported definition view                 |
| omitted     | retain the definition only in the current lexical or evaluation context                                |

| Evaluation | Contract                                                                                                                |
| ---------- | ----------------------------------------------------------------------------------------------------------------------- |
| `state`    | create mutable runtime storage owned by the active scope or Type                                                        |
| `const`    | require complete compile-time evaluation and bind the stable materialized result; failure to evaluate is a source error |
| omitted    | use the selected Dialect's ordinary definition semantics                                                                |

`public` does not manufacture write access. It publishes the target unchanged:
a Callable remains invocable, an Addressable remains readable, and a narrower
writable Addressable remains writable. `expose` is deliberately narrower. It is
legal only for owner-written state Addressables. The owner resolves the real
writable storage, while an external consumer resolves a read-only Addressable
projection supplied by `Writable::get_read_only()` with the same name and value
Type. The projection does not prove Writable and does not resolve back to the
writable identity. Repeated reads may observe different values. `expose`
therefore does not mean Constant, immutable data, inspector metadata, or a
general visibility accepted by Callables, Types, aliases, or enum cases.

Write capability on an address and access carried by a loaded value are
different facts. Removing write capability from an Addressable prevents
assignment to that address; it does not rewrite its Type. If exposed state has
Type `Access[T]`, a consumer may still use the loaded access value to mutate
`T`. A definition that intends transitive read-only access must publish a
`View[T]` value instead.

A Type-owned `state` Addressable participates in that Type's Structured storage
Layout regardless of publication. Publication controls name resolution and the
definition view used by external completion and ordinary inspectors; Layout
controls storage, lowering, and explicit raw-layout reflection. Consequently,
private state remains available to its owner and to layout lowering without
bloating the owner's external API. Internal completion may enumerate it while
external completion does not. Tool-specific presentation policy for an
otherwise public definition belongs in an attribute or the tool, not in a core
visibility modifier.

`const` is not write-once runtime storage. Its initializer must evaluate in the
active Dialect's compile-time context to a stable Abstract or evaluation fails.
Ordinary scalar results materialize as Constants, Type expressions retain their
Type identity, and a Dialect may define another stable compile-time domain such
as a type-owned singleton. The binding itself has no writable runtime storage;
a later ABI may materialize an address when its terminal representation needs
one.

Common combinations are:

| Source                                 | Owner view                  | Exported view                    | Storage Layout      |
| -------------------------------------- | --------------------------- | -------------------------------- | ------------------- |
| `public state value : T`               | writable Addressable        | same writable Addressable        | included            |
| `expose state value : T`               | writable Addressable        | read-only Addressable projection | included            |
| `private state value : T`              | writable Addressable        | absent                           | included            |
| `public const value : T = expression`  | stable compile-time binding | same binding                     | not mutable storage |
| `private const value : T = expression` | stable compile-time binding | absent                           | not mutable storage |

Evaluation shape: modifiers are token Codes, not attributes. Definitions does
not parse `public` as `Attribute("public")`, and no semantic visibility enum is
stored on Addressable or Callable. The owner retains internal definitions,
exported definitions, and Layout edges as references to the real Abstracts. An
`expose` definition alone earns a distinct publication edge because that edge
proves fewer capabilities than its writable target; it is not a copied member
record or shadow IR.

This replaces older ideas such as `hidden`, `stack`, `@const`, and `@comptime`.
The language has few spellings, while `Code::Type` still gives evaluators cheap
parse dispatch.

## Attributes And Directives

Attributes attach compiler metadata to members, layout fields, and parameters:

```ttx
@builtin @slot(0) .source : View[Unsigned_8]
@stage(fragment)
@binding @set(0) @slot(1)
```

Parser shape: an attribute starts with `Attribute`, whose token text includes
the leading `@`. It may stand alone as a marker or take one scalar value inside
`PackingStart` and `PackingEnd`. Scalar values may be text, unsigned or signed
integers, real numbers, or booleans. Structured values are not part of the
attribute model. Write another attribute when a declaration needs another
fact.

The scalar carrier is `Core::Static::Union`. `Attribute` owns the authored key,
not a parallel tag or value-storage implementation.

Attributes are not runtime values. They are consumed by the compiler or
forwarded into target metadata.

Some Dialects may consume directive-style attributes as standalone statements, but
package identity is not one of them. Exact package resolutions are envelope
configuration because build systems such as Bazel require output paths to be
declared before source evaluation runs. The Package Dialect body only describes
what the package exports.

Known shader ABI attributes have fixed local targets. The owning Dialect checks
their legality while it evaluates the declaration:

- `@stage(name)` applies to `Shader` definitions in `Shader` packages.
- `@push_constant` applies to exposed `foreign` ABI blocks in `Shader`
  packages.
- `@binding`, `@set(N)`, and `@slot(N)` describe separate resource facts on
  members inside `Shader` scopes.
- `@builtin(name)` and `@builtin @slot(N)` apply to shader builtin input members
  or layout fields.
- `@shader_type(Type)` applies to Library types that deliberately lower as a
  named shader ABI type. Backends must use this metadata or resolved Type
  identity. A matching member layout does not imply shader ABI identity.

Other attributes may remain target-specific metadata until a Dialect defines
their legality rules.

`@if` is an attribute-shaped directive. A host or Dialect may reserve that spelling
for compile-time control flow:

```ttx
@if(enabled) {
  state generated : Count = 1;
}
```

The tokenizer emits `@if` as `Attribute`. The Dialect that supports it decides
whether the following bytecode span is a scope instruction.

## Disabled Members

The disabled marker `/>` wraps the following member as disabled source:

```ttx
/> private experimental : Count = 1;
```

`Disabled` is a lexical marker, not a member, statement, or expression. A
compilation tokenization strips the marker and the source it guards before
semantic evaluation. A source-preserving tokenization emits the marker so tools
can format and highlight the original bytes. Disabled source never produces a
semantic object and is never lowered as a compile-time-false construct.

## Functions

Functions are explicit callable syntax:

```ttx
public func name[params] -> returns {
  ...
}
```

Parser shape: a function member starts with either `Func` or `modifier Func`.
After `Func`, require an `Addressable` function name, parse a parameter layout,
require `CallOp`, parse a return layout, then parse either a block or
`EndStatement`. Whether a declaration may omit its body belongs to its active
evaluator rather than to an `external` marker in the core grammar.

`func` is a keyword because functions are not merely ordinary values with type
`Func`. Evaluation constructs a `Callable : Abstract` carrying the stable
signature used for lowering, entry-point discovery, specialization, foreign
declarations, and shader compilation. Source bodies enrich that same object
through Dialect-defined contracts owned by the selected Dialect.

Both parameters and returns are layouts:

```ttx
private func size[] -> Count;

public func decode[.source : View[Unsigned_8]] -> [
  .ok : Bool,
  .image : Image,
] {
  ...
}
```

Callable has two semantic subtypes:

- `Static` is selected without a runtime receiver.
- `Self` is selected by a runtime value and consumes that receiver.

The subtype is the semantic classification. Consumers do not infer it from a
parameter name, a boolean flag, or the table in which a legacy function record
happened to be stored.

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

The first call resolves `Static/identity` beneath Counter. The second resolves
`Self/identity` beneath Counter and passes `counter` as the declared receiver.
Identical names are legal because the receiver selects Counter's static or self
lookup surface before name lookup. Duplicates within one surface are rejected.

In the Library dialect, bare `self` is shorthand for an ordinary typed member
whose name is `self` and whose type is the owning Type. It causes the Callable
object to implement `Self`. The member is part of the function signature rather
than an implicit compiler local. The parser requires it first.
`Self::get_parameters()` returns the complete effective layout, including this
receiver at position zero. Reflection, generated bindings, pack fitting,
register allocation, and ABI lowering all consume that layout. A Self call
contributes its receiver as the first value, appends the authored argument pack,
and fits the complete sequence. No consumer prepends or slices a second
call-site layout. Root source functions cannot use this shorthand because a
Source is not an addressable runtime value.

A callable placed under a Type is type-owned. That does not make “typed
function” another subtype. `Static` and `Self` describe invocation semantics.
Ownership is carried by the containing Type and the selected lookup surface.

The split is also a tooling contract. Completion after a type token enumerates
Static children. Completion after an expression result enumerates Self children
of the inferred resolved Type. A bare type name selects the static surface,
while semantic typing selects the self surface for values produced by member
access, indexing, or earlier calls.

Callable exposes invocation shape only. A declaration may be interpreted,
compiled locally, restored from a package, or supplied by another language
runtime, but those linkage facts belong to an ABI or execution contract. Core
Callable does not broaden Addressable or acquire a machine-address query.

The return side is a layout, not a separate grammar family.

The return layout is also the return carrier contract. A named return layout
preserves both the field names and their declared order. Callers may bind that
result into another named layout or aggregate if the names and types fit, but
the call boundary itself uses the function's declared return order.

### Foreign Function Declarations

The Foreign Dialect accepts public function declarations without bodies:

```ttx
private C : foreign {
  public func inflate[
    .source : View[Unsigned_8],
    .destination : Access[Unsigned_8],
  ] -> Count;
}
```

This is a Foreign-owned declaration form, not a general `external` keyword or
ordinary forward declaration. In that context a function without a body is an
ABI promise, and the linker or foreign ABI provider must resolve it.

## Members And Scoped Builtins

"Member" in this section names a source position inside a package or scope. It
does not imply a universal `Member` model object. Evaluation constructs the real
semantic object required by the declaration: an Addressable, Type, Alias,
Callable, or a Dialect-specific Abstract. Documentation and attributes remain with
the source owner or the richer object that exposes them. Disabled source is
handled by the lexical boundary and constructs no semantic object.

Parser shape: member parsing proceeds in layers: consume documentation comments,
consume attributes, then choose between function syntax and definition syntax.
If the definition kind is `enum`, parse enum members. If it is a Dialect-owned
scoped word such as `struct`, parse a member scope. Otherwise parse a value
definition.

Scoped builtins own member lists:

```ttx
private Header : struct {
  public width  : Unsigned_32;
  public height : Unsigned_32;
}

private C : foreign {
  public func inflate[
    .source : View[Unsigned_8],
    .destination : Access[Unsigned_8],
  ] -> Count;
}
```

The evaluator identifies scoped builtins by the first resolved type query in the
definition. Dialect and Type owners check that the builtin is legal in the
current scope and Dialect.

## Enums

Enums use a storage-typed brace scope:

```ttx
private Color : enum[Unsigned_8] {
  red = 1;
  green = 2;
  blue = 3;
}
```

Parser shape: after `Define enum`, require exactly one type argument naming the
enum storage type. Then require `ScopeStart` and parse named case assignments or
enum-owned function declarations until `ScopeEnd`. Positional values are
invalid because enum member names are the purpose of the construct.

Semantically, enum members are public compile-time values inside the enum
namespace. Each member value must fit the declared storage type. That makes the
storage width part of the source contract and lets the storage type reject
out-of-range values before lowering. The compiler may store enum metadata in
whatever internal representation is best, but the cases behave like:

```ttx
public const red   : Unsigned_8 = 1;
public const green : Unsigned_8 = 2;
public const blue  : Unsigned_8 = 3;
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
still works. The parser does not need a separate grouping representation.
Semantic pack fitting collapses the one-element pack in value contexts.

Pack modes must not be mixed:

```ttx
(.x = 1, .y = 2) // valid
(1, 2)           // valid
(.10 = 50)       // valid
(1, .y = 2)      // invalid
(.x = 1, .10 = 2) // invalid
```

Packs are source and evaluation shapes, not Abstracts. Positional syntax
produces Fluid Layouts for direct values and Composite Layouts when complete
child Layouts are joined. Nested positional syntax remains one observable
ordered sequence without copying a Ranged or Fluid child into a second list.
Named syntax produces a Named Layout and does not compose with positional mode.
An authored `.field = value` uses Binding when the name is a new edge rather
than a fact already owned by the value. The Layout does not copy or parallel
store names. The evaluator selects one mode before constructing the Layout and
returns Invalid for mixed modes.

Indexed packs are a narrow data-table initialization feature. They are intended
for sparse fixed-size aggregates such as ASCII lookup tables, Base64 decode
tables, opcode tables, and similar cases where most elements use defaults and a
few explicit slots differ.

The shared v1 model has no Indexed Layout subtype. Indexed syntax needs its
receiving Type to validate bounds, duplicates, element fitting, and defaults.
The owning evaluator performs that work and produces complete positional value
flow or Invalid before core Layout fitting.

An indexed designator uses an explicit integer literal after `.`:

```ttx
private decode_table : Vec[Unsigned_8, 256] = (
  .43 = 62,
  .47 = 63,
  .48 = 52,
  .65 = 0,
);
```

The designator position is not an expression context. It does not accept
character literals, names, enum values, arithmetic, or constants:

```ttx
(.'A' = 0)          // invalid
(.Ascii.upper_a = 0) // invalid
(.40 + 3 = 62)     // invalid
```

Decimal and hexadecimal integer literals are both valid, so `.65 = 0` and
`.0x41 = 0` are explicit indexes. Type and layout fitting checks that the target
is index-addressable, that each index is in range, that no index is repeated,
and that every value fits the target element type. Omitted positions use the
target element default.

Grouped value flow has an order. When it is passed, returned, or otherwise
materialized, its Layout order is consumed by the ABI or by the next compiler
stage. Names do not erase that order. They add semantic mapping information on
top of ordered fields only when authored by the source pack syntax or provided
by the receiving Layout.

Structs and other typed aggregates have concrete layouts. Their declaration
order is the semantic carrier order of the aggregate value. A target derives
storage and wire representation from that ordered projection. Type and layout
fitting may map a named fluid pack into a typed aggregate by name, but lowering
must still emit the aggregate in declaration order.

For example, this declaration order is `a, b, c`:

```ttx
private Thing : struct {
  public a : Unsigned_32;
  public b : Unsigned_32;
  public c : Unsigned_32 = 0;
}
```

This initializer is valid because the names identify the target fields:

```ttx
state x : Thing = (
  .b = 1,
  .a = 3,
);
```

The source Named Layout order is `b, a`. The constructed `Thing` stores fields
as `a, b, c`, filling `c` from its default. This is an automatic materialization
shuffle from a Named Layout into a concrete Layout. If a function declares a
named return Layout ordered as `b, a`, the return order is also `b, a`.
Assigning that result to `Thing` requires lowering to shuffle the returned
values into the target aggregate order.

TTX has three explicit ways to produce packs:

1. grouping values with `(...)`
2. swizzling fields with `value.[a, b, c]`
3. slicing a fixed-count range with `value:[start, count]`.

Those forms are the only source-level decomposition operations. A concrete
typed object inside a pack remains one value until source explicitly swizzles or
slices it back into a fluid pack. Grouping, swizzle, and slice produce
positional packs unless a field in the grouping explicitly authors a name.

Nested positional packs expose the same ordered sequence as a flat pack:

```ttx
(1, (2, 3))
```

fits the same positional Layout as:

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

For a concrete structured selection, the Type's Layout supplies the selected
Addressable fact but does not evaluate the access. The active Dialect constructs
a Projection from the receiver and that Addressable. The Projection resolves to
itself, returns the field Type from `Expression::get_type()`, and retains the
receiver path needed for evaluation and lowering.

A uniform positional selection is different. `Ranged` can prove a repeated
element Abstract without allocating one semantic edge per possible position.
The active Dialect owns the indexed Expression that retains the receiver and index
operands and publishes that proven element Type. It must not manufacture a
dynamic Addressable or extend Layout with runtime members. Swizzle and fixed
slice construct positional Layouts over the element Expressions produced by the
appropriate path. Join constructs Composite over already produced Layouts and
never implicitly decomposes a Structured typed value. Swizzle, Slice, and Join
do not need their own semantic Abstract identities. Their results already expose
shared fitting facts through Layout and selected value facts through
Expressions.

### Repacking

Repacking means producing a new pack in the carrier order required by the
receiving context. TTX does not need a separate repack operator because pack
construction, swizzle, slice, and named fields already express the operation
directly.

Names are transient during repacking. Swizzle uses names to select fields, but
the selected result is positional. Slice selects by position and has a
constant-evaluated count, so its result is also a fixed positional Layout.
Grouping positional values keeps positional order.
The only ordinary way to create a named repack result is to author named fields
with `.field = value`, or to fit a produced pack into a declared boundary whose
layout provides names.

Use positional composition when the target wants values in a specific order:

```ttx
state position : Vec3D = (screen_pos.[x, y], z);
```

The swizzle decomposes `screen_pos` into a positional pack, and the outer pack
adds `z`. Composite retains those two child Layouts while exposing the same
three positions to the target.

Use a named pack when names are the contract:

```ttx
state thing : Thing = (
  .b = source_b,
  .a = source_a,
);
```

The named pack states that the values are semantically `b` and `a`, regardless
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

`:[...]` is a fixed-shape decomposition operator. `:[index]` produces one
element Expression. `:[start, count]` requires `count` to evaluate to an
Unsigned Constant before it builds a Fluid Layout with that many element
Expressions. A constant index or start can select from any known Structured or
Ranged Layout. A concrete Addressable selection produces a Projection. A
uniform positional selection uses the indexed Expression owned by the active
Dialect. Its inputs retain the runtime position expression plus the fixed ordinal,
and its `get_type()` returns the element Type proven by the receiver. It does
not copy an Addressable or invent dynamic members in Layout.

A dynamic index or start is legal only when the receiver proves one uniform
element Type. A fixed homogeneous Type can prove that fact through `Ranged`.
A dynamic-extent View requires the receiver Type's narrow, Dialect-owned indexed
access contract. That contract owns bounds behavior; it is not another Layout
and does not make transient indexed positions Abstracts.

The distinction is shape, not whether every operand is known at compile time:

```ttx
color:[1]                 // fixed member selected from a Structured Layout
pixels:[pixel_index]      // one dynamically positioned uniform element
bytes:[offset, 4]         // four uniform elements at a dynamic offset
bytes -> slice(offset, count) // one View-like value with dynamic extent
```

When the count itself is dynamic, ordinary Self dispatch such as
`value -> slice(start, count)` returns one View-like typed value. The receiver
Type's indexed access contract proves the element Type and owns runtime bounds
behavior. C++ `View::Bytes::slice` follows the same dynamic-extent shape.

On the left side of assignment, a fixed slice supplies a fixed positional
target Layout. Concrete structured destinations are Projections whose selected
Addressables prove Writable. A uniform indexed destination instead requires
the receiver's Dialect-owned indexed access contract to prove write capability;
the evaluator does not fabricate one Writable Addressable per runtime position.
A positional source pack fits element by element. A homogeneous fixed-width
source may instead fit the selected homogeneous range as one aggregate write:

```ttx
target:[8, 5] = 0x[08 06 00 00 00];
target:[offset, 4] = pixel.[red, green, blue, alpha];
```

The explicit slice is the decomposition and write boundary. This rule does not
implicitly splat a typed value when it appears in an ordinary pack.

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
[Unsigned_32, Unsigned_32]
[.x : Unsigned_32, .y : Unsigned_32]
[@builtin @slot(0) .source : View[Unsigned_8], .count : Count]
```

Parser shape: a layout either starts with a type reference or with
`LayoutStart`. After `[`, `AddressOp` or `Attribute` means a named layout
field list. `LayoutEnd` means the empty layout. Otherwise parse an unnamed type
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

`for` binds a layout from each value produced by an iterable expression:

```ttx
for [.i : Count] in 0...count {
  ...
}

for [.x : Real_32, .y : Real_32] in points {
  ...
}
```

The iterable owns advancement and the Layout of one produced iteration value.
The binding Layout must fit that value. Its field count never changes the
producer's stride.

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
7. Nested positional packs expose one ordered sequence. Composite preserves
   complete child Layouts while participating in the same positional fitting.
8. Named packs may fit Structured aggregates when every name and resolved
   identity matches exactly once. Core Layout fitting requires the complete
   value set. A Dialect that permits omitted defaults obtains those defaults from
   the real Addressables and completes the Named value flow first.
9. Positional packs may fit Structured aggregates when their resolved identities
   match the target's Addressables in declaration order. Core fitting does not
   manufacture omitted entries.
10. Indexed pack syntax may initialize fixed-size homogeneous aggregates such as
    `Vec[T, N]`. Its owner validates literal indexes and expands them into the
    complete ordered value flow before core Layout fitting.
11. Structured typed values do not flatten into packs implicitly. Source must use
   swizzle or slice syntax to decompose a typed value into a fluid pack.
12. Repack operations produce positional packs unless the syntax explicitly
    authors names or the receiving boundary supplies them.
13. A named pack fitted into a typed aggregate maps by name, then lowers into
    the aggregate's declaration order. A function with a named return layout
    exposes that declared return layout's carrier order at the call boundary.
14. Duplicate or empty names make a Named Layout fail fitting. The owner with
    source or generated-data context retains the information needed for the
    diagnostic.

`Vec[T, N]`, `Vec2D`, `Vec3D`, `Vec4D`, `Color`, user structs, and other
aggregate types are concrete typed values. They may consume compatible packs,
and they may lower to the same storage representation, but they are not aliases
for packs. The fixed vector and color types have builtin struct fields:
`Vec2D.[x, y]`, `Vec3D.[x, y, z]`, `Vec4D.[x, y, z, w]`, and
`Color.[r, g, b, a]`.

## Expressions

Expressions produce values. Assignment is not an expression.

Executable syntax belongs to the Dialect that gives it meaning. The shared TTX
Abstract graph supplies stable declarations, layouts, local identities, and the
narrow Expression queries. Expression identity does not resolve to its result
Type. `get_type()` publishes that fact separately, and `get_inputs()` publishes
the ordered dependency Layout. A Dialect consumes its syntax into the common
identity-free Body tables described above and attaches that Body to its real
Callable, Stage, or lifecycle owner. Body is not a generic statement class
hierarchy and does not preserve the parse tree.

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
the precedence ladder. Primary evaluation can produce literals, addressables,
`self`, discard, type references, packs, and data expressions.
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
Unsigned_32 -> from(value) -> bit_and(Unsigned_32 -> from(0xFF));
```

## Access Chains

Postfix access is the main expression extension point:

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
action -> invoke(args)
```

Parser shape: access-chain parsing loops over postfix suffixes. `AddressOp`
continues only when followed by an addressable or type name. `AddressOp` is
lookup only: named-context lookup, type lookup, enum member lookup, or value
field lookup. `CallOp` is the only call marker. It requires a callable name and
a pack. The call base may be a value, `self`, a Source or Type query, or a
future function-pointer value. `LayoutStart` parses index or fixed-count
index-slice content. The slice count must be constant-evaluated; an index or
slice start may remain a runtime expression for a uniform indexed receiver.
`SwizzleOp` parses swizzle fields or a swizzle slice. A swizzle or swizzle slice
produces a positional pack. `PackingStart` after a type expression is invalid.
Aggregate construction is pack fitting against an expected type, while explicit
conversion is ordinary `-> from(...)` dispatch.

Access forms:

| Syntax            | Meaning                                   |
| ----------------- | ----------------------------------------- |
| `.field`          | field or named-context member access      |
| `:[index]`        | positional index access                   |
| `:[start, count]` | fixed-count positional slice              |
| `.[a, b, c]`      | swizzle that produces a positional pack   |
| `-> name(pack)`   | callable dispatch from the left-side base |

Calls always take a pack. If no arguments are present, the call still formats
as `receiver -> method()`. A Type or Source context resolves a Static
callable. An addressable receiver queries its Type identity, selects
a Self callable, and contributes the declared receiver parameter. Function-pointer
invocation is Self dispatch on the callable value, such as
`callback -> invoke(args)`.

The dispatch receiver must be concrete: a typed value, a Type, or a
Source context. A fluid pack is not a receiver because it has no concrete type or
addressable identity. To call through a value carried inside a pack, source or
the owning query must first select the concrete entry that is the receiver.

Dynamic-extent slicing is callable dispatch, not variable-arity `:[...]`
syntax. A receiver-defined `-> slice(start, count)` consumes runtime values and
returns a single View-like typed value. The receiver's indexed access contract
proves element access and result Type facts. `:[start, count]` instead keeps
`count` constant and produces that fixed number of element Expressions; its
`start` may be dynamic when Ranged or that indexed access contract proves one
uniform element Type.

TTX does not use braced initializers. Braces are scopes and statement blocks.
Aggregate initialization uses packs:

```ttx
private values : Vec[Unsigned_32, 4] = (1, 2, 3, 4);
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
```

Bare `self` is not an assignment target. It is a context root and requires an
access suffix. A package Source is not a lowercase pseudo-root or runtime
value. Package-owned state is reached through an ordinary bound Addressable or
Type context.

Valid assignment targets include:

```ttx
value
value.field
value:[index]
value:[start, count]
value.[x, y]
self.field
```

For `value:[start, count]`, `count` is constant and therefore the assignment
target has a fixed Layout. A dynamic `start` is valid only for a uniform indexed
receiver. A concrete selected Addressable must prove Writable. A dynamic
uniform selection instead requires write capability from the receiver's
Dialect-owned indexed access contract. The source either fits that positional
Layout element by element or supplies a homogeneous fixed-width value that the
writable range accepts as one aggregate copy.

Invalid examples:

```ttx
self = value;
value + other = 1;
receiver -> method() = value;
```

The parser handles this without speculative parsing: it parses the left
expression once, then checks whether the resulting shape is an assignable
address chain before accepting the assignment operator. A concrete final
Addressable must prove Writable. A read-only edge returned by
`Writable::get_read_only()` therefore remains readable but is not a legal
assignment target. A uniform runtime index instead uses the receiver's
Dialect-owned indexed write proof described above. The active Dialect owns
evaluation and lowering after the applicable capability proof.

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
`Terminal` end the current block. A modifier starts a declaration. `Return`
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

`if` and `while` take condition packs. The condition must fit the single-value
`Bool` Layout. Numeric truthiness is not implicit. Use an explicit comparison.

When a Dialect supports `@if`, it can use the same statement shape as `if` while
evaluating the condition as compile-time data.

`match` patterns are expressions or `_`. Each case owns an explicit block.
There is no fallthrough.

`break;` and `continue;` are reserved loop-control statements. The active Dialect
decides where they are legal. The syntax layer only preserves the statement
shape for later lowering.

## Literals

Literal classes:

| Source                 | Meaning                     |
| ---------------------- | --------------------------- |
| `123`                  | decimal numeric literal     |
| `0xFF`                 | hexadecimal numeric literal |
| `0.5`                  | floating literal            |
| `"Raw bytes"`          | quoted byte literal         |
| `0x[AA FF 12 45 ACDE]` | byte data literal           |
| `$[path/to/file]`      | embedded file data          |
| `true`, `false`        | `Bool` literals             |

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
produces `AA FF 12 45 AC DE`. Quoted byte literals decode their escape sequences
and produce the resulting bytes without an implicit null terminator. Both forms
produce byte-array values. TTX assigns no native String semantics to either.

`Bytes` literals and embedded-file literals are tokenized as whole literals, but
their fixed prefixes are currently source text entries on `Code`:

```text
Bytes    -> 0x[
Embedded -> $[
LayoutEnd -> ]
```

The formatter composes those fixed delimiters through `Code` and preserves the
literal payload from the token.

## Documentation

Line comments start with `//`. The tokenizer strips the marker and stores the
comment text as a lexical comment token. Consecutive comment tokens before a
type, member, or function are collected into one `Documentation` object in
source order:

```ttx
// Stored in source order.
// Attached to the following member.
private signature : Vec[Unsigned_8, 8] = 0x[89 50 4E 47];
```

Comments inside statement bodies remain lexical source facts for formatting and
diagnostics. They are not executable statements. Documentation belongs to the
type, member, or function that owns it and is preserved for formatter and LSP
queries. It does not participate in type identity or layout fitting.

Every Abstract exposes a stable Documentation reference. Source objects borrow
arena-owned `Comments`, implementation concepts can expose a generated
`Comment`, and missing prose returns the shared empty Comment. Alias presents
its local lines followed by its target's visible documentation, so Alias chains
accumulate context without a separate tool walk. Constant deliberately returns
the empty Comment because an authored name and its prose belong to the
Addressable that contains the value. Documentation is not a `display_name`
substitute and does not participate in identity, Type equivalence, Layout
equivalence, or Layout fitting.

## Identity Resolution

`Abstract::resolve()` is the total identity operation. Alias follows its target
transitively. Ordinary Abstracts normally return themselves. A consumer resolves
identity before proving a narrower contract such as Type. Type does not grow a
second identity mechanism for aliases.

Name lookup, nested queries, Generic parameterization, and numeric argument
validation remain the responsibility of the Abstract context that receives
them. Identity resolution does not mutate or replace the authored source query.
The formatter and diagnostics use the original source. Compiler analysis uses
the resolved Abstract identity and its proven contracts. Alias cycles are
rejected during graph construction, so resolution of a valid DAG terminates and
never returns a nullable result.

## Diagnostics And Recovery

The parser emits diagnostics where the source shape is wrong and then advances
enough to continue finding later errors. A missing required delimiter or marker
is reported before evaluation advances predictably, which makes recovery
reproducible without making one C++ helper part of the language contract.

Diagnostics use semantic descriptions from `Code::get_semantics()` and fixed
source spellings from `Lexicon::get_spelling()`. This keeps errors aligned with
the lexical contract:

```text
Expected definition `:` but got type
```

Semantic diagnostics prefer ranges that cover the meaningful construct, not
only the token where the compiler first noticed the problem. For example, an
invalid type argument list highlights the argument range when possible.

## Common Semantic Vocabulary

The common semantic vocabulary is intentionally small:

| Source form                        | Runtime model                  | Notes                                                 |
| ---------------------------------- | ------------------------------ | ----------------------------------------------------- |
| primitive numeric types            | value                          | fixed width, no implicit widening                     |
| `Bool`                             | value                          | only `true` and `false` are truthable by default      |
| `Vec[T, N]`                        | typed aggregate value          | homogeneous fixed-size aggregate                      |
| `Vec2D`, `Vec3D`, `Vec4D`, `Color` | typed aggregate value          | named components with stable semantic order           |
| layout                             | structural value stream        | anonymous heterogeneous aggregate                     |
| `struct`                           | nominal value                  | does not flatten implicitly                           |
| `object`                           | nominal managed value          | heap/reference semantics, Dialect-limited             |
| `foreign`                          | external ABI scope             | declarations lower to linked symbols                  |
| `Shader`                           | shader Dialect or scope        | may lower to GPU module plus host glue                |
| `Scene`                            | managed state machine node     | emits typed outcomes, never chooses another Scene     |
| `App`                              | process composition root       | owns Scene transitions and Render-to-Shader policy    |
| `enum`                             | compile-time namespace         | members lower to constants                            |
| `alias`                            | compile-time Abstract redirect | preserves its authored name while redirecting queries |
| `Type`                             | compile-time type value        | stores a resolved Type reference                      |

`Library`, `Package`, `Render`, `Shader`, `Scene`, and `App` are Dialects. They remain
PascalCase Type atoms in source. The host's `Dialects` Abstract context selects
their real model identities instead of the lexer or a registry.

## Lowering Direction

TTX maps naturally to LLVM-like IR concepts:

| TTX                           | Lowering idea                                             |
| ----------------------------- | --------------------------------------------------------- |
| package                       | module or compilation unit                                |
| Environment binding           | module dependency or package alias                        |
| function                      | function definition or Dialect-owned bodyless declaration |
| block                         | structured region that lowers to basic blocks             |
| `if`, `while`, `for`, `match` | branches, loops, phi/select logic, block graphs           |
| `break`, `continue`           | loop exits and loop-iteration control                     |
| field/index access            | address calculation, often `getelementptr`-like           |
| swizzle                       | vector shuffle, aggregate extract, or aggregate insert    |
| pack fit                      | call ABI shaping, return shaping, aggregate construction  |
| `const` values                | constants, metadata, specialization inputs                |
| `foreign` functions           | declarations resolved by ABI/linker                       |
| `Shader` package              | shader artifact plus host-side glue                       |

The source-level constructs are frontend contracts. Many disappear during
lowering, but they remain explicit long enough to produce good diagnostics,
check Dialect rules, and encode target metadata.

Terminal lowering is driven by Type, Layout, and explicit Dialect contracts,
not by source attributes that secretly encode one compiler enum. For every
parameter, result, Body value, field, or stored value, a lowerer first derives
one compilation-local target Representation:

```text
resolved semantic Type
-> prove terminal family, concrete vector/range contract, or explicit
   Dialect-owned representation edge
-> deconstruct semantic Layout through its real Addressables
-> derive target scalar/vector/aggregate, size, alignment, offsets, pointer,
   address-space, storage-class, interface, and ABI records
-> lower common Body operations against those temporary records
-> emit terminal bytes and discard the target records
```

A non-empty aggregate Layout is never collapsed to an invented scalar carrier
merely because a backend recognizes the outer Type name. A byte view, vector,
struct, render contract, and user aggregate are recursively deconstructed
according to their real contracts. Structural coincidence is insufficient: a
four-field color becomes a GPU vector only through an explicit Shader
representation edge or a real Vector proof. The source value remains one
semantic aggregate while each target derives its own physical projection.

An empty Layout alone does not say whether a Type is Terminal or merely an empty
composite. The resolved Type must prove `Terminal` or another explicit
representation contract. A terminal can then be viewed as `Unsigned`,
`Signed`, `Real`, `Flag`, or another toolchain-defined Terminal subtype. The
target, not Layout, chooses physical size, alignment, offsets, pointer width,
register class, storage class, and ABI carrier. A lower compiler does not need
to know whether the authored query reached the Type through an Alias, Generic,
Shader context, or foreign-language object.

Lowering therefore must not depend on an authored `@abi` number, a global
`Abi::Lowering` switch, a C++ type name, or a pointer-keyed side table. Those
forms duplicate semantic facts outside the Abstract graph and become wrong as
soon as a new Dialect or language runtime contributes another terminal contract.
Target Representation is derived data, is not a TTX identity, and is never
serialized as semantic source of truth. An archive stores semantic owners and,
when required, the already selected opaque terminal product.

## Design Invariants

When changing TTX, preserve these invariants:

1. The tokenizer assigns one Code to every emitted source span.
2. Every Code exposes its prescribed grouping through `Code::get_semantics()`.
3. The parser does not backtrack or suppress diagnostics to choose a parse.
4. Assignment remains a statement, not an expression.
5. Parentheses always mean pack.
6. Function parameters, function returns, and `for` bindings all use layouts.
7. Type parameterization proves the Generic contract over resolved arguments
   and returns a concrete compiler-owned Type.
8. Named packs start with `.field`. Named layouts start with `.field` or
   attributes followed by `.field`.
9. Named and positional aggregate fields do not mix.
10. Publication and evaluation intent remain separate ordered modifier token
    classes; neither becomes a semantic Kind stored on the resulting Abstract.
11. Dialect legality belongs to the Dialect or provider that owns the rule, not
    raw parse-shape construction.
12. Formatter output derives from the same token source text as the lexer
    whenever the token has fixed source text.
13. Every queryable semantic identity implements Abstract. Type is a queryable
    subtype, not the universal base. Reflection and schema identities are
    Abstracts too. Layout and metadata are facts, not competing identities.
14. Failed semantic resolution returns Invalid, never a null pseudo-Abstract.
15. Static and Self are Callable subtypes, and Self's complete parameter Layout
    contains the receiver at position zero.
16. Resolution passes borrowed `View::Bytes` without prescribing partitioning.
    The same ordered chain is deterministic while the DAG is unchanged. Alias
    may redirect an unchanged route. Optional export naming never substitutes a
    signature hash for the selected named ownership chain.
17. Lowering proves Terminal before applying aggregate rules to a Type's Layout.
    Non-Terminal Types recursively deconstruct every Addressable in that Layout,
    including the valid zero-entry aggregate case.
18. Layout is an ordered fitting contract over Abstracts. Fluid, Named,
    Structured, Ranged, and Composite express distinct semantics through
    inheritance. There is no Member record, kind enum, or Incomplete Layout.
19. Expression identity remains distinct from its result Type. Constant is an
    immutable zero-input Expression, and its equality includes domain, resolved
    Type, and payload.
20. `:[index]` produces one element Expression. `:[start, count]` requires a
    constant-evaluated Unsigned `count` and produces a fixed positional pack. A
    concrete Addressable selection uses Projection. A dynamic index or start
    requires a uniform element Type proven by Ranged or the receiver's
    Dialect-owned indexed access contract. A dynamic count is Callable dispatch
    that returns one View-like typed value.
21. TTX does not shadow names. A parameter, loop binding, or local definition is
    rejected when its name already resolves in the complete active context.
    Package and owner state require an explicit addressing root.
22. Writable is the only core write-capability proof for a durable Addressable.
    Its read-only projection is stable, preserves name and resolved Type, does
    not prove Writable, and does not resolve back to the writable identity.
    Runtime indexed mutation remains owned by the active Dialect and receiver Type.
23. A Scene owns state, lifecycle edges, render roots, and typed signals. It
    never owns a transition to another Scene or process-exit policy.
24. App owns the initial Scene and the complete transition mapping. A cyclic
    transition graph does not imply cyclic Source resolution because Scene
    definitions do not reference their transition targets.
23. Managed is the only common proof that a Type is a runtime-managed object
    reference. Allocation, tracing, object headers, movement, address spaces,
    and pointer representation remain runtime or target facts.
24. Body is one identity-free executable value retained by its real Callable,
    Stage, or lifecycle owner. It contains compact local IDs and non-null real
    graph edges; it is never an AST, Abstract, replayable Cursor, or second Type
    graph.
25. A Dialect controls presentation, accepted builtins, evaluation, legality,
    and additional versioned facts. It may restrict a common contract but may
    not reinterpret it, and Environment binding never imports Dialect rules.
26. Layout owns semantic order and directional fitting only. Physical size,
    offsets, target alignment, pointers, storage classes, register classes,
    ABI carriers, and collector policy belong to derived target/runtime owners.

These rules are what keep TTX readable while still letting it behave like a
compiler IR.
