# TTX

TTX is a small shared vocabulary for tools and languages that need to describe
the same program. It defines common ideas such as Types, values, addresses,
callable code, and the shape of data without forcing every language into one
syntax tree or one universal type system.

Each language still decides what its source means. TTX only defines the pieces
that a Package manager, compiler, editor, or runtime may need to inspect. Those
systems can refer to the same program object directly instead of building and
synchronizing private copies.

[Tetrodotoxin](../tetrodotoxin/README.md) is the reference host in this
repository. It uses TTX to keep the objects created by several languages in one
Workspace. Tetrodotoxin provides Packages, compilers, and runtime policy, while
TTX remains independent from those choices.

## Where TTX sits

TTX sits between reading source and producing a file for a particular tool or
machine:

```text
authored source
-> Tokens with exact source locations
-> language objects created from those Tokens
-> shared questions about the completed program
-> LLVM IR, SPIR-V, editor data, Archives, or another final product
```

The live graph keeps facts that still matter before a target, runtime, or output
format has been chosen. A tool can ask which Type an Alias represents, which
Type an Addressable reaches, which values a Pack supplies, which Layout a
Callable accepts, or whether one Layout fits another. TTX does not decide how
those facts become registers, offsets, editor messages, or stored bytes.

TTX therefore appears earlier than LLVM IR in a compiler. LLVM IR describes
computation after the language has chosen a form suitable for optimization and
machine-code generation. TTX keeps language-level meaning available while
several languages and tools are still putting the program together. A project
can use TTX for that shared meaning and LLVM IR for CPU compilation.

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
product. TTX gains a smaller cross-language boundary by leaving those rich
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

* [TTX design](ttx_design.md) explains why the graph, identity, Pack, Layout,
  and Terminal boundaries take this form.
* [TTX semantics](ttx_semantics.md) is the normative lexical and semantic
  contract.
* [Tetrodotoxin](../tetrodotoxin/README.md) shows how the reference host
  composes concrete languages around TTX.
