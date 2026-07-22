# Tetrodotoxin Design

Tetrodotoxin is the concrete TTX host. It owns named Dialects, Source and
Package lifetimes, exact package resolution, the SPIR-V target path, managed
runtime execution, the language-neutral Graphics boundary, and durable package
buffers. TTX remains the authority for shared semantic identity, Layout,
Generic, Callable, Addressable, Expression, and Body contracts.

The implemented vertical is deliberately one graph:

```text
real Abstract semantic identities
+ identity-free semantic Layouts
+ identity-free executable Bodies
+ explicit versioned Dialect-owned facts
```

Target Representation, runtime cells, and terminal bytes are derived consumer
products. They never become a shadow Type graph or alternate semantic
authority.

## Dialect contract

A Dialect controls source presentation, accepted builtins, evaluation,
legality, and additional facts. Its durable result participates in the shared
semantic graph. A Dialect may reject or narrow a common construct; it may not
reinterpret an existing common contract.

Environment binding only makes an identity available. It does not import the
producer's builtins or legality. Grammar with fixed continuations uses compile-
time `Interpreter::Definition<Dialect, Modifiers...>` composition. There is no
mutable evaluator registry, ClassDB, universal Scope, or runtime Definition
record.

The Dialect responsibilities and implementation status are:

| Dialect | Authority | Status |
| --- | --- | --- |
| Package | Environment-independent export composition only | Implemented |
| Library | inline and managed Types, Generic materializations, fields, constants, Callables, and host Bodies | Implemented for the vertical |
| Render | render values, constants, push constants, resources, and required Stage Callable contracts | Implemented for Render2D |
| Shader | exact Render implementation, GPU legality/interface facts, Stage Bodies, explicit representation edges, and Shader terminal production | Implemented for Default2D and Copy |
| App | managed state, lifecycle role edges, render roots, explicit Render-to-Shader selection, and CPU Bodies | Implemented for the minimal Demo |
| Scene | reusable managed state, lifecycle role edges, render roots, and typed outcomes | Specified by the canonical fixture, not implemented |

Foreign raw carriers and ABI linkage are outside this vertical. Managed Library
objects are not silently accepted by Shader.

## Package transaction

The production entry is a package descriptor path, not a hand-created token
stream or semantic fixture.

`Puffer::Package::Descriptor` parses the Source-owned Tokenizer for
`package.ttx`. Exact external requests and member declarations have these
forms:

```ttx
resolve Math : Perimortem.Math = "1.0";
source Types : Library = "library/types.ttx";
```

The version is two independent unsigned components. Member paths are normalized
relative to the descriptor and rejected when they escape the package root.

`Puffer::Package::Container` performs this transaction:

```text
parse descriptor
-> resolve every exact external Manifest/Package
-> locate and load every explicit member Source
-> let every Source own its bytes, Tokenizer, Arena, and roots
-> verify every member Dialect envelope
-> complete one Environment for external and local bindings
-> evaluate members in descriptor order
-> bind each completed real product for later members
-> evaluate the Package export body
-> compile reachable Shader programs into terminal products
-> publish one Packages::Sources
```

No member is evaluated before all source streams and external bindings exist.
Every Source borrows the same Environment and owns no imports or dependency
vector. `Packages::Sources` retains the explicit members, derives distinct
direct dependency Packages from the Environment, owns terminal products, and
assigns canonical definition IDs to the complete reachable graph.

The canonical standard graph is ordered so each member uses only exact
externals or earlier completed member products. That closes this vertical
without a forward-declaration proxy or incomplete Layout. It does not prove
the transaction sufficient for arbitrary cross-member or mutually recursive
declarations. Those require a package-owned reservation and synchronization
phase that installs the eventual real owners before their bodies evaluate.

Package owns only its anonymous semantic surface and dependency edges. Manifest
and resolver own external name and exact Version. Public discovery is the final
Package `Exports` surface; private Source roots and Environment bindings do not
leak through it.

## Library and common Body

The shared Environment owns the common `View`, `Access`, and `Fixed` formulas
and caches their materialized Types by exact parameter identity. Every member
Source therefore observes the same materialized address during one
interpretation transaction.

Library constructs the real Types used by the vertical:

- Math supplies `Point2D`, `Size2D`, and their scalar fields;
- Runtime supplies inline `Time` and `Frame`, managed `Window`, and real
  Callables;
- Graphics supplies explicit represented `Color`, managed `Image`, and the
  Image sampling Callable;
- fixed-range Types are materialized through their real Generic owners.

`object` produces a Type proving `Ttx::Model::Types::Managed`. That proof means
the value is a managed object reference. Layout still owns only semantic field
order and directional fitting; it carries no allocation, tracing, pointer,
target, or ABI policy.

Executable source is consumed once into `Ttx::Model::Body`. The Body contains
compact block/value/operand/operation tables and non-null edges to the real
Constant, Callable, or Addressable owners. Types live once on Body values. It
is not an Abstract, AST, scope graph, or replayable Cursor. Concrete Library
Callables, Shader Stages, and App lifecycle Callables retain it.

The common binary alternative stores only a bytecode, left value, right value,
and result value. `Tetrodotoxin::Model::Operations::Binary` owns the current
Add, Subtract, Multiply, and Divide bytecodes. Library, Shader, and future
Dialects independently decide which ordered operand Types are legal and which
result Type they produce. No Type pair becomes an Abstract or a cached
Callable. `Image::sample` remains a call to the real Image receiver Callable.
The host executor and SPIR-V lowerer consume the same Body tables under
different legality profiles.

## Render and Shader handshake

`Model::Render` is a real Type/value contract. The canonical `Render2D` owns:

- ordinary render Addressables for position, size, tone, and Image;
- two fixed-vector constants for quad positions and texture coordinates;
- push Addressables for position, size, and tone;
- an Image resource with descriptor set 0 and binding 0;
- required Vertex and Fragment Stage Callables with complete input/result
  Layouts and explicit builtin/location facts;
- explicit read sets for constant, push, and resource access.

`Model::Shader` is a real semantic object. `Default2D` retains the exact
resolved `Render2D` identity and one `Stages::Implemented` owner for every
required Stage. Each Stage retains its required Callable, complete interface,
common Body, and GPU facts. It does not copy ShaderParameter, Member, Function,
or Type records.

Shader construction is the failure-atomic contract gate. It proves:

- the selected Render is the real resolved identity;
- every required Stage is implemented exactly once and no unknown Stage is
  accepted;
- implementation parameter/result Layouts fit the required Layouts
  directionally;
- every constant, push, and resource access belongs to Render's declared set;
- interface locations, builtins, bindings, sets, and address spaces do not
  conflict;
- every Body operation is legal for Shader;
- managed Types do not enter GPU values;
- every used Type has a supported explicit Shader representation.

Structural coincidence is not representation. Graphics `Color` explicitly
points at the real `Vec4D` Type. `Point2D` and `Size2D` retain their explicit
representations, and fixed vectors prove the real Vector contract.

## SPIR-V target Representation

`//tetrodotoxin:spir_v_assembler` isolates the proven little-endian word
emitter from the old compiler. `//tetrodotoxin:spir_v_target` owns the planner,
lowerer, Package terminal traversal, interface sidecars, and internal validator.
The old ISA registry and name-driven compiler are not active dependencies.

For one Stage compilation the planner derives dense physical records from:

- terminal integer, real, and flag facts;
- concrete vector and aggregate Type contracts;
- explicit Shader representation edges;
- Stage Callable Layouts and common Body values;
- Addressable interface roles, locations, builtins, push roles, resource sets,
  bindings, and address spaces;
- the explicit Image sampling Callable contract.

These records are target Representation, not semantic identity, and they are
discarded after emission. Lowering never switches on a C++ type name, formatted
TTX name, authored ABI integer, or central Kind.

Default2D emits a Vertex module and Fragment module. Vertex lowering covers
constant arrays, dynamic indexing, unsigned-to-real conversion, push loads,
vector arithmetic, aggregate construction, `VertexIndex`, `Position`, and
location 0 handoff. Fragment lowering covers the Image resource, descriptor
set/binding 0/0, sampling, tone projections, multiplication, and location 0
output.

The internal validator checks word bounds, section order, unique and defined
IDs, instruction references, entry/function structure, execution model,
interface variables and decorations, locations, builtins, push offsets,
descriptor bindings, and promised sidecar metadata. Successful products are
stored on the Package at stable logical paths:

```text
shader/Default2D/vertex.spv
shader/Default2D/vertex.spv.interface
shader/Default2D/pixel.spv
shader/Default2D/pixel.spv.interface
```

## Worker-local Realm

`Runtime::Realm` is the first managed runtime owner. It uses the existing
Bibliotheca allocation substrate without changing Perimortem. One Realm is
pinned to one worker and provides nonmoving stable addresses for live managed
objects.

Runtime allocation cells are not semantic facts. Realm derives trace slots
from real managed Type fields and managed range/aggregate element Types. Live
App state and Body frames root managed values explicitly. Collection marks from
those roots, follows aggregate/object edges, and reclaims unreachable graphs
including cycles. A rooted graph survives collection. Cross-worker mutable
access is rejected. Whole-Realm teardown deterministically returns every
remaining allocation.

The source-visible storage boundary remains small: inline scalar, structure,
vector, choice/range values; managed object references; read-only `View[T]`;
writable `Access[T]`; and future Foreign-only raw carriers. `View` is not
writable, `Access` is writable, and neither claims `noalias`. Backing records
remain private runtime data.

## App lifecycle and Graphics

`Model::App` is a managed Type and composition owner. It retains ordinary
Callable edges for start, frame, and stop roles, explicit render-root
Addressables, and explicit Render-to-Shader bindings. Callable spellings are
presentation only; runtime never searches magic names. App state Layout is
sealed before its self-typed Bodies are built so receiver identity is real and
no proxy is needed.

`Runtime::Host` guarantees this sequence:

```text
create worker-local Realm
-> allocate and root App state
-> invoke start exactly once
-> construct a typed Runtime Frame
-> invoke frame for the requested iterations
-> discover render roots through App's explicit role edges
-> resolve the selected Shader and terminal products through Package edges
-> submit one neutral Graphics transaction per configured root
-> consume a real Continue, Exit, or Failure runtime value
-> invoke stop exactly once on normal exit and defined sink failure
-> release Graphics exactly once
-> unroot/collect state and tear down Realm
```

`Invalid` never crosses into execution as a runtime result. The first host
legality profile supports the constants, branching, jumping, and return values
needed by Demo and rejects unsupported Body alternatives before execution.

### Scene state machine

Scene is the next managed owner below App. Each Scene retains ordinary
`enter`, `frame`, and `exit` Callable edges, state Addressables, render roots,
and its own typed signals. A Scene frame returns `Scene::Flow::stay` or emits
one of those signals. It neither names another Scene nor decides that the
process should terminate.

Each signal is a real owner with a complete payload Layout. Empty signals do
not require an invented payload Type. Runtime Flow carries the emitting Scene
and signal coordinate plus fitted payload values, while the Archive retains
the real definition-ID edges.

App retains the initial Scene and the complete transition table. A transition
key is the real Scene and signal identity. Its action contains a direct target
Scene edge for replace or push, or an explicit pop or App-exit action. This
owner shape permits a Splash-to-Title-to-Splash loop without cyclic member
evaluation. The Scene members are independently completed before the App
member connects them.

Runtime will apply one transition after a Scene frame completes:

```text
observe Scene::Flow
-> find the exact App-owned transition edge
-> call the outgoing Scene exit role
-> release its external resources according to stack policy
-> allocate and root replacement state in the worker Realm
-> call the incoming Scene enter role
-> publish the new active Scene set
```

The transaction must fail through the App cleanup path before partially
entered Scene state becomes current. Active Scene render roots join App roots
only after evaluation and use the App's explicit Render-to-Shader mapping.
Graphics receives concrete Render values and remains unaware of Scene.

The source contract lives in
[`../apps/canonical/scene_demo`](../apps/canonical/scene_demo/). Production
validation currently parses only its Package descriptor. Scene evaluation,
general host Body execution, concrete Render payload submission, transition
execution, and durable Scene records remain required before this flow is an
implemented vertical.

Graphics owns a language-neutral `Transaction` and Sink contract containing
frame/root/binding coordinates plus opaque stage modules and interface
metadata. It includes no TTX frontend concepts. The headless Sink proves that
the configured `Render2D` root selects `Default2D`, without a window server or
GPU. This first transaction does not yet evaluate the App field or carry a
concrete `Render2D` value payload; closing that execution edge is required
before claiming actual render-value submission.

## Durable Package behavior

Archiver runs only after these concrete owners exist. Archive format 1 stores
the rich graph:

- Types and semantic Layouts;
- Generic identities, concrete arguments, and materializations;
- Addressables, Callables, constants, Expressions, and Bodies;
- managed-Type facts;
- Render, Shader, Stage, interface, representation, and App relations;
- direct dependencies and cross-package definition edges;
- Shader products, terminal paths, metadata, and bytes.

Local references use Package definition IDs. External semantic references use
dependency ordinal plus dependency definition ID. Render, Shader, and App use
stable contract UUID plus explicit schema Major.Minor and bounded payload.
There is no central extension registry.

Reader decodes and bounds every section before construction, creates final
nonmoving owner identities where self/member edges require them, reconstructs
Generic Types through the real materialization cache, validates Layouts,
Bodies, Dialect facts, dependency/Namespace legality, products, and terminals,
then publishes one `Packages::Precompiled`. Corruption returns `Invalid`; no
partial Package, dangling buffer, null semantic edge, or Source capability
escapes.

Repository owns registered archive bytes and recursively restores exact
dependencies. The acceptance run archives Math, Runtime, Graphics, and Demo,
discards caller buffers, restores the closure, and reruns the same Host against
Demo. The restored Package proves `Compiled` but not `Interpreted`. Lifecycle
observations, Render/Shader graph relations, terminal paths, interface
metadata, and Default2D SPIR-V bytes match the source-backed run; restored
bytes are independently owned.

## Connected worked flow

The authored sources are:

- [`standard/perimortem/graphics/package.ttx`](standard/perimortem/graphics/package.ttx)
  resolves Math, loads Library/Render/Shader members, and exports the selected
  identities;
- [`standard/perimortem/graphics/library/types.ttx`](standard/perimortem/graphics/library/types.ttx)
  constructs Color, Image, and the sampling Callable;
- [`standard/perimortem/graphics/renderers/renderer_2d.ttx`](standard/perimortem/graphics/renderers/renderer_2d.ttx)
  constructs the Render2D contract;
- [`standard/perimortem/graphics/shaders/default_2d.ttx`](standard/perimortem/graphics/shaders/default_2d.ttx)
  constructs both Default2D Stage Bodies;
- [`../apps/ttx/demo/main.ttx`](../apps/ttx/demo/main.ttx) constructs App state,
  explicit lifecycle roles, render root, and `Render2D -> Default2D` binding.

The larger human-review fixture at
[`../apps/canonical/scene_demo`](../apps/canonical/scene_demo/) specifies two
Scene members and their App-owned transition cycle. It is intentionally not
included in the implemented interpretation diagram below.

Their production interpretation is:

```text
authored Library/Render/Shader/App token bytecode
-> real Type/Generic/Addressable/Callable/Body identities
-> real Render2D and Default2D relations
-> target Representation -> validated SPIR-V terminal products
-> App runtime plan -> Realm lifecycle -> neutral headless submission
-> archive format 1 records/definition edges/terminal bytes
-> source-free restored graph -> equivalent Realm lifecycle and submission
```

See [`../ttx/ttx_design.md`](../ttx/ttx_design.md) for complete authored Shader
and App examples and [`archiver/README.md`](archiver/README.md) for exact format
and publication invariants.
