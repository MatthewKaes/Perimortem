# Tetrodotoxin Design

Tetrodotoxin is the concrete TTX host. This document describes how its
components connect. It does not duplicate each component's grammar, semantic
model, target rules, or durable format.

The shared TTX contracts remain in
[`../ttx/ttx_semantics.md`](../ttx/ttx_semantics.md). Concrete source grammar
lives under [`parser`](parser/). Concrete semantic owners live under
[`model`](model/). The Package parser owns the root transaction and confined
Workspace; the Package model owns its parsed and resolved facts.

## Owner graph

```text
Parser::Package::Workspace
        |
        v
Parser Cursor transactions
        |
        v
Model Source + shared Environment
        |
        v
finalized Package semantic graph
        |                    |
        v                    v
Compiler + Linker        Archiver Writer
        |                    |
        v                    v
terminal products        durable buffer
                             |
                             v
                         Archiver Reader
                             |
                             v
                    source-free Package graph
```

No component reconciles two competing semantic models. The Parser constructs
or selects real Model owners. Compiler and Archiver consume the finalized graph
through TTX and Tetrodotoxin contracts.

## Package transaction

The production entry is the root `package.ttx` beneath one confined package
root.

`Parser::Package::Source` consumes the document's opening Documentation, exact
Package envelope, ordered Resolution requests, and ordered normalized member
routes. It constructs `Model::Package::Source`; it does not store that data on
the parser. Its complete grammar is documented in
[`parser/package`](parser/package/).

The Package transaction is:

```text
pin one Parser::Package::Workspace
-> parse root package.ttx into Model::Package::Source
-> resolve every exact external Package
-> load every explicit member source beneath the pinned root
-> construct one shared Environment
-> let each Source own its bytes, Tokenizer, Arena, and roots
-> parse and evaluate each member through its selected Dialect
-> finalize every reachable semantic owner
-> publish one Package product
```

The parsed model contains no filesystem handle, opened path, Environment,
member Source, resolved Package graph, or parser bookmark. Parser supplies the
root transaction; Model supplies both the parsed declaration model and the
later semantic owners.

Every member Source borrows the same Environment and owns no package dependency
vector. Environment binding exposes identity without importing the producing
Dialect's legality. Package owns its anonymous semantic surface and dependency
edges. External name and exact Version remain resolver and Manifest facts.

## Source transaction

`Model::Source` owns one immutable source stream:

- copied source bytes and optional diagnostic path;
- one Tokenizer over those bytes;
- one Arena for derived objects;
- every semantic root produced from the stream; and
- the relationship between each root and its concrete Dialect.

Cursor exists only while a parser consumes that Source. A Source result must be
semantically processed while its Tokens are available. A token index, body
range, or Cursor snapshot is not an acceptable stored result.

The existing body-token-index Source overload is migration residue and not a
contract to preserve. The replacement entry consumes the selected body directly
or rejects it until a real semantic owner can do so.

## Environment transaction

Environment is the shared binding owner for one graph-construction transaction.
It owns:

- exact external Package bindings and their local Aliases;
- direct host or earlier-member bindings;
- immutable `View`, `Access`, and `Fixed` Generic formulas; and
- one append-only Generic materialization writer.

Every member Source observes the same materialized Type identity for the same
formula and arguments. Formula lookup remains immutable. Package and member
readiness may advance monotonically while construction is private, but a
published key is never deleted or remapped.

The Package owner keeps Environment and every member Source alive while a
consumer can query source-backed facts. Consumers finish first, materialization
queries stop, Sources are destroyed, and Environment is destroyed last.

## Semantic finalization

Parsing alone is not the Package publication barrier.

Before Compiler, Writer, or another immutable consumer begins, each concrete
owner must:

- complete every reachable Type, Addressable, Callable, Body, and
  Dialect-specific fact;
- seal its own mutable lookup surfaces;
- validate public signatures and cross-owner edges;
- reject unsupported or incomplete facts; and
- derive Package-local definition coordinates only after the graph is stable.

A later phase walks semantic owners. It never replays source Tokens. Failure may
leave unreachable Arena allocation inside the private transaction, but it
publishes no partial Source collection, Package, or terminal product.

## Embedded resources

Resource handling crosses three owners:

```text
Library or Scene literal parser
-> Environment logical route cache
-> borrowed Tetrodotoxin Workspace capability
```

Parser recognizes the `$[...]` operand and reports a source diagnostic.
Environment normalizes logical routes and owns successful transaction
snapshots. The package Workspace owns the same-opened-object confined read.

No layer falls back to the process working directory or a Source directory.
Environment and Parser never receive an operating-system handle. Workspace
never constructs a Constant or parser diagnostic.

Only reachable byte values may become semantic or durable Package facts. The
package root, authored route, route cache, unused input bytes, and filesystem
capability remain transaction state.

## Concrete Dialects

Tetrodotoxin's top-level source contracts are documented with their Parser
owners:

| Dialect | Parser contract | Semantic responsibility |
| ------- | --------------- | ----------------------- |
| Package | [`parser/package`](parser/package/) | anonymous exports and dependency edges |
| Library | [`parser/library`](parser/library/) | reusable Types, values, Callables, and CPU Bodies |
| Render | [`parser/render`](parser/render/) | render values, resources, and required Stage contracts |
| Shader | [`parser/shader`](parser/shader/) | exact Render implementation and Stage Bodies |
| App | [`parser/app`](parser/app/) | process composition and transition policy |
| Scene | [`parser/scene`](parser/scene/) | reusable managed state and typed signals |

[`parser/foreign`](parser/foreign/) owns the embedded Foreign source contract.
Foreign is not a top-level envelope and does not become Package resolution.

The Parser documents accepted source shape. Model documents retained semantic
facts. Compiler, Runtime, Graphics, Linker, and Archiver document what they
derive or execute. No Dialect README grants a neighboring component ownership
of its facts.

## Compiler and target boundary

Compiler borrows one finalized Package graph. It derives compilation-local
target records from proven Type, Layout, Addressable, Callable, Body, and
Dialect-specific facts.

Target records may include sizes, offsets, alignments, pointer forms, storage
classes, interface coordinates, register classes, and ABI carriers. They are
not TTX identities and never become the source of semantic truth.

The compiler does not infer representation from C++ type names, formatted TTX
names, structural coincidence, or a central Kind. It proves the narrow semantic
or representation contract required by its target.

Exact lowering rules and migration boundaries live in
[`compiler/README.md`](compiler/README.md).

## Linker and durable boundary

Linker owns source-independent object, symbol, relocation, and target-format
machinery. A stale compiler caller does not make that machinery legacy.

Archiver owns durable Package buffers. Writer accepts only a finalized Package.
Reader validates its complete bounded format before publishing a source-free
Package. Neither owner searches repositories, opens source files, executes
runtime objects, or treats a process address as durable identity.

The current durable contract and its supported graph subset live in
[`archiver/README.md`](archiver/README.md). This design does not assign another
format number or claim support beyond that README.

## Runtime and Graphics direction

Managed execution and language-neutral Graphics submission are designed
Tetrodotoxin concerns, but there is no active runtime or graphics component in
the current tree.

App and Scene Parser READMEs define the source pressure. Model may retain their
semantic owners when those contracts are implemented. A future Runtime owns
worker-local storage, lifecycle execution, transitions, cleanup, and Body
execution. A future Graphics owner receives concrete render transactions and
does not learn Source, Scene, or parser concepts.

The Scene fixture under
[`../apps/ttx/scene_lifetime`](../apps/ttx/scene_lifetime/) is therefore a
design-pressure input, not runtime evidence.

## Evidence boundary

Current target existence establishes implementation presence only. The active
Parser implements Comment, Builtins, Type, and Package parsing. The active
Model contains Source, Environment, Namespace, Package, Render, Shader, Stage,
and terminal contracts. Package contains the confined Workspace. Compiler,
Linker, and Archiver targets exist.

That inventory does not prove:

- Library, Render, Shader, App, Scene, or Foreign parsing;
- full Source evaluation without retained token positions;
- Package Container orchestration;
- embedded-resource Environment caching;
- runtime or Graphics execution; or
- source-free restoration of every documented semantic contract.

Each component must report its own build and behavioral evidence without
turning a structural fixture or neighboring target into an implementation
claim.
