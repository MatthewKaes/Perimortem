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
| Foreign (embedded) | complete source-local native data and Callable imports under an explicit FFI/ABI selector | Specified, not implemented |

Foreign is an embedded Dialect rather than a Source envelope. Library, Scene,
App, and any other CPU-executable Dialect admit it only through explicit opt-in;
Package and Shader do not. The grammar and semantic contract are normative,
but the current vertical has no Foreign parser, concrete durable owner,
Archiver records, or native lowering. Managed Library objects are not silently
accepted by Shader.

## Package transaction

The production entry is a package descriptor path, not a hand-created token
stream or semantic fixture.

`Puffer::Package::Descriptor` parses the Source-owned Tokenizer for
`package.ttx`. Exact external requests and member declarations have these
forms:

```ttx
resolve Math : Perimortem.Math = "1.0";
source "library/types.ttx";
```

The version is two independent unsigned components. Member paths are normalized
relative to the descriptor and rejected when they escape the package root. A
member's source envelope selects its Dialect. Application construction selects
the sole evaluated App root; `main.ttx` is only a naming convention.

`Puffer::Package::Container` performs this transaction:

```text
parse descriptor
-> resolve every exact external Manifest/Package
-> locate and load every explicit member Source
-> let every Source own its bytes, Tokenizer, Arena, and roots
-> verify every member Dialect envelope
-> populate one Environment with exact external bindings
-> evaluate members in descriptor order
-> append each completed real product for later members
-> evaluate an optional Package body or select the sole App root
-> compile reachable Shader programs into terminal products
-> publish one Packages::Sources
```

No member is evaluated before all source streams and external bindings exist.
Every Source borrows the same Environment and owns no package imports or
dependency vector. An embedded Foreign import is a source-local ABI contract,
not a package dependency or provider selection. `Packages::Sources` retains
the explicit members, derives distinct direct dependency Packages from the
Environment, owns terminal products, and assigns canonical definition IDs to
the complete reachable graph.

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

The package container transaction is the lifetime and finalization owner. It
keeps Environment, its formulas and Materializations writer, and every member
Source alive while consumers of Sources can query them. Construction is open
while bindings and readiness advance monotonically. Before
`Packages::Sources` derives definition IDs, the Dialect owners complete every
reachable Type, signature, and Body, call each concrete mutable surface's
`seal()` operation, and validate the graph. No Compiler or Writer observes the
graph before that barrier, and no graph mutation occurs afterward. Consumers
finish, materialization queries stop, member Sources are destroyed, and
Environment is destroyed last.

Once Foreign is implemented, the same barrier seals each Source's private
import surface and validates its ABI selector, symbol uniqueness,
Addressable capabilities, Types, and Callable Layouts. No Compiler, Linker, or
Writer may observe a partially declared import surface.

### Embedded resource transaction

The package Container establishes one canonical package root before it creates
the shared Environment. The Puffer Workspace opens that root once and owns the
resulting directory capability for the whole package transaction. Environment
borrows the capability. It never owns a process path, current working
directory, Source directory, repository search object, or independently opened
filesystem handle.

During semantic literal parsing, `$[path]` asks Source's Environment for a
package-relative resource. Environment first rejects an absolute or lexically
escaping authored route, normalizes every accepted segment to one relative
logical route, and queries its route cache. A cache miss invokes the borrowed
Workspace capability with that normalized route. Literal never sees the
Workspace, a filesystem path, or an operating-system handle.

The Workspace operation returns one closed result:

- success owns the complete bytes read from the selected regular file, and a
  zero-length success is distinct from failure;
- missing means no selected object exists;
- non-file means the opened object is not a regular file;
- unreadable means the selected object cannot be opened or completely read;
  and
- outside-root means filesystem resolution attempted to leave the pinned
  package root.

Absolute and lexical-escape failures are produced before the Workspace
operation. The other failures are observations from the confined open. No
result has a public unselected state, and no failure carries a partial byte
buffer.

Confinement and reading are one same-opened-object transaction. Workspace
resolves and opens the normalized route relative to its pinned root with a
kernel-enforced beneath-root constraint, queries file kind on that opened
object, and reads that same object through end of file. It never performs
`realpath` followed by `System::File::read`, never validates one pathname and
reopens another, and never returns a pathname or handle for Environment to
read. A Linux implementation may use `openat2` beneath-root resolution followed
by `fstat` and reads on the returned descriptor, but the same-opened-object
rule is the contract rather than a particular syscall spelling. An internal
symlink may resolve only when the confined open proves that its selected object
remains beneath the pinned root. Changing a symlink between validation and
read can therefore select neither an outside object nor a separately reopened
object.

Environment moves a complete success into transaction-owned backing before
publishing a cache entry. Repeated requests for the same normalized route
return one stable backing snapshot. Different routes with equal content may
share backing only after hash and byte equality both succeed; their Constant
identities remain distinct. Failure appends no route entry or byte backing.

Environment returns Literal a closed success-or-failure lookup result. Success
borrows the Environment-owned backing, including a valid empty view. Literal
maps each failure to a diagnostic on the embedded token and prevents Source
publication. Neither Environment nor Workspace writes a parser diagnostic.
There is no fallback to a Source directory, process working directory, or
unconfined `System::File::read`. Access to another location requires a resolved
Package rather than a filesystem escape.

The Literal parser constructs the concrete Bytes Constant before Body
publication. Constant evaluation may then reduce a fixed slice of a large
resource before Package traversal. Environment's package root, route cache,
and unused source bytes remain transaction state. Archive format `1` retains
only reachable Constants or deduplicated byte blobs, so restored compilation
requires no filesystem capability.

## Library and common Body

The shared Environment owns the common immutable `View`, `Access`, and `Fixed`
formulas plus one append-only Materializations writer keyed by formula and exact
parameter identity. Every member Source therefore observes the same
materialized address during one interpretation transaction. Type parameters
and returned Types must already resolve canonically to themselves. `None`, an
incomplete result, and same key reentrancy publish no key and remain retryable.
The first successful key is irrevocable in that writer.

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

### Embedded Foreign import surface

A CPU Dialect that opts into Foreign admits this shared member form:

```ttx
foreign "C" {
  public const library_limit : Unsigned_64;
  public state library_counter : Unsigned_64;
  public func library_add[
    .left : Unsigned_64,
    .right : Unsigned_64,
  ] -> Unsigned_64;
}
```

`"C"` is the FFI/ABI selector. It never names the package, object file,
library, or process that will provide a symbol. Package or link configuration
chooses providers independently.

Every block contributes to one reserved, private, source-local `foreign`
surface. The `public` modifier publishes the declaration to that surface; it
does not add the declaration or the surface to Package `Exports`. A dot selects
declared data and an arrow selects declared Callables. Only declared names
resolve; ambient Linker symbols cannot satisfy undeclared source uses:

```ttx
foreign.library_counter = foreign.library_limit;
return foreign -> library_add(
  foreign.library_counter,
  foreign.library_limit
);
```

Foreign `const` declares a global read-only external Addressable. It is not a
materialized TTX Constant. Foreign `state` declares a global writable external
Addressable, and Foreign `func` declares a bodyless external Callable with
complete parameter and result Layouts. The Type or Layout edges are already
complete when the declaration is accepted. These are explicit imports that a
native provider must satisfy, not incomplete TTX owners or declarations that a
later TTX body may complete.

The concrete Foreign surface retains ordered edges to real imported
Addressable, Writable, and Callable owners. Each imported owner retains its
block's selector, exact external symbol, access capability, documentation,
attributes, and real Type or Layout edges. This preserves repeated Foreign
blocks without inventing a provider relationship or losing their FFI
distinction. The surface does not copy those semantic owners or retain a native
provider, process address, or target relocation. Native compilation derives
undefined object/function symbols and target relocations from this graph; those
products remain compilation-local and the Linker matches them against
separately selected providers.

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
`prepare`, `update`, and `release` Callable edges, state Addressables, render
roots, and its own typed signals. The `Scene` prefix records the lifecycle role
of an ordinary authored Self Callable. A Scene update returns
`Scene::Flow::stay` or emits one of those signals. It neither names another
Scene nor decides that the process should terminate.

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

Runtime will apply one transition after a Scene update completes:

```text
observe Scene::Flow
-> find the exact App-owned transition edge
-> call the outgoing Scene release role
-> release its external resources according to stack policy
-> allocate and root replacement state in the worker Realm
-> call the incoming Scene prepare role
-> publish the new active Scene set
```

The transaction must fail through the App cleanup path before partially
entered Scene state becomes current. Active Scene render roots join App roots
only after evaluation and use the App's explicit Render-to-Shader mapping.
Graphics receives concrete Render values and remains unaware of Scene.

The source contract lives in
[`../apps/ttx/scene_lifetime`](../apps/ttx/scene_lifetime/). Production
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

The current format 1 Writer and Reader have no Foreign section. The prototype
extension must encode the source-local surface and complete ordered import
entries atomically, including each entry's selector, symbol, capability,
documentation, attributes, and Type or Layout edges. It must never encode
provider selection, a process address, or a target relocation. Until that
record exists, Writer must reject a graph containing Foreign rather than
silently discard its import contract.

Local references use Package definition IDs. External semantic references use
dependency ordinal plus dependency definition ID. Render, Shader, and App use
stable contract UUID plus explicit schema Major.Minor and bounded payload.
There is no central extension registry.

Reader decodes and bounds every section before construction, creates final
nonmoving owner identities where self/member edges require them, reconstructs
Generic Types through the real materialization writer, validates Layouts,
Bodies, Dialect facts, dependency/Namespace legality, products, and terminals,
then publishes one `Packages::Precompiled`. Corruption returns `Invalid`; no
partial Package, dangling buffer, null semantic edge, or Source capability
escapes.

Prototype format 1 durably supports only the existing `View::Type`,
`Access::Type`, and `Fixed::Type` result contracts and their ordered arguments.
Writer rejects another Generic result before publication. Supporting an
arbitrary formula would require an authored common provenance and formula
schema. Neither Materializations buckets nor a universal registry substitutes
for that contract.

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
[`../apps/ttx/scene_lifetime`](../apps/ttx/scene_lifetime/) specifies two
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
