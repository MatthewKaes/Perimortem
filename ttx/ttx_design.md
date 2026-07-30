# TTX Design

TTX is the narrow interchange boundary between lexical producers, semantic
domains, and downstream consumers. It owns only facts that remain meaningful
without knowing the concrete language, source transaction, package, target, or
runtime.

## One semantic graph

`Abstract` is the root of every semantic identity. Narrow contracts add only
the queries needed to exchange a fact across domains. A consumer retains the
real identity through `Reference` or another borrowed edge rather than copying
names and members into a shadow Type graph.

This boundary is deliberately closed for version 1:

```text
Abstract
├── Invalid
├── Alias
├── Type
│   └── Value
│       ├── Flag
│       ├── Real
│       ├── Signed
│       └── Unsigned
├── Addressable
└── Callable
```

`Documentation`, `Layout`, `Reference`, and `Attribute` do not inherit
`Abstract`. They support identities without acquiring identity or resolution
of their own.

## Progressive construction

A graph owner may reserve stable objects before every edge is ready. An
incomplete semantic query returns the shared `Invalid` object. Once a query
returns a valid identity, later enrichment may refine incomplete queries but
must not invalidate that earlier correct answer.

Construction failures, parser errors, invalid indexes, and failed Layout fits
are not graph identities. They use ordinary `Option` or `Union` results.
`Invalid` remains reserved for total semantic queries that must return an
`Abstract`.

## Resolution and category proof

`resolve()` returns the represented identity. `resolve_context(route)` asks the
receiving identity to interpret borrowed route bytes. TTX defines neither a
path grammar nor a central resolver.

`implements()` proves a public semantic category through its stable contract
identifier. `is<Category>()` performs a Boolean proof, while
`visit<Category>(match, mismatch)` dispatches without exposing an unchecked
narrowed reference.

There is no universal Kind, class database, semantic registry, copied member
record, nullable graph edge, or allocated path history.

## Layout fitting

Layout is an identity free, directional fitting contract over an ordered group
of real Abstracts. TTX supplies five common implementations:

* `Fluid` fits ordered values by represented identity.
* `Named` fits uniquely named values by name and represented identity.
* `Structured` retains the actual Addressable entries of a Type.
* `Ranged` repeats one real Abstract over a fixed interval.
* `Composite` combines two complete Layouts without flattening them.

Layouts retain no copied Type names, fields, documentation, physical offsets,
ABI rules, or target storage. Those facts remain on their semantic or target
owners.

## Language boundary

TTX includes `Type`, `Value`, `Addressable`, and `Callable` because they are the
shared exchange contracts for domains and signatures. It does not include the
rules that create or evaluate those facts.

Expressions, bindings, projections, constants, generic formulas, mutability,
receiver roles, executable bodies, concrete scalar Types, and publication
policy belong to the concrete language that defines their legality. Reuse by
several languages does not make such policy universal.

The normative contract is [ttx_semantics.md](ttx_semantics.md).
