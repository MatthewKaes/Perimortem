# TTX Language Design

TTX is a human-authored source IR. It keeps the source readable while exposing
enough structure that token consumers do not have to rediscover whether a span
is a name, modifier, call, layout, literal, or operator.

TTX does not name or select a reference host. A compiler, editor, formatter,
interpreter, or interchange boundary may consume it by implementing the same
lexical and semantic contracts.

## Design goals

TTX favors visible structure and cheap deterministic consumption:

- lexical classification carries stable local meaning;
- semantic identity remains on real graph owners;
- identity-free shape is not promoted into another object graph;
- failed semantic queries return a real `Invalid` object;
- parsing and evaluation do not create shadow Types or copied member records;
- target, runtime, diagnostic, and durable facts stay on their own owners; and
- source and Cursor state are construction inputs rather than executable
  authority.

This places TTX between a conventional source language and a lowered compiler
IR. It retains authored names, comments, modifiers, attributes, layouts, and
value-flow shape, but it does not prescribe a universal module system,
filesystem, runtime, backend, or archive.

## Source IR and token bytecode

The Tokenizer emits one ordered stream of Tokens. Every Token carries a
`Lexical::Code` describing the prescribed category of its source span.
Payload-bearing Tokens retain their source coordinates so diagnostics and tools
can project the authored bytes through the Tokenizer-owned source.

The stream is variable width at the instruction level. A consumer may treat one
Token as an instruction or consume a larger span for a declaration, layout,
function, or body. TTX fixes the lexical categories; the selected parser or
Dialect fixes the instruction boundary and legality.

The concrete Code values are part of the Lexer contract. They are not a stable
standalone serialization format and cannot be interpreted without the matching
Lexicon and Tokenizer behavior.

## Lexical shape

Casing and punctuation make common categories visible:

| Form          | Lexical role                                      |
| ------------- | ------------------------------------------------- |
| `snake_case`  | addressable name                                  |
| `PascalCase`  | Type-shaped name                                  |
| `.snake_case` | named field or member selector                    |
| `.10`         | indexed designator                                |
| `@name`       | attribute                                         |
| `_`           | discard                                           |
| `(...)`       | grouped value flow                                |
| `[...]`       | layout, arguments, index, or fixed slice by owner |

Fixed grammar words and operators receive their own Codes. The parser does not
reclassify `alias`, `func`, `dialect`, publication modifiers, evaluation
modifiers, control-flow words, or fixed operators from ordinary identifiers.

Whitespace is not semantic. The formatter may choose a canonical presentation,
but changing whitespace does not change the Token categories.

## Shared semantic graph

The shared semantic model is a directed graph of `Abstract` identities.
Objects can be reached through several bindings or Alias edges, so an Abstract
does not store one authoritative parent path.

The root contract is intentionally small:

```text
Abstract
├── Alias
├── Invalid
├── Exports
├── Type
│   ├── Terminal
│   └── host-defined Type contracts
├── Generic
├── Expression
│   └── Constant
├── Addressable
│   └── Writable
└── Callable
    ├── Static
    └── Self
```

`Layout`, `Documentation`, and `Body` are identity-free contracts or values.
They do not inherit Abstract merely to become queryable.

Consumers first resolve identity, then prove the narrow contract they require.
A Type consumer does not need to know whether an authored query passed through
an Alias, a Generic materialization, or a host-owned context.

## Resolution

`Abstract::resolve()` returns represented identity.
`Abstract::resolve_context(route)` gives the complete borrowed route to the
current owner. The receiving Abstract decides whether to compare the route
atomically, consume a prefix, forward a suffix, or redirect the unchanged view.

TTX does not allocate a Route object, prescribe one separator, retain path
history, or choose a preferred Alias. Repeating the same ordered query chain
from the same Abstract in an unchanged valid graph returns the same identity.

Failure returns the binary-wide `Invalid` object. It never returns a nullable
pseudo-Abstract. Diagnostics retain the authored route and source range on the
source-owning consumer rather than storing them on Invalid.

## Types and Generic materialization

`Type` represents a real semantic value identity and supplies one total Layout.
A Type is not the root of every semantic object.

`Terminal` narrows Type with value width, byte size, and byte alignment.
`Unsigned`, `Signed`, `Real`, and `Flag` add domain meaning. The width-specific
TTX classes publish their direct fixed facts, but TTX owns no global prelude or
mutable supported-Type registry. A host chooses which concrete instances are
available from its context.

`Generic` is an immutable Type-construction instruction, not a Type. It exposes
an ordered parameter signature whose alternatives are Type, unsigned integer,
signed integer, and Boolean values. A graph-construction transaction owns the
append-only materializations created from one exact formula identity and one
exact ordered argument list.

The first successful key remains stable. Rejected, incomplete, or recursively
active keys append nothing and can be retried after graph readiness advances.
Names, routes, parents, formatted text, and hashes are not semantic identity.

## Layout and value flow

Layout owns ordered shape and directional fitting. It stores no offsets,
pointer widths, address spaces, target alignment, ABI carriers, runtime
collector policy, documentation, defaults, or copied member records.

The shared Layout contracts are:

- `Fluid` for positional value flow;
- `Named` for uniquely named value flow;
- `Structured` for the real Addressables owned by a Type;
- `Ranged` for one repeated semantic edge over a fixed interval; and
- `Composite` for positional composition of complete child Layouts.

Fitting is directional: a source Layout proves whether it can supply a target
Layout and which original source Abstract supplies each target slot.
Structural coincidence never creates Type identity. A typed aggregate also
does not flatten into value flow without an explicit source operation.

Target storage is derived later. A target may compute offsets, alignments,
register classes, storage classes, descriptor coordinates, or ABI carriers,
but those records do not become Layout facts or semantic identity.

## Expressions and constants

`Expression` is one evaluatable value. It remains distinct from its result Type
and publishes its Type and ordered input Layout through narrow queries.

`Constant` is an immutable zero-input Expression already in normal form. The
shared domains are unsigned integer, signed integer, real, flag, and bytes.
Equality includes the domain, resolved Type, and payload. TTX has no native
String Constant; a host may build String semantics above byte values.

`Projection` retains a receiver and one real selected Addressable. `Binding`
gives another Expression an authored flow name. Neither becomes a symbol table,
scope, member record, or alternate resolution graph.

Parsing, operator legality, folding, and execution remain consumer concerns.
They may construct these shared facts but do not broaden their contracts.

## Addressable and Callable

`Addressable` is a named address to typed data. `Writable` is the narrower proof
that assignment may target that address. Its stable read-only projection keeps
the same name and resolved Type without proving Writable or resolving back to
the writable identity.

`Callable` owns complete parameter and result Layouts. `Static` is selected
without a runtime receiver. `Self` includes its receiver exactly once at
parameter zero. Ownership beneath a Type does not create another Callable
subtype.

Machine addresses, linkage, relocation, runtime storage, and invocation policy
belong to narrower consumer contracts. Core Callable and Addressable do not
grow those facts for convenience.

## Executable Body

Body is the common identity-free executable representation. A concrete
executable semantic owner retains it.

Body uses compact local IDs for blocks and values. Operations retain local
operands and only the real semantic edges required by their shape. Calls refer
to real Callables, loads and stores refer to real Addressables, and produced
values carry their proven Types.

Source Tokens and Cursor positions never remain the executable program. A
consumer walks the Body and semantic graph; it does not replay the Tokenizer or
maintain a second statement tree for each target.

Different consumers may accept different validated Body subsets. Their
execution storage and target records remain derived products.

## Documentation

Documentation is a stable ordered view of comment lines. It has no semantic
identity and does not participate in resolution, Type equality, Layout fitting,
or object identity.

`Comment` supplies one generated line or the shared empty result. `Comments`
borrows authored lines. Alias may compose its local documentation before the
target's visible documentation without modifying either owner.

## Host extension boundary

TTX defines no canonical Dialect catalogue, Source class, Environment, package
grammar, member ordering policy, filesystem transaction, Foreign ABI, runtime
lifecycle, target representation, or archive format.

A host may reserve lexical markers and build those systems above the shared
contracts. It documents their grammar and behavior with their real owners,
outside the TTX specification.

The normative contract is [ttx_semantics.md](ttx_semantics.md).
