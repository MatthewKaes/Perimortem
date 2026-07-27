# TTX Language Design

TTX is a host neutral source and semantic substrate. It owns lexical bytecode
plus the target independent graph catagorys that let concrete Dialects
exchange real semantic identities without copying them into private models.

TTX does not own a source file transaction, dialect composition, package,
filesystem, compiler target, linker, runtime, or any other toolchain concerns.

## Semantic graph

`Abstract` supplies identity, local naming, documentation, and contextual
resolution. Narrow catagorys add only the queries required by their semantic
meaning. `Reference` stores nonnull borrowed graph edges. `Documentation`,
`Layout`, `Body`, and Attributes remain identity free values.

These catagorys are one shared vocabulary. A concrete Dialect constructs only
the facts its grammar requires. Reuse never authorizes a second Type graph, a
central Kind table, or a copied member schema.

## Mutable construction

The graph may be assembled progressively as long as every semantic identity
has a stable address. An incomplete Type remains a real Type and its `resolve()`
query may return Invalid until its required facts are ready.

The graph is also flexible with qurries allowing for mutable state. Having retained
state is not a requirement of the TTX model, but without it the performance degrades
significantly for a large number of cases.

`Generic` for example can be implemented as formulas create resulting `Type` products
with a `resolve_context` mapping "arguments" to it's generated `Types`. Allowing `Generic`
to mutate it's subtree allows it to cache results rather than emitting factually
identical trees for every request.

### Context "layering"

All qurries on a TTX graph are non-nullable and stable. This requires all retained
state to be addative only. Failed or recursive attempts publish nothing and may be
retried as the graph context is enriched by progressive passes. `Invalid` can be
used to represent incomplete qurries but Abstracts need to be careful as `Invalid`
does not mean a querry could _never_ resolve. It simply denotes that the current
one didn't.

### Layer stability

Any query that returns a non-`Invalid` result is considered factually resolved for the
lifetime of the graph.

This invariant means that any valid query can be cached by its consumer or any Abstract
along a given resolution route.

## Graph queries

TTX's C++ API provides a wrapper for manipulating and querying the graph while using
the C++ object model.

`implements()` proves a public C++ catagory through its stable UUID. Native
consumers use `is<Catagory>()` for a Boolean proof and
`visit<Catagory>(match, mismatch)` for performing non-nullable queries.

`visit` delivers the real catagory to the match callback and the exact
`const Abstract&` to the mismatch callback. It never returns an unchecked
narrowed reference and never traps on ordinary input mismatch.

`Invalid` remains the absorbing result of a failed graph query but in C++ it provides
a stable fact (using `Ttx::Concept::Invalid::get_invalid()`) to enable reference
wrappers and to provide an address for `resolve`. `invalid.resolve()` always returns
itself.

Parser transactions, index bounds, fitting failures, and construction validation
use Perimortm's `Option` or `Union` instead so they do not inject `Invalid` into committed
graph edges.

## Layout

Layout is the identity free shape and directional fitting catagory. Layouts
allow information to be extracted from one domain into another even when the
current context is foreign to the source or target domain. Layouts support
this by connecting a `to` morphism from any class catagory to a `from` morphism
from any class catagory (including itself) without consulting either domain.

To perform these transactions the source and target classes are lowered to
one of five layout domains: `Fluid`, `Named`, `Structured`, `Ranged`, and
`Composite`. The source and target then negotiate in the layout domain and
if a possible `fitting` is caculated the target class can then use that to
exfiltrate that data without violating domain isolations. 

Layouts exist outside of the Abstract graph but negotiate fit using Abstract
querries with `Source` and `Target` layouts understanding only their host
domain. This allows real semantic edges and exposing fitting evidence without
allocating a second mapping or requiring a universal type system.

Layouts can consult any number of intermediary domains as part of negotiation
but they persist no abstract facts themselves. A layout can't contain Type names,
fields, documentation, attributes, physical offsets, ABIs, ect. Those facts stay
in their real semantic graphs or target owners.

## Type and executable facts

While TTX aims to remain domain agnostic the data model chooses to be opinionated
in areas that aid optimizing for it's core use case (compilers, game engines, ect).

Since many of these systems are built off of (and need to integrate with) existing
systems TTX provides default domains to ease integration:

The active shared domain for the data model includes:

```text
Abstract
├── Documentation
├── Attribute
├── Alias
├── Invalid
├── Layouts
│   ├── Composite
│   ├── Named
│   ├── Range
│   ├── Structured
│   └── Fluid
├── Type
│   ├── Value
│   └── Proto types for common type systems (flag, real, unsigned, signed)
├── Addressable
└── Callable
```

These domains are not _required_ but avoiding them will typically require bootstrapping
a large amount of parallel infastructure. In particular the `Layouts` domain generalizes
much of the layout lowering for fit negotiations that any sensible domain would need.

## Host boundary

TTX defines no universal source or binary format.

A host owns those concrete systems. It decides Source lifetime, parser
dispatch, Namespace and Workspace policy, package and filesystem behavior,
concrete Dialect grammar, graph finalization, target lowering, linking,
runtime state, and archive formats.

The normative catagory is [ttx_semantics.md](ttx_semantics.md).
