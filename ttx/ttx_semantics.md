# TTX Language Semantics

This document is the normative catagory for the host-neutral TTX
representation. It defines lexical token bytecode, the shared semantic graph,
identity-free Layouts, and the common values consumers may construct.

Concrete parsing policy belongs to the parser that consumes the Tokens.
Concrete Dialect, package, filesystem, runtime, target, linker, and archive
policy belongs to those owners. A repository consumer does not become part of
the TTX specification merely because it uses TTX Codes or model catagories.

## Formal model

TTX has two layers:

- lexical analysis maps authored bytes to an ordered Token stream whose Codes
  expose stable local categories; and
- consumers evaluate those Tokens into shared semantic identities and
  identity-free values.

The Token stream is deterministic for one concrete Lexer catagory. Semantic
validity is contextual because name resolution, catagory proof, Generic
materialization, Layout fitting, and consumer legality depend on the graph
supplied by the host.

TTX does not require a parse tree or universal semantic pass between those
layers. A consumer may construct real semantic facts directly while consuming
Tokens. If later completion is required, it walks those facts rather than
replaying a Cursor or retaining token bookmarks as unfinished meaning.

## Token bytecode

A concrete Lexer emits one Token for every recognized source span. Each Token
carries an eight-bit `Lexical::Code`. `Terminal` reserves `0x00` and `Unknown`
reserves `0xFF`. Every other value belongs to the exact Lexer catagory that
assigned it.

Code values are not a stable independent serialization. A Token stream can be
exchanged only with the matching Lexer and Lexicon catagory.

Payload-bearing Tokens retain the source coordinates needed to project their
authored bytes. The Token does not copy text, allocate a path, or own source
lifetime. A consumer obtains text through the Tokenizer-owned source.

Tokens are the smallest decoded units, not fixed-width language instructions.
A consumer decides whether one instruction uses one Token or an ordered span.
That decision must be deterministic and must not reinterpret a Code after it
has already been consumed.

### Lexical categories

The common categories include:

| Source shape               | Code role                    |
| -------------------------- | ---------------------------- |
| `snake_case`               | addressable name             |
| `PascalCase`               | Type-shaped name             |
| fixed grammar words        | dedicated keyword Code       |
| publication and evaluation | dedicated modifier Codes     |
| `@name`                    | Attribute                    |
| `0x[...]`                  | byte literal                 |
| `$[...]`                   | embedded-resource operand    |
| punctuation and operators  | dedicated delimiter/operator |
| comment line               | Comment                      |

The Lexicon owns fixed spellings. Tokenizers, diagnostics, and source emitters
reuse that mapping instead of duplicating keyword and operator text.

TTX defines the category of an embedded-resource operand but not how bytes are
loaded. It defines the category of a Dialect marker but not which Dialect names
a host accepts.

## Semantic object model

The shared semantic model is a directed graph of `Concept::Abstract` objects.
An object may be reachable through several bindings or Alias edges, so it does
not store one authoritative parent path.

The catagories are:

| Catagory      | Required meaning                                                                                         |
| ------------- | -------------------------------------------------------------------------------------------------------- |
| `Abstract`    | semantic identity, local name, documentation, and contextual resolution                                  |
| `Type`        | a self contained domain that defines zero or more sub domains depending on context                       |
| `Value`       | a self contained domain that is context free for all resolutions (`resolve()` == `resolve_context(...)`) |
| `Alias`       | a unit domain that forwards to another domain for all resolutions                                        |
| `Invalid`     | a unit domain that contains exactly itself as a sub domain for all resolutions                           |
| `Addressable` | a stable name inside a domain that maps an edge to another `domain` or `layout`                          |
| `Callable`    | a stable name inside a doamin that maps an edge to a `layout`                                            |

`Layout`, `Documentation`, and `Body` remain identity-free catagories or values.
They do not inherit Abstract to gain discovery.

There is no universal Kind, `Typed` marker, class database, mutable semantic
registry, copied Type tree, nullable member record, or mandatory reflection
field.

## Catagory proof

Every declared semantic catagory owns a stable 128-bit
`Perimortem::System::Uuid`. An implementation recognizes its declared catagory
and delegates unrecognized identifiers through its public base chain.

Catagory identifiers identify interfaces only. They are not object identity,
export identity, cache keys, path hashes, package versions, or serialized
handles.

Native consumers use:

```text
abstract.is<Catagory>()                 -> Bool
abstract.visit<Catagory>(match, mismatch)
```

`is()` proves the public catagory. `visit()` calls the match function with the
real catagory or the mismatch function with the exact `const Abstract&`.
Fallible narrowing does not trap or expose an unchecked reference. Semantic
queries report `Invalid` through their own boundary. Parser and construction
transactions use Option or Union instead of treating a mismatch as graph
state.

An implementation may report only catagories represented by its public C++
inheritance. C++ implementation reuse does not manufacture another semantic
fact.

## Resolution

Abstract exposes two total operations:

```text
resolve()                 represented identity
resolve_context(route)    resolution of borrowed bytes in this context
```

The receiving Abstract owns route interpretation. It may compare the view
atomically, consume a prefix, pass a suffix, or redirect the unchanged view.
TTX prescribes no path object, separator grammar, traversal algorithm, or
resolution cache.

The required rules are:

| Rule             | Catagory                                                              |
| ---------------- | --------------------------------------------------------------------- |
| local name       | `get_name()` names the current Abstract                               |
| represented name | `resolve().get_name()` names represented identity                     |
| identity         | `resolve()` is idempotent for an unchanged valid graph                |
| borrowed route   | `resolve_context()` receives the caller's complete borrowed view      |
| partitioning     | one combined route and several ordered queries need not be equivalent |
| determinism      | the same ordered chain is stable while the graph is unchanged         |
| termination      | a valid graph cannot redirect forever                                 |
| failure          | failure returns `Invalid&`, never a nullable pseudo-Abstract          |

Alias retains its local name, local documentation, and one borrowed target. It
redirects both resolution operations through the target's represented identity.
The graph owner guarantees target lifetime and rejects cycles before the Alias
becomes queryable.

Diagnostics retain the authored input and failed boundary on the source-owning
consumer. Abstract and Alias do not retain route history for presentation.

## Invalid and total references

`Invalid : Abstract` is the binary-wide semantic failure object. It is closed,
stateless, and absorbing: further resolution through Invalid returns the same
Invalid object.

Invalid does not store the failed route, diagnostic, source range, package
state, recovery choice, or specialized error category. Those facts remain on
the owner that performed the failed query.

Empty collections use empty views. Semantic absence never uses a null pointer
or a local sentinel. Narrow operations become reference based after their
preconditions are proven.

`Concept::Reference<Catagory>` is the non-null borrowed edge used by contiguous
semantic collections. It preserves the object supplied by its owner; consumers
call `resolve()` explicitly when represented identity is required.

## Exported surfaces

`Exports : Abstract` is the optional public-definition catagory. It supplies:

```text
get_export_count()  -> Count
get_export(index)   -> const Abstract&
```

Every valid index returns the real exported edge in authored publication order.
An invalid index returns Invalid. Every exported edge has a unique non-empty
local name, and contextual lookup of that name returns the same edge.

Alias remains visible at the export boundary so authored naming and
documentation survive. Consumers resolve it only when they need represented
identity.

Direct lookup is closed over the enumerated surface. Private roots, outer host
bindings, storage Layouts, dependency locators, and source records do not leak
unless the owner publishes a real edge for them.

TTX defines no universal Namespace, Source, module, package, or Group. A host
may implement one as an Abstract and prove Exports when its public roots are
enumerable.

## Types

`Type : Abstract` represents semantic value identity. A resolved valid Type
returns one total `const Layout&`. Layout has no incomplete alternative.

An empty Layout does not prove that a Type is scalar. A consumer must prove
`Terminal` or another narrow representation catagory before applying scalar
rules.

`Managed : Type` proves only that values are managed object references.
Allocation, tracing, movement, object headers, pointer width, address spaces,
and runtime storage belong to runtime or target owners.

`Terminal : Type` adds:

```text
get_width()      -> Count
get_size()       -> Count
get_alignment()  -> Count
```

Width describes the value domain. Size and alignment describe storage. Neither
dictates register or instruction width.

`Unsigned`, `Signed`, `Real`, and `Flag` provide the common terminal domain
proofs. The width-specific TTX classes return literal names, fixed values, and
generated documentation directly. TTX owns no prelude, supported-width
registry, or required set of installed instances.

`Unsigned_8` is the common byte element Type. There is no second scalar Byte or
Bytes Type identity. A concrete language may define a byte array value domain
whose resolved collection Type remains distinct from its element Type.

## Generic materialization

`Generic : Abstract` is an immutable Type-construction instruction. It is not a
Type and has no Layout.

A Generic publishes its complete ordered parameterization before arguments are
consumed. The parameter alternatives are:

```text
const Type&
Unsigned_64
Signed_64
Bool
```

The graph-construction transaction owns
`Generic::Materializations`. Its exact key is the Generic identity plus ordered
arguments. Type arguments compare by resolved identity; scalar arguments
compare by value. Names, parents, routes, rendered spellings, and hashes do not
participate in identity.

The writer is append only:

- an existing exact key returns the same stable Type address;
- a successful new key appends one materialization;
- rejection or incomplete readiness appends nothing;
- direct or indirect same-key recursion appends nothing;
- a rejected key may be retried after graph readiness advances; and
- the first success for a key is irrevocable in that writer.

Formula identity, parameterization, and construction rules are immutable.
Referenced graph facts may become ready monotonically, but a formula must
reject a key until every fact that could change a successful result is ready.

The writer borrows Generic and Type identities and copies compact scalar
arguments. Their owners outlive its last query.

## Layout

`Layout` is the directional fitting catagory over an ordered group of real
Abstracts:

```text
get_size()                    -> Count
get_abstract(index)           -> Option<const Abstract&>
fits(target)                  -> Bool
get_fitted(target, index)     -> Union<const Abstract&, Layout::Errors>
```

The closed errors are `IndexOutOfBounds`, `SizeMismatch`, and
`IncompatibleFit`. An invalid index has no value. Layout transaction failures
do not inject Invalid into the semantic graph.

The shared implementations are:

- `Fluid`: positional value flow;
- `Named`: uniquely named value flow fitted by name;
- `Structured`: the actual Addressables owned by a Type;
- `Ranged`: one real Abstract repeated over a fixed interval; and
- `Composite`: positional composition of two complete Layouts.

Fitting is directional: source fits target. Fluid and Named entries fit by
resolved identity. Structured preserves real Addressable identity. Ranged
returns the same semantic edge for every valid position. Composite delegates
to the child that owns each segment.

`get_fitted()` returns the original source edge supplying a target slot. Named
therefore exposes the permutation it proved without allocating a second
mapping.

Layout stores no copied field name, Type, documentation, attributes, default,
offset, storage class, or target representation. Additional facts remain on
the real semantic object or a narrower owner.

Structural coincidence does not create Type identity. A typed aggregate does
not flatten into Fluid flow without an explicit source operation.

## Addressable and Callable

`Addressable : Abstract` supplies the real Type edge of data reached through a
named address. The Type remains a stable identity while its own `resolve()` may
return Invalid until graph construction completes.

`Writable : Addressable` proves assignment capability. Its read-only projection
is a separate stable Addressable with the same name, documentation, and
resolved Type. It does not prove Writable and does not resolve back to the
writable identity.

Write capability on an address is distinct from the Type of the loaded value.
Removing Writable does not rewrite a loaded `Access[T]` into `View[T]`.

`Callable : Abstract` supplies complete parameter and result Layouts. `Static`
is selected without a runtime receiver. `Self` includes the receiver exactly
once at parameter zero. Invocation, reflection, fitting, and lowering consume
that same complete signature.

Owning a Callable beneath a Type does not create another subtype. Linkage,
machine address, runtime invocation, and target ABI remain on narrower consumer
catagories.

## Executable Body

`Body` is an immutable identity-free executable value retained by its concrete
semantic owner. It is not an Abstract, AST, parse tree, scope graph, Cursor
snapshot, or second Type graph.

A Body contains compact tables:

- blocks own ordered operation ranges and explicit terminators;
- local value IDs identify parameters, locals, and produced values;
- operand and result Types live once in the value table;
- operations retain local IDs and only the real graph edges required by their
  shape;
- loads and stores refer to real Addressables, with stores requiring write
  proof;
- calls refer to real Callables or explicit intrinsic owners; and
- branches and returns use local block and value IDs rather than source
  positions.

Every result-producing operation declares its result Type and produces one
fresh local value ID. Every ID, range, control edge, Type, and graph edge is
validated before publication.

The operation alternatives describe structural shapes. A binary operation can
store one consumer-owned bytecode and exactly two local operands without TTX
defining the operator's Type rule. Domain operations remain calls to their real
owners rather than universal opcodes.

Source Tokens and Cursor state are consumed construction inputs. They never
survive as executable authority. A later pass walks Body and semantic owners;
it does not reopen the Tokenizer.

## Documentation

`Documentation` is an identity-free ordered view of comment lines. Every
Abstract returns a stable Documentation reference.

An authored owner may borrow arena-backed `Comments`. A generated concept may
return one `Comment`. Missing documentation returns the shared empty Comment.
Alias may expose its local lines followed by the target's visible lines.

Documentation does not participate in resolution, semantic identity, Type
equality, Layout equality, or fitting.

## Derived consumers

Target representation is derived from real Type, Layout, Addressable, Callable,
Body, and consumer-specific facts. It may contain offsets, alignments, pointer
forms, address spaces, storage classes, descriptor coordinates, register
classes, or ABI carriers for one compilation. Those records are not TTX
semantic identities and do not become Layout fields.

Runtime storage is likewise derived. Managed cells, frames, collectors,
workers, process addresses, and scheduling are not TTX facts.

Evaluation identities such as Expression, Binding, Projection, and Constant
belong to the concrete language that defines their legality and value domains.
They may retain real TTX Type, Layout, and Addressable edges without becoming
part of the core TTX model.

Archives, repositories, source caches, package manifests, filesystem roots,
and diagnostics remain outside TTX. A durable owner may serialize selected
semantic facts, but it must not treat target records, process addresses,
Cursor positions, or source paths as semantic truth.

## Design invariants

Changes to TTX preserve these rules:

1. The Tokenizer assigns one Code to every emitted source span.
2. A Token stream is interpreted only with its exact Lexer catagory.
3. Token text is projected through Tokenizer-owned source rather than copied
   into every Token or parser object.
4. A selected consumer consumes authored syntax once. Token indexes and Cursor
   bookmarks never stand in for unevaluated semantic facts.
5. Every queryable semantic identity implements Abstract. Type is not the
   universal base.
6. Layout, Documentation, and Body remain identity free.
7. Semantic failure returns Invalid, never a null pseudo-Abstract.
8. Resolution passes borrowed bytes and leaves route partitioning with the
   receiving Abstract.
9. Alias preserves local identity and documentation while redirecting
   represented identity.
10. Exports enumerates the exact public edges available through its context.
11. Type returns a total Layout after successful resolution.
12. Terminal is proven before scalar storage rules are applied.
13. Generic is an immutable construction instruction, not a Type.
14. Materialization identity is formula identity plus ordered semantic
    arguments; the first successful exact key remains stable.
15. Layout owns order and directional fitting, not copied members or physical
    representation.
16. Writable is the core write-capability proof for durable Addressables.
17. Self includes its receiver exactly once at parameter zero.
18. Body contains compact local IDs and real semantic edges; it is never a
    replayable parser or a competing semantic graph.
19. Target, runtime, diagnostic, filesystem, package, linker, and archive facts
    stay on their own owners.
