# TTX

TTX is a compact, host neutral vocabulary for source and semantic facts. It
defines lexical token codes, stable semantic identities, and Layouts that
describe how values fit together. A language host can interpret those facts,
inspect them, or lower them without first copying them into another model.

Tetrodotoxin is the reference host in this repository. It supplies concrete
languages, packages, workspaces, compilers, and runtime policy while TTX remains
independent of all of them.

## Source vocabulary

The TTX lexer turns authored bytes into an ordered Token stream. Tokens retain
their source coordinates and a `Lexical::Code`; payload text continues to come
from the original source view. Fixed words, names, literals, delimiters, and
operators therefore remain distinct without forming a universal syntax tree.

Distinct operator and delimiter spellings receive distinct lexical Codes. TTX
preserves those Codes without assigning them one universal expression grammar
or result policy. A concrete language assigns grammar and result contracts while
consuming the exact semantic interfaces it requires.

Type, Addressable, Callable, and the supporting Layout contract remain
independent semantic interfaces. Sharing a name or appearing in one concrete
language construct does not merge them into a common member model.

## Semantic vocabulary

Every queryable semantic identity implements `Abstract`. The closed TTX version
1 categories are:

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

`Layout`, `Documentation`, `Reference`, and `Attribute` support those identities
without becoming identities themselves.

A `Type` exposes one complete Layout. An `Addressable` names typed data. A
`Callable` exposes parameter and result Layouts. An `Alias` keeps its own local
name and documentation while resolving to another identity. `Invalid` is the
total result of a semantic query that cannot be answered.

## Layouts

A Layout is an ordered view of real semantic facts and a directional fitting
contract. TTX supplies four common forms:

- `Fluid` fits ordered values.
- `Named` retains uniquely named entries and fits them by name.
- `Ranged` repeats one entry over a fixed interval.
- `Composite` joins complete Layouts without flattening them.

Layouts describe semantic shape. Target size, field offsets, registers, pointer
forms, and runtime storage belong to the terminal that lowers a completed graph.

## Further reading

- [TTX semantics](ttx_semantics.md) is the normative lexical and semantic
  contract.
- [TTX design](ttx_design.md) explains the graph, resolution, and Layout model.
- [Tetrodotoxin](../tetrodotoxin/README.md) documents the reference host and its
  languages.
