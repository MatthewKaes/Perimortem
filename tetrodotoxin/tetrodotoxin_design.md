# Tetrodotoxin Design

Tetrodotoxin is the concrete host for TTX. This document connects its owner
boundaries without copying the shared TTX graph into one model per source
Dialect.

TTX owns lexical bytecode and the target independent semantic contracts
specified by `ttx/ttx_semantics.md`. Tetrodotoxin owns Source lifetime,
Environment construction, concrete grammar, filesystem policy, generators,
runtime policy, and durable products.

## Owner graph

The foundational dependencies are:

```text
Language ----\
Environment ---> TTX -> Perimortem
Library -----/

Package -> Language
```

Language knows no concrete Dialect. It owns source lifetime and universal
envelope parsing only.

Environment owns the semantic Workspace and Namespace used while completed
Sources are connected. Its eventual Graph will finalize those roots, resolved
Packages, and selected product roots.

Package owns one authored Package root, confined package reads, and durable
Package products. It supplies inputs to Environment without owning the semantic
Graph.

Library owns Library grammar and the reusable CPU compilation path. It consumes
shared TTX identities and completed Environment facts rather than owning a
second Type model.

The first concrete Library Source parser adds `Library -> Language`. A direct
Environment dependency is added when Library construction or compilation
consumes that owner.

The intended product direction is:

```text
Package::Workspace confined bytes
        |
        v
Language::Source owners
        |
        v
concrete Package, Library, App, Scene, Render, and Shader roots
        |
        v
Environment Namespace and future Graph finalization
        |
        v
App planning + Library compiler + Linker + Shader + other generators
        |
        v
opaque graph and terminal entries + Package::Manifest
        |
        v
Package::Distribution -> Package::Writer -> format 1 archive
                                              |
                                              v
                                  Package::Reader
                                              |
                                              v
                               validated Distribution
                                              |
                                              v
                           future source free graph restoration
```

No component reconciles competing semantic models. A concrete parser
constructs its real root. Environment connects those roots through real TTX
identities. Generators consume the finalized result. Package validates and
transports Manifest and Entry values without learning the representation
inside opaque entries.

The finalized graph, graph codec, Distribution construction, archive codec,
and functional Reader and Writer are not implemented.

## Universal Source transaction

`Language::Source` is one immutable source lifetime owner. It is not a TTX
Abstract and does not imitate semantic resolution.

Source owns, in destruction safe order:

1. one Arena;
2. one copy of the diagnostic path;
3. one copy of the authored text;
4. one Tokenizer over that owned text;
5. one concrete Abstract root produced in its Arena.

Construction is one static transaction:

```text
Language::Source::parse(
  text,
  path,
  borrowed map<exact Dialect name, static parse function>,
  errors)
```

The transaction consumes:

```text
zero or more opening comment lines
dialect : Type;
concrete Dialect body
end of document
```

It greedily creates opening Documentation, looks up the exact Dialect name,
calls the selected static parser with the same forward Cursor, and accepts the
result only when:

1. the parser returns one real Abstract root;
2. the parser consumes the complete body;
3. the transaction adds no diagnostic.

Source copies caller text before tokenization. The Cursor, parser map, token
position, and diagnostic borrow do not survive construction. The completed
Source keeps the Tokenizer because Tokens and source projections borrow its
owned stream.

There is no Dialect base class, Frontend object, parser inheritance, separate
Container, second tokenization pass, callback registry, generic definition
index, or parser bookmark. The compiled toolchain constructs the parser map and
keeps it alive only for the call.

The current static parser signature supplies the Cursor and opening
Documentation. No generic context object is approved. A future concrete parser
that proves it needs another capability must first place that capability on its
real owner without moving Library or Package policy into Language.

## Concrete Source roots

Each selected parser constructs one concrete Abstract root in the Source Arena.
That root owns its Dialect semantics and resolution behavior.

Package currently provides:

```text
Package::Language::Dependency
Package::Language::Source
Package::Language::Parser::parse
```

The Package root retains opening Documentation, exact dependency requests, and
normalized member routes. It retains no filesystem handle, parser state,
opened member Source, resolved graph, or archive record.

Future Library, App, Scene, Render, and Shader roots follow the same lifetime
rule without inheriting from a common Source concept. Their shared identity
surface is already `Ttx::Concept::Abstract`.

## Library

Library grammar constructs real TTX Type, Layout, Generic, Addressable, and
Callable identities required by CPU source. `Library::Language` owns
Expression, Binding, Projection, Constant, and typed Constant domains because
Library defines their legality and value rules. Those contracts retain real
TTX edges instead of copying the Type or Layout graph.

The current Type parser consumes a real TTX resolution context and the
Environment owned Generic materialization transaction. A complete Library
Source root will retain its declarations and Bodies while publishing completed
edges through Environment Namespace.

Library compilation may lower completed CPU facts retained by a Library, App,
or Scene owner without converting that owner into a Library Source, copied
Namespace, or shadow Type graph.

A complete Library Source root, declaration discovery transaction, Body model,
and full Library parser remain unimplemented.

## Package transaction

The production authored entry is `package.ttx` beneath one confined package
root. `main.ttx` is a filename convention only. A completed Package selects
the sole App root.

The implemented authored transaction is:

```text
Language::Source::parse
-> Package::Language::Parser::parse
-> Package::Language::Source
```

Dependency is a request for later exact package resolution, not the resolution
itself. Member routes are normalized package relative values.

The future assembly transaction is:

```text
pin one Package::Workspace
-> read package.ttx through the pinned root
-> parse one Language::Source with the Package parser map
-> require a Package::Language::Source root
-> resolve every exact Dependency
-> read each declared member beneath its package root
-> parse each member through the compiled parser map
-> finalize every reachable concrete owner
-> select the sole completed App root
-> publish one Environment graph
```

Package Workspace owns filesystem confinement. Language Source owns text,
tokens, allocation, and root lifetime. Environment coordinates completed
roots. No separate source Container is inserted between those owners.

## Finalization

Mutable construction is private to each concrete semantic owner. Before
compilation, graph encoding, or another immutable consumer begins, every owner
must:

1. complete each reachable required fact;
2. seal every mutable lookup surface;
3. validate public signatures and cross owner edges;
4. reject Invalid from committed graph edges;
5. derive durable coordinates only after semantic identity is stable.

Failure may leave unreachable Arena allocation inside a private transaction,
but it publishes no partial Source, graph, Distribution, or terminal product.
Finalization walks semantic owners and never replays source Tokens.

Source success commits only its complete local root. Cross Source binding and
Package resolution occur after both participating roots exist, so a failed
parse never requires rollback of another owner.

## Embedded resources

Every authored Source and embedded resource lives beneath one Package root.
Another location is reachable only through an exact Package Dependency.

The accepted spelling is Package root relative:

```ttx
const file_header : Fixed[Unsigned_8, 64] =
  $[resources/table.bin]:[0, 64];
```

The eventual implementation must preserve these boundaries:

1. the concrete Dialect recognizes the embedded operand and owns its source
   diagnostic;
2. Package Workspace performs the confined read against the same opened
   package root;
3. successful logical routes are deduplicated for the graph construction
   transaction;
4. empty files are valid byte values;
5. read failure is an error and never becomes empty bytes;
6. constant folding may retain only a reachable slice in terminal artifacts;
7. no layer falls back to the process working directory or the containing
   Source directory.

The static parser interface currently has no resource capability input.
Resource loading therefore remains unimplemented rather than being hidden
behind a global, filesystem access in Language, or a generic context wrapper.

## Concrete Dialects

Tetrodotoxin source contracts live with their concrete owners:

1. Package owns Dependency requests and member routes.
2. Library owns reusable CPU Types, values, Callables, and compilation.
3. Render owns render values, resources, and required Stage contracts.
4. Shader owns exact Render implementation and Stage Bodies.
5. App owns startup profile, platform entry, lifecycle, and transition policy.
6. Scene owns reusable managed state, typed signals, and lifecycle roles.

Foreign is embedded source syntax for CPU like Dialects. It is not a top level
envelope and does not become a Package Dependency. `const`, `state`, and
`func` distinguish imported read only symbols, addressable storage, and
callables.

Source order does not determine binding. A concrete Dialect may discover names
before it completes definitions, initializers, and Bodies. Mutable objects keep
stable identity during that transaction. Cursor positions and parser replay
never stand in for unresolved semantic facts.

## Compiler, Linker, and Package products

Library owns the reusable CPU compilation path. It derives target records from
completed TTX semantic and Library representation contracts. Target records may
contain sizes, offsets, alignments, pointer forms, storage classes, interface
coordinates, register classes, and ABI carriers. They are not semantic graph
identities.

Shader owns SPIR V representation and module emission. Shader generation and
SPIR V compilation remain outside the current Library parser effort.

Linker owns source independent objects, symbols, relocations, target formats,
and System V archive construction. A stale caller does not make that machinery
legacy.

Package Distribution owns Manifest and Entry values. Writer encodes a
Distribution. Reader validates a complete bounded Package format before
publishing a Distribution. Package Reader and Writer never search source
repositories, execute runtime objects, or retain a live semantic graph.

The prototype has one mutable format numbered `1`. It requires no backwards
compatibility before a released boundary.

## App and Scene execution

App owns one startup profile and one lifecycle policy. `Windowed`, `Terminal`,
and `Headless` are App owned profiles rather than Runtime Package exports.
Each selects support libraries, generated platform entry facts, and package
configuration.

Managed and Unmanaged lifecycle policies retain direct
`Ttx::Model::Callables::Static` edges. The declaration name is irrelevant.

Scene owns prepare, pause, resume, update, and release roles. App owns replace,
push, pop, and exit transitions:

```text
push     pause current, prepare pushed
pop      release current, resume prior
replace  release current, prepare replacement
exit     release current without resume
```

The Scene file is its Scene object. A Scene role such as
`Scene prepare[self]` is a Self Callable assigned to that role.

There is no Runtime Package. Linked support libraries, generated platform entry
semantics, package configuration, and lifecycle code form process runtime
behavior. Input is eventually queried through the graph resolved
`Perimortem.System` edge. Scalar delta time is the only planned explicit
nonreceiver update parameter.

The canonical pressure fixture is
[`../apps/ttx/scene_lifetime`](../apps/ttx/scene_lifetime/). A smaller Terminal
App may establish generated entry, Library lowering, and Linker behavior first,
but it does not replace the Scene lifecycle goal.

## Evidence boundary

Current code establishes the shared Comment parser, owning Source transaction,
static Package parser, Package Source values, confined Package Workspace,
shared TTX semantic contracts, Environment Namespace and Workspace, CPU and
Shader instruction emitters, and source independent Linker machinery.

That inventory does not prove:

1. complete Library, Render, Shader, App, Scene, or Foreign parsing;
2. a finalized Environment graph;
3. Package assembly, Distribution, or archive orchestration;
4. embedded resource loading, caching, or folding;
5. CPU or Shader lowering from semantic facts;
6. runtime or Graphics execution;
7. source free restoration.

Each component reports its own build and behavioral evidence. A fixture,
tokenization result, README, target build, or test created beside an
implementation is not an independent semantic oracle.
