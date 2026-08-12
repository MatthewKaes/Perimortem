# TTX

TTX is a small semantic interchange vocabulary for live multi domain semantic
IRs. It gives concrete languages shared contracts for identity, resolution,
Types, Addressables, Callables, and semantic Layout without requiring them to
share one syntax tree or one universal type system.

A language still owns the meaning of its source. TTX owns only the contracts
that another language, package system, compiler, editor, or runtime can use
directly. Those consumers can meet in one live graph and refer to the same
semantic object instead of copying it into a model of their own.

Tetrodotoxin is the reference host in this repository. It supplies Workspaces
that use TTX to retain one live semantic IR of concrete language objects.
Tetrodotoxin also supplies packages, compilers, and runtime policy while TTX
remains independent of all of them.

## Where TTX sits

TTX lives between source interpretation and the representations used by a
particular compiler or tool:

```text
authored source
-> Tokens with exact source locations
-> semantic objects constructed by a concrete language
-> shared queries over one live graph
-> LLVM IR, SPIR-V, editor data, semantic archives, or another Terminal product
```

The live semantic IR keeps facts that are still meaningful before a target,
runtime, or output format has been chosen. It can tell a consumer which exact
Type an Alias represents, which Type an Addressable reaches, which Layout a
Callable accepts, or whether one Layout fits another. It does not decide how
those facts become registers, offsets, object records, editor messages, or
serialized bytes.

This makes TTX earlier than LLVM IR in a compiler pipeline. LLVM IR describes
computation after a language has chosen a lowered form suitable for optimization
and code generation. TTX describes the semantic identities that must remain
available while languages and tools are still composing the program. A project
can use TTX for that shared semantic layer and LLVM IR for CPU compilation.

## The shared vocabulary

### Source and Tokens

The TTX Lexer turns authored bytes into an ordered Token stream. Each Token
retains its source coordinates and a `Lexical::Code`. Payload text continues to
come from the original source view.

Tokens preserve spelling and location without imposing one grammar on every
language. A concrete language can construct retained semantic objects while it
reads the stream. TTX does not require a retained syntax tree. Another host may
own a source model when its tools require one. Tetrodotoxin constructs
Monographs directly from Tokens and retains no second source graph.

### Semantic identities

Every queryable semantic identity is an `Abstract`. The closed TTX
categories are:

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

`Layout`, `Documentation`, and `Reference` support those identities without
becoming identities themselves.

A `Type` exposes one complete Layout. An `Addressable` names typed data. A
`Callable` exposes parameter and result Layouts. An `Alias` keeps its own local
name and Documentation while resolving to another identity. `Invalid` is the
total result of a semantic query that cannot be answered at that stage.

A Reference retains a nonnull borrowed edge to the exact object
constructed by its owner. Equal names, equal structure, and equal Layouts do not
make two objects the same Type. That distinction lets packages, languages,
editors, and compilers share a fact without maintaining synchronized copies.

### Semantic Layout

A Layout is an ordered view of exact semantic identities and a directional
fitting contract. TTX supplies five common forms:

* `Value` contains one exact atomic Type as a terminal leaf.
* `Fluid` fits ordered values.
* `Named` retains uniquely named entries and fits them by name.
* `Ranged` repeats one entry over a fixed interval.
* `Composite` joins complete Layouts without flattening them.

Layouts describe semantic shape and how one ordered view fits another. A
leaf Type contributes its own one-entry Value Layout, while an empty Layout
carries no value and fits every other empty Layout. An Addressable therefore
requires a Type with at least one Layout entry. Scalar Values separately define
abstract machine width and storage requirements. Target object layout, field
offsets, registers, address spaces, pointer forms, and runtime storage belong to
the consumer that chooses a physical representation.

### Terminal products

A Terminal product is a completed output whose use no longer depends on the
live graph or its process identities. LLVM IR, SPIR-V words, debug data, object
modules, native binaries, editor data, formatted source, and semantic archives
are examples.

Terminal describes a boundary role rather than one common product model. Each
compiler, linker, formatter, editor service, or package owner defines the
format it emits. No live `Abstract` identity or `Reference` survives in that
output.

Most Terminal products intentionally preserve only the facts needed by their
next consumer. LLVM IR, debug data, and object files cannot reconstruct the
complete source semantic graph. A semantic archive has a different purpose. It
can avoid source acquisition, lexing, and parsing by retaining the owner facts
needed to create a fresh graph and apply the graph owner's validation,
completion, and publication contract.

Reconstruction equivalence concerns public semantic observations rather than
internal graph shape. Names, categories, represented identity relationships,
semantic edges, order, Layout behavior, completion, and concrete owner facts
remain observable. Process addresses and private supporting structure do not.

## Relationship to other compiler models

[LLVM IR](https://llvm.org/docs/LangRef.html) is the natural companion when a
completed program needs optimization and machine code. It is already past many
of the source and language decisions that TTX keeps available.

[MLIR](https://mlir.llvm.org/) is a closer comparison when a project needs an
extensible family of intermediate representations and rewrite pipelines. TTX
also lets independently owned domains compose, but they meet through semantic
identities rather than expressing every domain as operations and values. MLIR
is a stronger fit when progressive transformation is the center of the design.
TTX is aimed at the earlier problem of sharing meaning before the project
commits to one lowered representation.

A language AST remains the direct choice when one frontend owns the source and
needs a rich model of its declarations and syntax. Clang is the stronger choice
when faithful C or C++ semantics and its mature tooling ecosystem are the
product. TTX gains a smaller cross language boundary by leaving those rich
rules with each concrete language. The cost is that TTX does not provide them
on the language's behalf.

## Is TTX a fit for my project?

TTX is designed for projects where several languages or semantic domains must
compose before lowering, or where packages, editors, compilers, and runtimes
would otherwise keep translating the same facts into private models. It is
particularly useful when exact identity matters across those boundaries and
when one completed graph must feed several independent products.

That flexibility asks more of a project than a conventional compiler stack.
Every concrete language must own its grammar, semantic objects, diagnostics,
and the work it contributes to the host's completion barriers. A persistent
language must also define and validate the facts needed to reconstruct its part
of the graph. Persistence is optional. TTX does not provide a generic AST,
optimizer, ABI model, target layout engine, debugger model, or graph
serializer.

Another foundation is probably a better fit when the project begins with
lowered computation, when operation rewriting is the main abstraction, when C
or C++ compatibility is required, or when one language can be served cleanly by
one AST and one typed intermediate representation. TTX is a semantic companion
to downstream compiler infrastructure, not a drop in replacement for the LLVM
or Clang ecosystems.

## Further reading

* [TTX design](ttx_design.md) explains why the graph, identity, Layout, and
  Terminal boundaries take this form.
* [TTX semantics](ttx_semantics.md) is the normative lexical and semantic
  contract.
* [Tetrodotoxin](../tetrodotoxin/README.md) shows how the reference host
  composes concrete languages around TTX.
