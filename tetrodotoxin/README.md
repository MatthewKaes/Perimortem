# Tetrodotoxin

Tetrodotoxin is the active host for TTX packages, standard Dialects, target
products, durable package buffers, and execution. TTX owns the shared language
contracts; Tetrodotoxin owns the concrete transaction that turns package files
into one executable and archiveable semantic graph.

## Implemented vertical

The production path is:

```text
package directory
-> parse package.ttx Descriptor
-> resolve exact external Manifests and Packages
-> confine and load every explicit member Source
-> tokenize every Source through its owned Tokenizer
-> publish completed external bindings into one package Environment
-> evaluate Library, Render, Shader, and App members in descriptor order
-> publish each completed member into that shared Environment
-> evaluate Package exports
-> compile selected Shader stages to SPIR-V terminals
-> publish one Packages::Sources
```

The canonical acceptance closure is:

```text
Perimortem.Math 1.0
Perimortem.Runtime 1.0
Perimortem.Graphics 1.0
Demo 1.0
```

Math supplies real geometry Types. Runtime supplies managed Window and inline
Frame/Time facts plus semantic Input and Key contracts. Graphics supplies real
Library Types, `Render2D`, and the `Default2D` Shader. Demo supplies one managed
App with direct lifecycle roles, one Render root, and an explicit
`Render2D -> Default2D` binding.

No Source imports another Source or stores package dependencies. Package
membership is container data. Every member Source borrows the same completed
external Environment. The current Container then publishes each completed
member before evaluating the next descriptor member. Package is published only
after member evaluation, export evaluation, Shader validation, and terminal
compilation succeed.

## Owner boundaries

- [`model`](model/) owns Source lifetime, Environment, Namespace, anonymous
  Package, canonical package-local definition IDs, concrete Library facts,
  Render/Shader/App semantic owners, direct dependencies, terminals, and
  Shader-to-terminal products.
- [`interpreter`](interpreter/) consumes TTX token bytecode. Its fixed compile-
  time Definition composition and concrete Package, Library, Render, Shader,
  and App evaluators attach facts to the real owners.
- [`puffer/package`](puffer/package/) owns Descriptor parsing, the package
  Container transaction, the source-backed Workspace, and terminal
  Materializer. [`puffer/resolution/package`](puffer/resolution/package/) owns
  exact Catalog/Repository lookup and source-free recursive restore.
- [`target/spir_v`](target/spir_v/) owns compilation-local target
  Representation, Shader legality, SPIR-V planning, internal validation, and
  stable terminal metadata.
- [`runtime`](runtime/) owns worker-local Realm, common Body host execution,
  App loop, rooting, cleanup, and Package-closure Shader product lookup.
- [`graphics`](graphics/) owns language-neutral transactions, the Sink
  contract, and the headless acceptance backend. It includes no TTX frontend
  contracts.
- [`archiver`](archiver/) owns versioned durable buffers. It has no source,
  filesystem, repository-search, or runtime-object authority.
- [`linker`](linker/) remains the active source-independent ELF/archive
  packager.

## Active Bazel targets

The current owner-shaped targets are:

```text
//tetrodotoxin:model
//tetrodotoxin:interpreter
//tetrodotoxin:spir_v_assembler
//tetrodotoxin:register_allocator
//tetrodotoxin:x86_64_assembler
//tetrodotoxin:spir_v_target
//tetrodotoxin:runtime
//tetrodotoxin:graphics_runtime
//tetrodotoxin:linker
//tetrodotoxin/archiver:archiver
//tetrodotoxin/puffer:package_descriptor
//tetrodotoxin/puffer:package_container
//tetrodotoxin/puffer:package_workspace
//tetrodotoxin/puffer:package_repository
//tetrodotoxin/puffer:package_materializer
```

The reusable SPIR-V word emitter is isolated in `spir_v_assembler`; activating
it does not activate the old compiler execution tree or ISA registry.

There is no active Puffer CLI, LSP target, or general native compiler target in
this slice. The superseded ISA and private compiler execution trees were
removed after the common Body and owner-shaped target path replaced them; the
preserved assembler and allocation utilities consume selected instructions or
the common Body rather than a second semantic system. The vertical does not
self-host all standard packages, implement a window/GPU backend, or expose
Foreign raw carriers.

The canonical multi-Scene application under
[`../apps/canonical/scene_demo`](../apps/canonical/scene_demo/) is retained as
the next acceptance target. Its production Package descriptor parses, but no
Scene evaluator, transition runtime, or durable Scene schema is claimed. The
fixture deliberately exercises Splash and Title state, semantic input, typed
signals, a transition loop, resource loading, and multiple coordinated Source
members so those requirements cannot be hidden by the minimal Demo.

## Shared graph and Dialects

The durable model is one graph of real TTX Abstract identities plus
identity-free Layout and Body values. Package owns only export composition.
Library owns reusable inline/managed Types, Generic materializations,
Callables, constants, and host Bodies. Render owns value state, constant/push/
resource roles, and required stage Callable contracts. Shader owns exact
Render implementation edges, Stage Bodies, GPU interface facts, explicit
representation edges, and terminal production. App owns managed state,
lifecycle Callable edges, render roots, and explicit Render-to-Shader binding.
Scene is specified as a managed state owner with lifecycle edges, render roots,
and typed signals. App, not Scene, owns initial-state and transition policy so
cyclic navigation does not create cyclic Source dependencies.

A Dialect controls source presentation, accepted builtins, evaluation,
legality, and additional versioned facts. It may reject or narrow a common TTX
construct, but it may not reinterpret a common contract. Environment binding
exposes identity only; it does not merge Dialect builtins or rules.

## Default2D target path

`Render2D` retains the exact Addressables and Callables for its ordinary state,
constants, push constants, Image resource, and Vertex/Fragment requirements.
`Default2D` retains that real Render identity, one implementation of every
required Stage, complete input/result Layouts, one common Body per Stage, and
the location/builtin/binding facts needed by the planner.

Before emission, Shader construction rejects missing or duplicate required
stages, unknown stages, directional Layout mismatches, undeclared Render
access, managed GPU values, unsupported representation, and conflicting
interface facts. `Color` supplies an explicit representation edge to the real
`Vec4D` Type; structural coincidence is not used.

SPIR-V lowering derives dense target records from terminal Type facts, concrete
vector/aggregate contracts, Addressables, stage interfaces, and Body
operations. It emits vertex and fragment modules plus versioned interface
sidecars at stable logical paths:

```text
shader/Default2D/vertex.spv
shader/Default2D/vertex.spv.interface
shader/Default2D/pixel.spv
shader/Default2D/pixel.spv.interface
```

The internal validator checks module bounds/order, IDs, references,
entry/function structure, execution models, interface variables, locations,
builtins, push offsets, descriptor bindings, and promised metadata.

## Runtime path

Realm is worker-local, nonmoving, and stable-address for live managed objects.
It derives trace slots from real managed Type fields and range element Types,
marks from explicit App/frame roots, reclaims unreachable cycles, rejects
cross-worker mutation, and tears down all storage deterministically.

Host executes lifecycle Bodies through direct App edges:

```text
create Realm
-> allocate and root App state
-> invoke start once
-> provide Runtime Frame and invoke frame
-> submit explicit render roots through explicit Shader bindings
-> consume Continue, Exit, or Failure runtime value
-> invoke stop once on normal and defined failure paths
-> release Graphics
-> unroot, collect, and tear down Realm
```

The headless Sink observes the neutral transaction and proves the configured
`Render2D` root selects `Default2D`; it requires no window server or GPU. The
current transaction carries root/binding coordinates and terminal products,
not an evaluated `Render2D` field payload.

The host legality profile currently executes Flag constants, branches, jumps,
and returns. Common aggregate, projection, call, load, store, conversion, and
arithmetic Body operations are durable and used by Shader lowering, but they
are not yet realized by the host executor. App state allocation and lifecycle
ordering are therefore real while general Library and App execution remain the
next runtime layer.

## Durable execution

Archive format 1 stores real Type/Layout/Generic/Addressable/Callable/Constant/Body,
Render/Shader/App, dependency, product, and terminal facts. Local edges use
Package definition IDs; external edges use dependency ordinal plus the
dependency's definition ID. Render, Shader, and App extension records use
stable contract UUIDs and explicit Major.Minor schema versions.

Repository owns archive bytes and recursively restores exact dependencies.
Reader validates all bounded sections, references, graph legality, Bodies,
extensions, dependencies, products, and terminals before publishing one
source-free `Packages::Precompiled`. Restored Demo executes the same lifecycle
and submits independently owned, byte-identical Default2D modules without
consulting Source, Tokenizer, or filesystem paths.

See [tetrodotoxin_design.md](tetrodotoxin_design.md) for the full transaction,
[archiver/README.md](archiver/README.md) for durable formats, and
[puffer/README.md](puffer/README.md) for package orchestration.

## Implementation conventions

Comments explain ownership, invariants, and the reason an algorithm has its
shape. A long function names each material stage in full prose before the code
that performs it. Code comments use complete sentences rather than dash or
semicolon shorthand. They do not restate individual statements.

Function bodies are arranged as readable paragraphs. A paragraph contains its
declarations first, then its statements, then at most one control-flow block.
When another paragraph follows a control-flow block, a blank line separates
the two. This spacing is reviewed manually because the formatter cannot infer
the intended algorithmic paragraphs.

C++ files are formatted only through `.vscode/format.sh` with explicit paths.
The complete unit suite is invoked through
`bazel run //validation:unit_tests`, which supplies the required runfiles and
generated products.
