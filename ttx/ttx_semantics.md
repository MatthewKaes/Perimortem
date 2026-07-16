# TTX Language Semantics

TTX is a frontend source IR language. It is authored by humans, but it stays
close to the compiler's semantic model. A TTX file makes package boundaries,
storage shape, addressability, layout, and lowering intent visible in the
source. Tetrodotoxin uses TTX as its default frontend, but the TTX source IR and
token bytecode can also serve as an interchange boundary for another host that
agrees to the token and data model.

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
builds a module graph, and code generation can continue through type, layout,
ISA, ABI, provider, and compilation queries. Fast compilation is part of that
goal, not an afterthought. Every syntax feature must justify its parse cost,
recovery cost, and downstream context cost.

To make that work, the syntax carries a large amount of semantic information
directly. Casing separates addressable names from type names. Sigils encode
visibility and compile-time addressability. The fixed `alias` keyword has its
own lexical class. ISA-owned definition words such as `struct`, `object`,
`enum`, and `foreign` are lowercase addressable spellings whose meaning belongs
to the active evaluator. They are not PascalCase type references. Packs,
layouts, access chains, and assignment statements all have distinct local
shapes.

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
  import binding, Abstract identity resolution, Type proofs, ISA rules, pack
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

## Token Bytecode And VM Execution

TTX source text is the human-authored source IR. Lexical analysis lowers that
source IR into TTX token bytecode: a compact instruction stream whose token
classes already carry stable semantic categories such as `Type`, `Addressable`,
`Import`, `Assign`, `TypeAccessOp`, `AddressOp`, `CallOp`, modifiers, attributes,
and fixed operators.

The class value is stored in 8 bits, and payload-bearing tokens keep source text
views beside that class. The exact vocabulary is defined by `Lexical::Class`,
not frozen as a prose count. Those values are arbitrary as byte values, but not
arbitrary as language facts: each token class exists because it gives an
envelope evaluator, body evaluator, or other host-owned decoder a useful
intended category before any larger instruction is decoded.

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
authoring domain, and makes TTX facts queryable: types, layouts, package
exports, callable facts, ABI facts, shader facts, or other context.

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

This model is close to attribute-grammar and query-based incremental compiler
systems. The important distinction is ownership:

- TTX source owns authored bytes and source spans.
- Lexical owns token classification and token payload views.
- Abstract objects and their virtual contracts form the shared TTX semantic
  graph. Type, Alias, Generic, Expression, Constant, Callable, Static, Self,
  Addressable, and Invalid are contracts in that graph. Layout is the fitting
  contract over an ordered group of those real objects.
- Host envelope evaluators own whatever source preamble they choose to execute.
- Import, module, package, cache, and invalidation layers belong to the host
  that needs them.
- ISAs or evaluators own the bytecode spans they understand and the facts they
  make queryable from those spans.
- ABI, provider, and backend queries expose derived answers without cloning the
  program into a second semantic tree.
- Terminal emitters own output-specific representations such as SPIR-V words,
  LLVM IR, object files, archives, generated headers, editor JSON, or formatted
  source text.

## Semantic Object Model

The evaluated semantic model is a directed graph of `Abstract` objects. An
object may be reached through more than one import or alias edge, so its
ownership graph is not forced into a tree and the object does not store one
authoritative parent path. Resolution passes the remaining borrowed
`View::Bytes` directly through the objects it reaches. It does not allocate or
persist a parallel path graph.

The base hierarchy is:

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
│   │   ├── Ttx::Model::Types::Unsigned
│   │   ├── Ttx::Model::Types::Signed
│   │   ├── Ttx::Model::Types::Real
│   │   └── Ttx::Model::Types::Flag
│   └── ISA-defined model types
├── Ttx::Model::Callable
│   ├── Ttx::Model::Static
│   └── Ttx::Model::Self
└── Ttx::Model::Addressable
```

`Ttx::Abstraction` owns the restricted identity and resolution substrate.
`Ttx::Model` owns the shared semantic vocabulary built on that substrate.
Model is a namespace and source-IR ownership boundary, not a central registry
or a second graph.

These names describe up-castable semantic contracts:

| Contract      | Required meaning                                                                    |
| ------------- | ----------------------------------------------------------------------------------- |
| `Abstract`    | object name, identity redirection, and progressive context resolution               |
| `Alias`       | closed named redirection of identity and context queries to another Abstract        |
| `Type`        | resolved semantic identity with a total Structured Layout query                     |
| `Generic`     | instruction contract that resolves arguments to a concrete Type                    |
| `Pack`        | grouped value flow with a fitting Layout and no implied Type                        |
| `Expression`  | one evaluatable value with a result Type query and ordered input Layout             |
| `Constant`    | immutable Expression already in normal form with value equality                    |
| `Projection`  | Expression selecting one Addressable from one receiver Expression                  |
| `Binding`     | Expression giving one underlying Expression an authored flow name                  |
| `Callable`    | complete parameter and result Layouts plus an address/linkage query                 |
| `Static`      | invocation selected through a Type or package without a receiver                    |
| `Self`        | invocation whose addressable receiver is parameter zero                             |
| `Addressable` | named semantic edge whose resolution supplies the addressed Abstract                |
| `Invalid`     | absorbing failed resolution                                                         |

Terminal Types add narrow storage and domain contracts:

| Contract   | Required meaning                                           |
| ---------- | ---------------------------------------------------------- |
| `Terminal` | Layout leaf with direct byte size and alignment             |
| `Unsigned` | non-negative integer domain                                |
| `Signed`   | signed integer domain                                      |
| `Real`     | floating-point domain                                      |
| `Flag`     | two-value logical domain                                   |

The first narrow native contracts are deliberately reference based:

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

The final query may return Addressable or Invalid. Layout itself stores no
member record. It exposes ordered Abstract references. Structured narrows those
entries to the actual Addressable objects in a Type. Their names, resolved child
Types, documentation, attributes, defaults, and ISA facts remain on those real
objects or on richer contracts they implement. Consumers resolve identity
before using a narrow contract. No nullable reference is part of the Layout
interface.

Contiguous semantic collections store `Abstraction::Reference<Contract>`, a
non-null borrowed reference value. It preserves the object it receives.
Consumers call `resolve()` explicitly when they need represented identity, so a
Structured Layout retains its real Addressables while a Generic Argument can
retain the resolved object chosen at its own query boundary.

### Contract Proof And Upcasting

TTX contract inheritance is queryable without C++ RTTI, a ClassDB, a global
type-number allocator, or a hash. Every declared semantic contract owns a
stable 128-bit `Perimortem::System::Uuid`. Its implementation recognizes that
identifier and delegates unrecognized identifiers to its direct base contract.
The work is therefore two word comparisons per shallow inheritance level and
requires no allocation or dynamic registry lookup.

Contract identifiers identify interfaces only. They never identify an Abstract
object, replace its name, form a resolution route, select an export symbol,
version a package, or become a serialized object handle. Object identity inside
one stable DAG remains its address. Durable identity remains a reversible chain
of names. An incompatible change to a contract's required operations receives a
new contract identifier rather than silently changing the old meaning.

Native consumers use the total query pair:

```text
abstract.is<Type>()  -> Bool
abstract.as<Type>()  -> const Type&
```

`is<Contract>()` asks the object to prove the declared contract and its base
chain. `as<Contract>()` checks that proof and returns a reference. Asking it to
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

`Type` is not the root of this model. Callable, diagnostic, ISA, and
future runtime objects do not inherit Type merely to become queryable. There is
also no `Typed` marker. A package is a top-level Type whose own context resolves
its imported and declared children. The only common resolution operations are:

```text
abstract.resolve()             resolve represented identity
abstract.resolve_context(route) resolve borrowed bytes in this context
```

A Type may expose independent static and self lookup surfaces as its derived
contract develops, and may back each surface with its own map. The receiver
semantics select the surface before lookup. The Abstract base does not prescribe
those methods, containers, or route grammar.

An Alias is resolved before a consumer proves the resulting contract. Lower
layers can therefore operate on `Type` without knowing whether the authored
query passed through an Alias, Generic construction, or an ISA-specific
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
lookup and slicing policy. A small context may compare names directly. A
package Type may use a table for its children. A Type may keep independent
static and self maps. These are object-owned lookup policies rather than modes
hard-coded into a central resolver.

The following rules are the ground truth for this contract:

| Query rule          | Required behavior                                                                                           |
| ------------------- | ----------------------------------------------------------------------------------------------------------- |
| local name          | `get_name()` names the current Abstract, including an authored Alias                                        |
| canonical name      | call `resolve().get_name()` when the represented identity's name is required                                |
| identity            | `resolve()` returns a real Abstract reference and is idempotent for an unchanged valid DAG                  |
| route ownership     | `resolve_context(route)` receives the entire borrowed view and the current Abstract owns its interpretation |
| route partitioning  | chained queries and one combined route are not required to be equivalent                                    |
| context redirection | a resolver may forward an unchanged route while selecting a different context                               |
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
`Palette` in the package Type and `Color` in the aliased Type without
manufacturing or rewriting an intermediate path object.

Alias is closed around that responsibility. It owns only its local name and a
borrowed target reference. It does not own Type behavior, layout,
documentation, attributes, diagnostics, package membership, or route history.
The graph owner guarantees target lifetime and rejects alias cycles before an
Alias becomes queryable.

Names remain unique inside each lookup surface. Static and self callables may
share a name because the receiver chooses the Type-owned surface before lookup.
If a host exports a symbol, that optional export owner walks its selected public
ownership chain and writes each name reversibly. Resolution itself does not
store an allocated path, hash a signature, or choose a lexicographically
preferred alias.

### Invalid And Total References

`Invalid : Abstract` is the semantic failure object. It represents missing
names, a wrong requested contract, ambiguous resolution, rejected alias-cycle
construction, invalid archives, unsupported ISA construction, and unresolved
linkage when a semantic object is required. The source-owning caller retains the
authored route, source range, and presentation context. Invalid does not grow a
second diagnostic state model. A successfully constructed Alias is therefore
always acyclic.

Queries through Invalid are absorbing and always return the same Invalid. This
prevents one bad name from producing a cascade of unrelated failures. Invalid
is closed and stateless. It never owns the failed route, source range, message,
or a specialized failure subtype. The source-owning query retains those facts.

The semantic interface follows these rules:

| Situation                       | Representation                                          |
| ------------------------------- | ------------------------------------------------------- |
| failed name or contract query   | `Invalid&`                                              |
| no children                     | empty child view                                        |
| unresolved callable address     | explicit unresolved Addressable or `Invalid&`           |
| corrupt restored package        | Invalid Abstract in place of the top-level Type         |
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
evaluate it. The active ISA owns operator legality, executable bodies, parsing,
evaluation, and diagnostics.

Pack and Layout are related but not interchangeable. Pack is a queryable
Abstract that carries grouped value flow through the semantic DAG. Layout is
the identity-free fitting view exposed by that Pack. This separation lets an
evaluator return either one Expression, one Pack, or Invalid without turning
every Type Layout into a value object. Concrete positional and named Packs own
their fitting views by composition and do not inherit Layout as another public
contract. Expression dependencies remain an identity-free Layout because an
operand list is not itself an authored multi-value result.

Projection and Binding are concrete Expression facts. Projection retains one
receiver Expression and the selected Addressable, then publishes the
Addressable's resolved Type through `get_type()`. Binding retains one authored
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

| Contract                | Payload       | Contextual fitting rule                              |
| ----------------------- | ------------- | ---------------------------------------------------- |
| `Constants::Unsigned`   | `Bits_64`     | any Unsigned Type that can represent the value       |
| `Constants::Signed`     | `Signed_64`   | any Signed Type that can represent the value         |
| `Constants::Real`       | `Real_128`    | exact resolved Type in the first slice               |
| `Constants::Flag`       | `Bool`        | any Flag Type                                        |
| `Constants::Bytes`      | `View::Bytes` | exact resolved Type in the first slice               |

Constant equality requires the same domain contract, the same resolved Type,
and the same payload. Two independently allocated constants with those facts
are equal. Real NaNs compare as one semantic value so equality remains a valid
cache equivalence relation. Constant identity is never collapsed to Type
identity. TTX has no native String constant. Quoted source text is decoded to
bytes, and a language that wants String semantics builds its own Type and
operations from that data.

Exact decimal source text may remain an ISA-owned literal Expression until an
expected Type chooses a floating format. `Constants::Real` represents a value
that has already been evaluated into `Real_128`. A future folding facility can
be another narrow contract over expressions that can prove a Constant. It does
not require an evaluation method on every Expression.

## Layout Facts

Layout is the shared fitting contract over an ordered group of real Abstracts.
It answers how many objects are present, which Abstract is at an index, whether
this source shape fits a target shape, and which source Abstract supplies each
target slot. An invalid index or fit returns Invalid. Layout is not an Abstract,
a member container, a Type registry, a storage record, or a lifecycle state.

The three v1 contracts are deliberately separate classes:

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

Fitting is directional: `source.fits(target)`. Fluid and Named values may fit a
Structured target without acquiring Type identity themselves. A Structured
value does not silently decompose into value flow. Source uses swizzle or slice
syntax to make that transition explicit.

`source.get_fitted(target, target_index)` returns the original source Abstract
that supplies that target slot. A failed fit, invalid index, or missing mapping
returns Invalid so the source owner can retain the diagnostic. Fluid and
Structured preserve positional order. Named exposes the permutation it proved
by name and resolved identity. This is fitting evidence, not a copied member or
an allocated mapping. A caller can construct target-ordered value flow without
repeating Named matching.

Layout does not copy a field's name, Type, documentation, attributes, default,
storage, or ISA metadata into a generic entry. The Abstract supplies its own
name and represented identity. Structured supplies the stronger guarantee that
the object is the real Addressable edge owned by the Type. Additional facts stay
on that object, its source owner, or a narrower derived contract. There is no
`Member` model object and no optional-name/default bit embedded in Layout.

`Type::get_layout()` returns `const Layouts::Structured&`. A Terminal has an
empty Structured Layout and directly publishes its target byte size and
alignment. For a composite, lowering walks the Addressables in order, resolves
each one, proves the resulting Type contract, and recurses into that Type's
Structured Layout. Composite offsets and aggregate size/alignment are derived
from those Terminal facts. Carrier, register class, calling convention, and
wire policy remain later compiler queries. None of these are Layout fields.

A Type-targeting Alias is resolved before the Type contract is proved. A
Generic is a compile-time instruction rather than a Type or value.
Parameterization must produce a resolved Type whose
Structured Layout can be queried. Layout identity alone never substitutes for
Type identity.

Callable parameters and results use the base Layout contract because signatures
may be Fluid, Named, or Structured. Static and Self are Callable objects found
through their owning contexts, not Layout entries. Self includes its receiver at
parameter zero, so fitting, reflection, invocation, and lowering all consume the
same complete signature.

Core fitting requires the objects presented to it. A source language or ISA
that supports omitted defaults resolves those defaults on the real Addressable
objects and constructs the complete value flow before asking Layout to fit.
This keeps default policy out of every layout consumer.

### Resolution, Completion, And Invalidation

Source loading may require preregistration for recursive Types, aliases,
mutually visible Callables, imports, or ISA-specific declarations. That is a
host construction technique, not a universal publication lifecycle.

A host may reserve a stable, nonmoving Type or containing resolver before all
facts are known. While the requested fact is incomplete, the object or system
resolves that query to Invalid. Once the owner has enough information,
resolution reaches the real Type and `get_layout()` is a total Structured
reference. Layout has no `Incomplete` kind, and an empty Structured Layout is
never overloaded to mean "not ready."

Different hosts may complete the graph differently. One may enrich a reserved
object after a declaration pass. Another may replace an enclosing resolver or
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
   instruction set and expose TTX facts from that execution.
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
another language runtime as long as the boundary makes the TTX facts its
queries need available through resolution.

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

| Source shape                | Token class            | Semantic meaning                                |
| --------------------------- | ---------------------- | ----------------------------------------------- |
| `snake_case`                | `Addressable`          | runtime names, fields, functions, local values  |
| `PascalCase`                | `Type`                 | type names, aliases, ISA names, package names   |
| `enum`, `struct`, `foreign` | `Addressable`          | ISA-owned definition forms, not type references |
| `alias`                     | `Alias`                | fixed Abstract-redirection definition           |
| `public`, `private`, etc.   | modifier tokens        | ISA-owned visibility, ownership, or storage     |
| `@name`                     | `Attribute`            | metadata attached to members, params, fields    |
| `@if`                       | `Attribute`            | directive owned by an ISA or host               |
| `break`, `continue`         | control keyword tokens | loop-control statements                         |
| `0x[...]`                   | `Bytes`                | byte data literal                               |
| `$[...]`                    | `Embedded`             | embedded file literal                           |
| `_`                         | `Discard`              | wildcard or ignored value                       |

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

The source spelling is `dialect`, but semantically it names an evaluator
installed in the active host. A host with different installed ISAs may reject a
source another host accepts.

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

`alias` is the fixed definition keyword. ISA-owned lowercase spellings such as
`object`, `struct`, `enum`, and `foreign` are definition forms only when the
active evaluator assigns them that meaning. None of them are ISA names. For
example:

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
Imported definitions must still be valid in the importing ISA after identity
resolution.

There is no `using` or wildcard import syntax. Imports are named aliases so
source reviews and diagnostics can see package boundaries.

## Names And Type Queries

TTX uses casing as a semantic boundary:

- `snake_case` names are addressable runtime values or fields.
- `PascalCase` names are types, aliases, package names, or ISA names.

Type references are progressive Abstract queries whose final result must prove
the Type contract. The object does not own one string path. Each resolver
receives borrowed `View::Bytes` and may interpret them atomically, pass a sliced
suffix, or redirect them unchanged without allocating a second representation:

```ttx
Bits_32
Vec[Bits_8, 4]
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
- `:[...]` dispatches to constant-evaluated index or slice semantics.

`::` is therefore not string concatenation. It establishes a nested context
query whose receiving Abstract owns the lookup grammar. An evaluator may offer
the remaining `Graphics::Color` bytes as one view or issue ordered queries at
the source operators. Those forms are not required to be equivalent. The final
result must prove Type. The source owner retains the authored input and the
operation being resolved so it can report the exact failed boundary.

Type arguments use `[]`, not `<>`, because `<` and `>` are comparison
operators. Numeric type arguments, such as the `4` in `Vec[Bits_8, 4]`, are
part of the type query and are checked while proving that query.

Parameterization is Generic dispatch over resolved arguments. A type reference
such as `View[Bits_8]` resolves `View`, proves that it implements `Generic`,
resolves `[Bits_8]`, and asks the Generic to create or find the concrete Type.
An Abstract that is not Generic produces Invalid at that exact step. The
Generic validates its accepted argument form and returns a real Abstract. The
caller then resolves that result and proves Type. Generic is not a placeholder
Type and does not have a Layout of its own.

Each named Generic is a registered formula such as `Vec`, `View`, or `Dict`.
The current source context owns the lookup surface and may index Generic names
separately from concrete Type names. The Generic object owns its accepted
argument schema, materialization rule, and concrete-Type cache. TTX does not
define a global Generic registry or switch on formula names.

The evaluator constructs each `Argument` before calling the formula. An
Abstract argument stores the result of `resolve()`. Bool and unsigned arguments
store their tagged scalar values. Constants remain Abstract arguments because
their resolved identity is the value, not its Type. Two constants compare as
the same Argument when their domain, resolved Type, and payload are equal.
Argument order is preserved. The ordered Argument sequence is therefore the
complete cache key inside one formula. Aliases and type-producing expressions
that resolve to the same final Abstract share a cache entry. Unrelated
Abstracts with the same local name remain different entries. The key never
includes a parent pointer, authored route, formatted Type name, or hash.

Missing formula lookup returns Invalid at the owning context. A resolved object
that does not implement Generic fails the contract proof. A Generic whose
argument schema does not accept the supplied values returns Invalid from
materialization. These are separate source errors even though they share the
same semantic failure object.

The concrete Type returned by parameterization is compiler-owned. Its stable
handle is used for local equivalence, member lookup, nested Type lookup,
Callable lookup, and Layout queries. A host that exports it derives a durable
name by walking a selected named ownership chain. The normalized argument
sequence is an input to construction, not a second semantic model. The source
owner retains the authored route for diagnostics. A publication owner selects
and renders a public ownership chain independently of local cache identity.

`alias` creates an Alias Abstract that preserves the authored local name and
redirects to its resolved target. When that target is a Type, a Type consumer
calls `resolve()`, proves the Type contract, and then forgets the Alias.
Documentation belongs to the source declaration or owning context beside the
Alias, not to the closed Alias object itself.

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

Decode shape: a type reference starts with `Type`, or a numeric token when
decoding a numeric type argument. `TypeAccessOp` continues progressive context
resolution. The receiving Abstract owns route interpretation, and the completed
query proves the resolved object's Type contract. Type arguments start with
`IndexStart`, contain type references separated by `PackingOp`, and end with
`IndexEnd`.

## Terminal Registration And Package Types

A package is a top-level Type. Its ordinary context indexes imported and
declared Types, values, and callables using the lookup structures appropriate
to that package. It is not a parallel package-shaped semantic object or a
special branch in the resolver.

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
It does not own a prelude, singleton catalogue, supported-width list, or one C++
class per spelling. A toolchain constructs stable instances for the formats it
supports, gives them names in its own resolution context, and supplies each
instance's byte size and alignment. If that context does not install a width,
the ordinary Abstract query returns Invalid.

The current Perimortem C++ surface can register `Bool`, `Bits_8`, `Bits_16`,
`Bits_32`, `Bits_64`, `Signed_8`, `Signed_16`, `Signed_32`, `Signed_64`,
`Real_32`, `Real_64`, and `Real_128`. The `Bits_*` spellings implement the
`Unsigned` contract. Their current names do not create a separate Bits concept.
`Count` can be an Alias to the registered 64-bit Unsigned instance. `CppSize`
can be an Alias to the Unsigned instance matching the active C++ interface.
`True` and `False` are Flag values, not additional Types.

Size and alignment are real Terminal facts, but they still do not dictate
register or instruction width. A one-byte Unsigned Type may correctly use a
32-bit carrier or move when observable stores and arithmetic preserve its value
domain. Aggregates such as `Vec`, `View`, and a language-defined String Type
instead expose their real Addressable structure unless a toolchain deliberately
registers them as another Terminal contract. A byte-array Constant can use an
aggregate Type without creating a native TTX String concept.

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
private CountAlias : alias = Count;                 // compile-time Abstract redirect
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

| Modifier  | Intended contract                                    |
| --------- | ---------------------------------------------------- |
| `public`  | visible API that other sources may read or call      |
| `private` | implementation detail owned by the active ISA        |
| `expose`  | externally readable data, written by its owner       |
| `state`   | stateful storage that is not part of the value shape |
| `const`   | write-once or compile-time data                      |

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

The scalar carrier is `Core::Static::Union`. `Attribute` owns the authored key,
not a parallel tag or value-storage implementation.

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
  named shader ABI type. Backends must use this metadata or resolved Type
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

Callable has two semantic subtypes:

- `Static` is selected by a type or package receiver.
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
call-site layout. Root package functions cannot use this shorthand because a
package is not an addressable runtime value.

A callable placed under a Type is type-owned. That does not make “typed
function” another subtype. `Static` and `Self` describe invocation semantics.
Ownership is carried by the containing Type and the selected lookup surface.

The split is also a tooling contract. Completion after a type token enumerates
Static children. Completion after an expression result enumerates Self children
of the inferred resolved Type. A bare type name selects the static surface,
while semantic typing selects the self surface for values produced by member
access, indexing, or earlier calls.

Callable exposes an address query because a declaration may be unresolved,
interpreted, compiled locally, restored from a package, or supplied by another
language runtime. A resolved call receives an Addressable Abstract with the
appropriate linkage contract. An unresolved external receives an explicit
unresolved Addressable or Invalid. It never receives `nullptr` and never derives a
linker name by hashing its signature.

The return side is a layout, not a separate grammar family.

The return layout is also the return carrier contract. A named return layout
preserves both the field names and their declared order. Callers may bind that
result into another named layout or aggregate if the names and types fit, but
the call boundary itself uses the function's declared return order.

### Foreign Function Declarations

The Foreign ISA accepts exposed function declarations without bodies:

```ttx
private C : foreign {
  expose func inflate[.source : View[Bytes]] -> Bytes;
}
```

This is a Foreign-owned declaration form, not a general `external` keyword or
ordinary forward declaration. In that context a function without a body is an
ABI promise, and the linker or foreign ABI provider must resolve it.

## Members And Scoped Builtins

"Member" in this section names a source position inside a package or scope. It
does not imply a universal `Member` model object. Evaluation constructs the real
semantic object required by the declaration: an Addressable, Type, Alias,
Callable, or an ISA-specific Abstract. Documentation and attributes remain with
the source owner or the richer object that exposes them. Disabled source is
handled by the lexical boundary and constructs no semantic object.

Parser shape: member parsing proceeds in layers: consume documentation comments,
consume attributes, then choose between function syntax and definition syntax.
If the definition kind is `enum`, parse enum members. If it is an ISA-owned
scoped word such as `struct`, parse a member scope. Otherwise parse a value
definition.

Scoped builtins own member lists:

```ttx
private Header : struct {
  public width  : Bits_32;
  public height : Bits_32;
}

private C : foreign {
  expose func inflate[.source : View[Bytes]] -> Bytes;
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
whatever internal representation is best, but the cases behave like:

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

Packs are Abstracts for in-flight value groups. A Pack has semantic object
identity so evaluators, tools, and lowering can return and query it, but it is
not one Expression, a Type, runtime storage, or an addressable object. It
exposes an identity-free Layout for fitting. `Packs::Positional` exposes Fluid
and `Packs::Named` exposes Named.

A positional Pack borrows an already normalized view of real Abstract entries.
The evaluator flattens nested positional Packs while constructing that view, so
the Pack owns only one Fluid fitting object and allocates no second list. A
named Pack owns one Named fitting object over actual named Abstracts and never
flattens. An authored `.field = value` uses Binding when the name is a new edge
rather than a fact already owned by the value. The Layout does not copy or
parallel-store names. The evaluator selects one mode before constructing a
Pack and returns Invalid for mixed modes.

Indexed packs are a narrow data-table initialization feature. They are intended
for sparse fixed-size aggregates such as ASCII lookup tables, Base64 decode
tables, opcode tables, and similar cases where most elements use defaults and a
few explicit slots differ.

The shared v1 model has no Indexed Pack subtype. Indexed syntax needs its
receiving Type to validate bounds, duplicates, element fitting, and defaults.
The owning evaluator performs that work and produces complete positional value
flow or Invalid before core Layout fitting.

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

Decimal and hexadecimal integer literals are both valid, so `.65 = 0` and
`.0x41 = 0` are explicit indexes. Type and layout fitting checks that the target
is index-addressable, that each index is in range, that no index is repeated,
and that every value fits the target element type. Omitted positions use the
target element default.

Packs have a carrier order. When a pack is passed, returned, or otherwise
materialized, the pack's field order is the order consumed by the ABI or by the
next compiler stage. Names do not erase that order. Names add semantic mapping
information on top of ordered fields only when they are authored by the pack or
provided by the receiving layout.

Structs and other typed aggregates have concrete layouts. Their declaration
order is the semantic carrier order of the aggregate value. A target derives
storage and wire representation from that ordered projection. Type and layout
fitting may map a named fluid pack into a typed aggregate by name, but lowering
must still emit the aggregate in declaration order.

For example, this declaration order is `a, b, c`:

```ttx
private Thing : struct {
  expose a : Bits_32;
  expose b : Bits_32;
  expose c : Bits_32 = 0;
}
```

This initializer is valid because the names identify the target fields:

```ttx
state x : Thing = (
  .b = 1,
  .a = 3,
);
```

The source pack carrier order is `b, a`. The constructed `Thing` stores fields
as `a, b, c`, filling `c` from its default. This is an automatic materialization
shuffle from a named fluid layout into a concrete layout. If a function declares
a named return layout ordered as `b, a`, the return carrier order is also
`b, a`. Assigning that result to `Thing` requires lowering to shuffle the
returned values into the target aggregate order.

TTX has three explicit ways to produce packs:

1. grouping values with `(...)`
2. swizzling fields with `value.[a, b, c]`
3. slicing a constant-evaluated range with `value:[start, count]`.

Those forms are the only source-level decomposition operations. A concrete
typed object inside a pack remains one value until source explicitly swizzles or
slices it back into a fluid pack. Grouping, swizzle, and slice produce
positional packs unless a field in the grouping explicitly authors a name.

The evaluator flattens nested positional packs while constructing the outer
Pack:

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

The Type's Structured Layout supplies the selected Addressable facts, but it
does not evaluate an access. An expression ISA constructs a Projection from
the receiver and selected Addressable. That Projection resolves to itself,
returns the field Type from `Expression::get_type()`, and retains the receiver
path needed for evaluation and lowering. Swizzle and fixed slice construct a
Positional Pack over those Projection Expressions. Join constructs another
Positional Pack over already produced values and never implicitly decomposes a
Structured typed value. Swizzle, Slice, and Join do not need shared subclasses
unless a future consumer needs a query that Pack and Projection cannot answer.

### Repacking

Repacking means producing a new pack in the carrier order required by the
receiving context. TTX does not need a separate repack operator because pack
construction, swizzle, slice, and named fields already express the operation
directly.

Names are transient during repacking. Swizzle uses names to select fields, but
the selected result is positional. Slice selects by constant-evaluated
positions, so its result is also positional. Grouping positional values keeps
positional order.
The only ordinary way to create a named repack result is to author named fields
with `.field = value`, or to fit a produced pack into a declared boundary whose
layout provides names.

Use positional composition when the target wants values in a specific order:

```ttx
state position : Vec3D = (screen_pos.[x, y], z);
```

The swizzle decomposes `screen_pos` into a positional pack, and the outer pack
adds `z`. The evaluator flattens that nested Pack before construction, so the
target sees three values.

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

`:[...]` is a compile-time decomposition operator. Its index, start, and count
must evaluate to Unsigned Constants before the positional Pack of Projections
is built. A dynamic range is a different operation because it produces one
typed view rather than a compile-time pack. The canonical dynamic form is
ordinary Self dispatch such as `value -> slice(start, count)`. Its receiver
Type proves the future Indexable contract and the Callable returns a View-like Type. C++
`View::Bytes::slice` follows the same dynamic shape.

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
7. The evaluator flattens nested positional packs before constructing the
   Pack whose Layout participates in fitting.
8. Named packs may fit Structured aggregates when every name and resolved
   identity matches exactly once. Core Layout fitting requires the complete
   value set. An ISA that permits omitted defaults obtains those defaults from
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

Executable syntax belongs to the ISA that gives it meaning. The shared TTX
Abstract graph supplies stable declarations, layouts, local identities, and the
narrow Expression queries. Expression identity does not resolve to its result
Type. `get_type()` publishes that fact separately, and `get_inputs()` publishes
the ordered dependency Layout. An ISA may attach an owned executable body to a
Callable, but that body representation is not a generic statement hierarchy in
the shared TTX model.

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
Bits_32 -> from(value) -> bit_and(Bits_32 -> from(0xFF));
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
lookup only: package lookup, type lookup, enum member lookup, or value field
lookup. `CallOp` is the only call marker. It requires a callable name and a
pack. The call base may be a value, `self`, a package or type query, or a future
function-pointer value. `IndexStart` parses constant-evaluated index or
index-slice content.
`SwizzleOp` parses swizzle fields or a swizzle slice. A swizzle or swizzle slice
produces a positional pack. `PackingStart` after a type expression is invalid.
Aggregate construction is pack fitting against an expected type, while explicit
conversion is ordinary `-> from(...)` dispatch.

Access forms:

| Syntax            | Meaning                                      |
| ----------------- | -------------------------------------------- |
| `.field`          | field, package member, or type member access |
| `:[index]`        | constant-evaluated index access              |
| `:[start, count]` | constant-evaluated index slice               |
| `.[a, b, c]`      | swizzle that produces a positional pack      |
| `-> name(pack)`   | callable dispatch from the left-side base    |

Calls always take a pack. If no arguments are present, the call still formats
as `receiver -> method()`. A type or package receiver resolves a Static callable.
An addressable receiver resolves its inferred Type identity, selects a Self
callable, and contributes the declared receiver parameter. Function-pointer
invocation is Self dispatch on the callable value, such as
`callback -> invoke(args)`.

The dispatch receiver must be concrete: a typed value, a type name, or a
package scope. A fluid pack is not a receiver because it has no concrete type or
addressable identity. To call through a value carried inside a pack, source or
the owning query must first select the concrete entry that is the receiver.

Dynamic slicing is callable dispatch, not relaxed `:[...]` syntax. A
receiver-defined `-> slice(start, count)` consumes runtime values and returns a
single View-like typed value. The receiver's future Indexable contract proves
element access and result Type facts. The compile-time `:[...]` operator instead
requires Constant arguments and produces projections or a positional pack.

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
```

Bare `self` is not an assignment target. It is a context root and requires an
access suffix. A package is a Type, not a lowercase pseudo-root. Package-owned
state is reached through the ordinary bound package or Type context.

Valid assignment targets include:

```ttx
value
value.field
value:[index]
value:[start, count]
value.[x, y]
self.field
```

Invalid examples:

```ttx
self = value;
value + other = 1;
receiver -> method() = value;
```

The parser handles this without speculative parsing: it parses the left
expression once, then checks whether the resulting shape is an assignable
address chain before accepting the assignment operator.

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

`if` and `while` take condition packs. The condition must fit the single-value
`Bool` Layout. Numeric truthiness is not implicit. Use an explicit comparison.

When an ISA supports `@if`, it can use the same statement shape as `if` while
evaluating the condition as compile-time data.

`match` patterns are expressions or `_`. Each case owns an explicit block.
There is no fallthrough.

`break;` and `continue;` are reserved loop-control statements. The active ISA
decides where they are legal. The syntax layer only preserves the statement
shape for later lowering.

## Literals

Literal classes:

| Source                 | Meaning                                     |
| ---------------------- | ------------------------------------------- |
| `123`                  | decimal numeric literal                     |
| `0xFF`                 | hexadecimal numeric literal                 |
| `0.5`                  | floating literal                            |
| `"Raw bytes"`          | quoted byte literal                         |
| `0x[AA FF 12 45 ACDE]` | byte data literal                           |
| `$[path/to/file]`      | embedded file data                          |
| `true`, `false`        | `Bool` literals                             |

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

Comments inside statement bodies remain lexical source facts for formatting and
diagnostics. They are not executable statements. Documentation belongs to the
type, member, or function that owns it and is preserved for formatter and LSP
queries. It does not participate in type identity or layout fitting.

Documentation is intentionally separate from identity resolution. The source
declaration or owning context may document why an Alias exists, while the
resolved target has its own documentation. A tool may present either source or
resolved documentation, or accumulate presentation while it follows the Alias
chain. Alias itself owns neither prose nor a `display_name` substitute.
Presentation does not participate in identity, Type equivalence, Layout
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

Diagnostics use human names from `Class::get_name()` and source spellings from
`Class::get_source_text()`. This keeps errors aligned with the token model:

```text
Expected definition `:` but got type
```

Semantic diagnostics prefer ranges that cover the meaningful construct, not
only the token where the compiler first noticed the problem. For example, an
invalid type argument list highlights the argument range when possible.

## Common Semantic Kinds

The common semantic vocabulary is intentionally small:

| Kind                               | Runtime model                  | Notes                                                 |
| ---------------------------------- | ------------------------------ | ----------------------------------------------------- |
| primitive numeric types            | value                          | fixed width, no implicit widening                     |
| `Bool`                             | value                          | only `true` and `false` are truthable by default      |
| `Vec[T, N]`                        | typed aggregate value          | homogeneous fixed-size aggregate                      |
| `Vec2D`, `Vec3D`, `Vec4D`, `Color` | typed aggregate value          | named components with stable semantic order           |
| layout                             | structural value stream        | anonymous heterogeneous aggregate                     |
| `struct`                           | nominal value                  | does not flatten implicitly                           |
| `object`                           | nominal managed value          | heap/reference semantics, ISA-limited                 |
| `foreign`                          | external ABI scope             | declarations lower to linked symbols                  |
| `Shader`                           | shader scope or ISA concept    | may lower to GPU module plus host glue                |
| `enum`                             | compile-time namespace         | members lower to constants                            |
| `alias`                            | compile-time Abstract redirect | preserves its authored name while redirecting queries |
| `Type`                             | compile-time type value        | stores a resolved Type reference                      |

`Library`, `Package`, `Render`, and `Shader` are ISAs. They remain PascalCase
type atoms in source. ISA registration decides what they mean instead of the
lexer.

## Lowering Direction

TTX maps naturally to LLVM-like IR concepts:

| TTX                           | Lowering idea                                            |
| ----------------------------- | -------------------------------------------------------- |
| package                       | module or compilation unit                               |
| import                        | module dependency or package alias                       |
| function                      | function definition or ISA-owned bodyless declaration    |
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
resolved Type
-> Terminal: query its family, size, and alignment
-> otherwise: walk its Structured Layout and recursively lower each Addressable
```

A non-empty Structured Layout is never collapsed to an invented scalar carrier merely
because a backend recognizes the outer Type name. A byte view, vector, struct,
render contract, and user aggregate are recursively deconstructed according to
their actual Addressables. The source value remains one semantic aggregate. The
terminal representation is its ordered projection. Calls, returns, stack
placement, register classification, generated host declarations, and archive
descriptions must consume the same projection.

An empty Layout alone does not say whether a Type is Terminal or merely an empty
composite. The resolved Type must prove `Terminal`. It can then be viewed as
`Unsigned`, `Signed`, `Real`, `Flag`, or another toolchain-defined Terminal
subtype. A lower compiler needs only those Type and Terminal interfaces. It does
not need to know whether the authored query reached the Type through an Alias,
Generic, shader context, or foreign-language object.

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
17. Lowering proves Terminal before reading a Type's Structured Layout.
    Non-Terminal Types recursively deconstruct every Addressable in that Layout,
    including the valid zero-entry aggregate case.
18. Layout is an ordered fitting contract over Abstracts. Fluid, Named, and
    Structured express distinct semantics through inheritance. There is no
    Member record, kind enum, or Incomplete Layout.
19. Expression identity remains distinct from its result Type. Constant is an
    immutable zero-input Expression, and its equality includes domain, resolved
    Type, and payload.
20. `:[...]` requires constant-evaluated Unsigned arguments and produces
    projections or a positional pack. Dynamic slicing is Callable dispatch that
    returns one View-like typed value.

These rules are what keep TTX readable while still letting it behave like a
compiler IR.
