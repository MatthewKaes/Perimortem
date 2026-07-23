# TTX

TTX is the repository's human-authored source IR, lexical bytecode, and shared
semantic model. It deliberately stops before package filesystems, execution
runtimes, and target encoders. Tetrodotoxin supplies those owners.

The active core is built by `//ttx:lexical`, `//ttx:concept`, `//ttx:model`,
and the convenience target `//ttx:ttx`.

## Pipeline

```text
owned source bytes
-> Source-owned Tokenizer
-> Dialect-selected evaluation
-> one graph of real Abstract identities
   + identity-free Layouts
   + identity-free executable Bodies
   + Dialect-owned versioned facts
-> runtime plan or target Representation
-> terminal product
```

The lexer assigns a concrete `Lexical::Code` to every emitted token. A Dialect
then owns how many tokens form one instruction, which builtins are accepted,
how those instructions are evaluated, and which extra facts are legal. A
Dialect may reject or narrow a common construct; it may not reinterpret an
existing common contract. Merely binding an identity into an Environment does
not import the producing Dialect's builtins or legality rules.

Source text and Cursor state are construction inputs. Evaluated semantic facts
remain on their real owners. Executable source is consumed into one common
`Model::Body`; target-specific records are derived only after the semantic
graph is complete.

## Model map

`Ttx::Concept` owns the contracts that every host shares:

- `Abstract` owns semantic identity, documentation, resolution, and local
  contract proof.
- `Invalid` is semantic failure. Semantic queries return a real reference or
  `Invalid`, never a nullable semantic pointer.
- `Reference<T>` is a non-null graph edge.
- `Layout` is an identity-free ordered shape and directional fitting contract.
  It owns no offsets, address spaces, pointer widths, target alignment, ABI
  carriers, register classes, or collector policy.
- `Documentation` is an identity-free prose view.

`Ttx::Model` owns common semantic identities and values:

- `Type` owns value identity and supplies its semantic `Layout`.
- `Types::Managed` proves that values of a Type are managed object references.
  It says nothing about tracing, allocation, movement, or target storage.
- terminal Type families prove numeric or flag meaning and width.
- `Types::Generic` is an immutable compile-time formula. Its ordered signature
  selects `const Type&`, `Unsigned_64`, `Signed_64`, or `Bool` arguments. The
  active graph transaction owns append-only materializations, so a valid key
  returns one stable real Type identity. The Generic formula is not a Type
  itself.
- `Types::Generics::Fixed` materializes `Fixed[T, N]` as a homogeneous
  `Ranged` Layout. Its signed extent must be non-negative.
- `Expression` is a value fact distinct from its result Type. `Constant` is an
  immutable, zero-input Expression.
- `Addressable` is a named typed storage location. `Writable` is its narrower
  assignment capability; neither is a runtime pointer.
- `Callable` owns complete parameter and result Layouts. A `Self` Callable
  contains its receiver exactly once at parameter zero.
- `Exports` is the only public discovery surface. Private roots and Environment
  bindings do not leak through it.
- `Alias` retains its authored local identity while resolving to its target.

The semantic graph has no universal Kind, class database, mutable Dialect
registry, copied member/function records, shadow Type graph, or allocated path
history.

## Layout and fitting

The active identity-free Layout implementations are:

- `Fluid`: positional value flow;
- `Named`: named value flow;
- `Structured`: ordered real Addressable members;
- `Ranged`: one real element Type repeated a fixed number of times;
- `Composite`: concatenated Layouts.

Fitting is directional. A source Layout proves whether it can supply a target
Layout and which real Abstract supplies each target slot. Structural
coincidence never creates semantic identity. In particular, four scalar fields
do not make a vector unless the Type proves the vector contract or supplies an
explicit representation edge in the owning Dialect.

Target Representation is a consumer-side product. A SPIR-V planner may derive
physical scalar/vector/aggregate, pointer, storage-class, location, binding,
and offset records for one compilation. Those records are neither Types nor
Layouts and are not serialized as semantic truth.

## Executable Body

`Model::Body` is an immutable identity-free value retained by the concrete
Callable, Shader Stage, or App lifecycle owner that produced it. Compact local
IDs index:

- parameters and produced values;
- blocks and their operation ranges;
- operands and return ranges;
- constants and aggregate construction;
- projections and indexing;
- calls to real Callable identities;
- typed loads and stores through real Addressables;
- Dialect-coded unary and binary values, branches, jumps, and returns.

Every graph reference is a non-null edge to the real Type, Constant, Callable,
or Addressable. A common binary value stores a bytecode and exactly two local
operand IDs; its Dialect owns the bytecode meaning and proves the operand and
result Types recorded in the Body value table. Domain operations remain calls
to domain owners: texture
sampling is a call to the Image sampling Callable, not a universal shader
opcode. `Bodies::Builder` is the only mutable construction phase and publishes
nothing until IDs, ranges, control flow, result Types, and terminators validate.

The host interpreter and SPIR-V target consume this same Body. There is no
separate host statement tree or shader AST. Consumer profiles may support only
a validated subset. The current host executor is limited to App control flow,
while the Shader target consumes the arithmetic, aggregate, projection, load,
index, call, and conversion operations required by Default2D.

## Scene and App composition

Scene is the reusable managed state boundary beneath App. A Scene owns
`enter`, `frame`, and `exit` Callable edges, render roots, and typed signals.
It emits an outcome but never names the next Scene. App owns the initial Scene
and maps `(Scene, signal)` pairs to replace, push, pop, or process exit policy.
This makes Splash-to-Title-to-Splash a runtime state-machine cycle without a
cyclic Source dependency or forward proxy.

The complete proposed source is maintained in
[`../apps/canonical/scene_demo`](../apps/canonical/scene_demo/). Its Package
descriptor is parsed through the production descriptor path, but Scene
evaluation and execution are not implemented yet. The fixture is not counted
as semantic acceptance until its owners, transitions, runtime transaction, and
archive records exist.

## Source envelopes and packages

The reference Tetrodotoxin envelope begins with a named Dialect:

```ttx
dialect : Library;
```

Package membership and exact external resolution belong to the package
container, not Source:

```ttx
dialect : Package;

resolve Graphics : Perimortem.Graphics = "1.0";
source Main : App = "main.ttx";

public Demo : alias = Main::Demo;
```

The quoted `Major.Minor` is parsed as two independent unsigned components,
never through floating point. Every member Source owns its bytes, Tokenizer,
Arena, and evaluated roots while borrowing one package-owned Environment.
Source has no imports or dependency vector. The active Container publishes
completed members in descriptor order, so declaration synchronization for
arbitrary mutually referring members is not yet implemented. Public package
names come only from the final Package `Exports` surface. Manifest and
repository own the external name and exact Version.

See [ttx_semantics.md](ttx_semantics.md) for normative contracts and
[ttx_design.md](ttx_design.md) for author-facing syntax and the complete
Render2D/Default2D/Demo walkthrough plus the canonical Scene composition.

## Repository map

- [`lexical`](lexical/) owns Codes, tokens, Tokenizer, Cursor, Lexicon, and
  lexical diagnostics.
- [`concept`](concept/) owns Abstract, Documentation, Invalid, Layout, and
  non-null Reference.
- [`model`](model/) owns Type families, Layout implementations, Generics,
  Expression/Constant, Addressable/Writable, Callable, Alias, Exports, and
  common Body.
- [`../tetrodotoxin/model`](../tetrodotoxin/model/) owns Source, Environment,
  Package, durable Dialect facts, Render/Shader/App owners, and terminals.
- [`../tetrodotoxin/interpreter`](../tetrodotoxin/interpreter/) owns
  token-consuming Package, Library, Render, Shader, and App evaluation. Scene
  evaluation is the next owner-shaped Dialect rather than a retained old ISA.
- [`../tetrodotoxin/puffer`](../tetrodotoxin/puffer/) owns descriptors,
  path-confined member loading, exact package resolution, package workspaces,
  repositories, and terminal materialization.
- [`../tetrodotoxin/target/spir_v`](../tetrodotoxin/target/spir_v/) owns
  compilation-local Representation and SPIR-V lowering from real contracts.
- [`../tetrodotoxin/runtime`](../tetrodotoxin/runtime/) owns worker-local Realm
  and App execution; [`../tetrodotoxin/graphics`](../tetrodotoxin/graphics/)
  owns the language-neutral submission boundary.
- [`../tetrodotoxin/archiver`](../tetrodotoxin/archiver/) owns the version 1
  durable package buffer.

The superseded `tetrodotoxin/isa` and private compiler execution trees have
been removed after their useful algorithms were migrated. The remaining
whole-compiler and CLI sources are outside active targets and are not current
semantic authority.
