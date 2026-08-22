# TTX

TTX is the shared semantic vocabulary that makes Tetrodotoxin extensible. It
gives purpose specific languages a common way to describe Types, values,
addresses, Callables, Layouts, source locations, and documentation while each
language keeps the model that makes its domain useful.

A Package manager, editor, compiler, or runtime can ask those common questions
of the original language object. The answer stays attached to one identity
instead of being copied into a private model for every tool.

[Tetrodotoxin](../tetrodotoxin/README.md) brings those objects together in one
Workspace. TTX remains small enough to be used by another host, but its clearest
role in this repository is the semantic foundation shared by the complete
Tetrodotoxin platform.

## Where TTX sits

TTX connects authored source to every part of the platform that needs to
understand its meaning:

```text
authored source
-> Tokens with exact source locations
-> language objects created from those Tokens
-> shared questions about the completed program
-> LLVM IR, SPIR-V, editor data, Archives, or another final product
```

The live graph keeps facts that still matter while languages and tools are
putting the program together. A tool can ask which Type an Alias represents,
which Type an Addressable reaches, which values a Pack supplies, which Layout a
Callable accepts, or whether one Layout fits another. Physical registers,
offsets, editor messages, and stored bytes appear later in the component that
owns each product.

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

Every language object that tools can query belongs to one of these TTX
categories:

```text
Abstract
├── Invalid
├── Alias
├── Type
├── Addressable
├── Pack
└── Callable
```

`Layout`, `Documentation`, and `Reference` describe or connect those objects
without becoming separate language objects themselves.

A `Type` exposes one complete Layout. An `Addressable` names typed data. A
`Pack` carries produced value flow and exposes its complete output Layout. A
`Callable` exposes parameter and result Layouts. An `Alias` keeps its own local
name and Documentation while resolving to another identity. `Invalid` is the
total result of a semantic query that cannot be answered at that stage.

A Reference points to the object created by its owner. Equal names, structures,
and Layouts do not make two objects the same Type. This distinction lets
Packages, languages, editors, and compilers share one fact instead of keeping
several copies synchronized.

### Packs and semantic Layout

A Pack preserves the identity of produced value flow without turning that flow
into a Type. It may supply no values, one value, or several positional, named,
ranged, or composed values. One ordinary value-producing expression is already
a Pack, while a multi-value Pack remains fluid until a receiving contract
chooses to materialize a Type. `()` supplies an empty Layout and fits `[]`
without inventing a zero-value Type identity.

The common source shapes make that direction visible. Parentheses group values
that are being supplied, while brackets describe values that are required:

```ttx
()                         // empty Pack
(value)                    // the same one-value Pack as value
(left, right)              // positional Pack
(.x = left, .y = right)    // named Pack

[]                         // empty Layout
[Left, Right]              // positional Layout
[.x : Left, .y : Right]    // named Layout
```

Named Layout slots use `.name : Type` because they promise a descriptor, while
named Pack slots use `.name = value` because they supply value flow. Concrete
languages decide where either delimiter may be omitted without ambiguity.
Omitting a delimiter does not change the semantic Pack or Layout.

A Layout describes an ordered shape and how supplied values fit it. TTX provides
five common forms:

- `Value` is an identity-free Layout containing one atomic Type.
- `Fluid` describes and fits ordered positional entries.
- `Named` retains uniquely named slots and fits them by name. A slot can borrow
  a name independently while fitted queries still return the exact source
  identity.
- `Ranged` describes one entry repeated over a fixed interval.
- `Composite` joins complete descriptors without flattening them.

Layouts describe promised semantic shape and how supplied values fit it. Layout
decorators preserve the fitting rules of the source that owns each entry. A
leaf Type contributes its own one-entry Value Layout, while an empty Layout
describes no value and fits every other empty Layout. A Type with that shape
can retain contextual facts, but it cannot enter value flow. An Addressable
therefore requires a Type with at least one Layout entry. Concrete languages
define their own scalar families, logical and numeric refinements, and abstract
machine storage requirements.
Target object layout, field offsets, registers, address spaces, pointer forms,
and runtime storage belong to the consumer that chooses a physical
representation.

Every concrete Type that a language allows as an ordinary value has a nonempty
Layout and a default.
That language defines and creates the value. TTX does not infer it from cleared
memory or provide one universal default object. An empty View is still one View
value, while an empty Layout means that no value was produced.

### Terminal products

A Terminal product is a finished output that no longer depends on the live
Workspace. LLVM IR, SPIR-V, debug data, object modules, native programs, editor
data, formatted source, and semantic Archives are examples.

Terminal describes where live language objects end, not one shared file format.
Each compiler, linker, formatter, editor service, or Package defines the output
it produces. References to live Workspace objects never appear in that output.

Most Terminal products keep only the facts needed by their next consumer. LLVM
IR, debug data, and object files cannot rebuild the complete language model. A
semantic Archive has a different purpose. It keeps enough information for each
language to create fresh objects without acquiring and parsing the source again.

A restored Archive must present the same public names, categories,
relationships, order, Layout behavior, and language facts. It does not need to
reproduce process addresses or the old in-memory arrangement.

## How the platform uses TTX

TTX gives each part of Tetrodotoxin a useful view of the same program:

* Dialects publish exact semantic identities while retaining their own grammar
  and richer language rules
* Workspace keeps those identities and their source transactions alive through
  completion
* Package connects sources and dependencies through stable authored routes
* Puffer uses Anchors, Documentation, and common semantic categories for editor
  services
* Compilers consume Types, Packs, Layouts, Addressables, and Callables before
  choosing a physical representation
* Archive writers ask each persistent Dialect for the facts needed to build a
  fresh equivalent Monograph

No single consumer becomes the master model for the others. The common graph
stays focused on meaning that crosses a boundary, while a compiler module,
debug record, formatted source, or Archive carries the facts needed by its next
consumer.

## Why keep the boundary small?

TTX is most useful when several languages or semantic domains compose in one
Workspace, or when Packages, editors, compilers, and runtimes would otherwise
translate the same facts into private models. Exact identity can then cross
those boundaries and one completed graph can feed several products.

That flexibility asks each language to own its grammar, semantic objects,
diagnostics, and contribution to the host's completion barriers. A persistent
language also defines and validates the facts needed to reconstruct its part of
the graph. TTX supplies the shared contracts rather than guessing those richer
rules on the language's behalf.

Keeping the boundary small is what makes it useful to a family of domain
specific languages. A new Dialect can expose the pieces that cooperate with the
platform without surrendering the concepts that make its source worth having.

## Further reading

* [TTX design](ttx_design.md) explains why the graph, identity, Pack, Layout,
  and Terminal boundaries take this form.
* [TTX semantics](ttx_semantics.md) is the normative lexical and semantic
  contract.
* [Tetrodotoxin](../tetrodotoxin/README.md) shows how the complete platform
  composes concrete languages around TTX.
