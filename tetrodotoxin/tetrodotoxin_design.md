# Tetrodotoxin Design

Tetrodotoxin is the concrete host for TTX. TTX defines lexical bytecode and the
closed target independent semantic substrate. Tetrodotoxin defines the source
Dialects, construction Environment, generators, runtime policy, and package
policy that use it.

This document describes the live owner boundaries first. Planned products are
called out explicitly and do not become current contracts merely because their
grammar or acceptance data already exists.

## Owner graph

The active build dependency direction is:

```text
Environment -> Package -> Language -> TTX -> Perimortem
Library -> TTX -> Perimortem
Shader -> Perimortem
Linker -> Perimortem
```

The accepted production ownership flow is broader than the current build
graph:

```text
Bazel request
-> Puffer orchestration
-> Environment Workspace
-> installed Package and concrete source Dialects
-> completed owner Monographs
-> Package Archive and Linker Object Modules
-> Linker native product
-> declared Bazel outputs
```

This flow is a staged implementation contract. Puffer does not acquire
language, package, compiler, or linker policy by coordinating their owners.
The current Puffer target exposes only its LSP process.

Language defines no concrete source grammar. It provides the stateful Dialect
interface, its common Monograph root, and deterministic parser fragments.

Package supplies the first concrete Dialect and Monograph shape. Its
implemented contract interprets dependency requests and exact Source name to
path bindings into a Package Monograph.

Environment owns the Workspace that installs concrete Dialects and hosts their
interpretation. It owns graph allocation, Dialect lifetime, imported Monograph
lifetime, exact authored source name lookup, staged source order, retained
source bytes, ordered semantic completion, and the TTX registry supplied to
each Dialect.

Library owns CPU language semantics that are not universal TTX facts. Its
current target also owns native x86_64 instruction assembly. A future Library
Dialect and compiler extend this owner without copying the TTX graph.

App owns startup and lifecycle policy. Scene owns retained Scene declarations,
live Scene instances, declared child identity, and render submission facts.
The future `tetrodotoxin/graphics` owner supplies language neutral retained
graphics child and submission contracts. Render and Shader remain concrete
Dialect owners rather than substitutes for that runtime boundary.

No component reconciles competing semantic models. A concrete Dialect creates
its real Monograph using TTX identities and its own narrower contracts.

## Language contracts

`Language::Dialect` is the C++ extension point for one source grammar. A
compiled toolchain installs concrete Dialect types into an Environment
Workspace under exact authored names.

Each Dialect receives the shared TTX registry when Environment constructs it.
Its interpretation entry point receives:

```text
Environment owned Arena
forward TTX Cursor
opening Documentation
shared Abstract registry
```

Interpretation returns either no result or one
`Language::Dialect::Monograph&`. The concrete Monograph is an Abstract and is
constructed in the supplied Arena. It retains the opening Documentation and its
host Dialect so later queries use the same semantic context that created it.

Language owns two shared parser fragments today:

1. `Parser::Comment` greedily consumes adjacent comment lines and preserves
   empty authored lines in one Documentation Block.
2. `Parser::Dialect` consumes the universal `dialect : Type;` instruction and
   returns the exact authored Dialect name.

Concrete body grammar remains on the concrete Dialect. Shared spelling alone
does not justify moving a semantic parser into Language.

The accepted lifecycle adds two owner neutral operations to the common
Monograph and Dialect boundary:

1. Workspace invokes one ordered Monograph post pass after local staging and
   dependency restoration drain.
2. A concrete Dialect encodes and restores its own opaque precompiled Monograph
   payload through the importing Workspace Arena.

Language owns only that dispatch shape. It does not define a Package Archive,
terminal registry, concrete payload schema, or cross owner product variant.
These operations are planned and are not present in the current interface.

## Environment transaction

`Environment::Workspace` is the lifetime and dispatch owner for one semantic
construction environment. It owns one Arena for installed Dialects and
interpreted graph values. It retains installed Dialect instances, binds exact
authored names to those Dialects, and binds imported routes to Monographs.

Dialect installation is typed:

```text
workspace.install_dialect<Package::Dialect>("Package")
```

Environment constructs a distinct Dialect instance for each successful
installation. It destroys those instances before releasing their shared Arena.
That ordering keeps Dialect state alive while retained Monographs can refer to
their host.

The accepted production transaction is staged:

```text
Workspace stages an explicit root semantic name and source path
-> Package Storage reads one confined, same opened object
-> Workspace retains the bytes
-> Workspace parses Documentation and `dialect : Type;`
-> Workspace dispatches the remaining Cursor to the exact installed Dialect
-> the Dialect constructs its real Monograph in the Workspace Arena
-> Workspace retains it under the authored semantic name
-> Package resolves exact dependency Archives
-> Package Source bindings stage more inputs in authored order
-> after all staging and restoration drain, Workspace runs post pass in
   retained order
```

`Workspace::resolve_context(name)` returns the retained Monograph or the TTX
Invalid object. The Workspace therefore supplies a total graph query without
introducing a nullable semantic edge. The path remains available for source
diagnostics but never becomes a semantic name implicitly.

Environment does not own concrete Package or Library grammar. It also does not
own filesystem confinement, target lowering, runtime state, archive encoding,
or an additional Namespace model. Direct import accepts separate semantic name,
diagnostic path, and content views and copies them into the Workspace Arena.
Local Package import drains an Arena backed FIFO of separate semantic names and
logical routes. Package Storage content enters the same semantic import
transaction directly because Storage and its views already belong to that
Arena. Both public operations return the imported Monograph Option. Dependency
restoration and ordered post pass remain absent.

## Package Dialect

The Package body has two ordered regions:

```ttx
resolve Math : Perimortem.Math = "1.0";
resolve Graphics : Perimortem.Graphics = "1.0";

source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
source Main from "main.ttx";
```

`Package::Language::Dependency` is an exact authored request containing its
local name, package name, and pinned version. It is not the resolved external
package.

`Package::Language::Source` is an exact authored binding from one semantic name
to one package path. Package Storage opens the path beneath the package root
and Workspace stages the member under the local name. No path segment,
filename, or file order derives semantic identity.

`Package::Language::Monograph` retains opening Documentation, ordered
Dependency requests, and ordered Source bindings. It retains no filesystem
handle, fetched package, opened member source, target artifact, or archive
record.

The current Package target contains the complete authored manifest
interpretation and confined Storage. Dependency acquisition, application
selection, Package Archive encoding and restoration, and exact repository
selection remain planned Package work.

`main.ttx` remains a filename convention. Future package assembly selects the
sole completed App Monograph rather than granting its filename or local Source
name semantic authority.

## Library language

TTX owns the shared target independent Type, Value, Layout, Addressable, and
Callable contracts. Library adds the CPU language semantics that not every
Dialect needs.

1. Expression, Binding, and Projection represent Library value semantics.
2. Constant and its concrete domains represent retained Library values.
3. Generic, Access, View, and Fixed represent Library materialization.
4. Concrete Bool, integer, and real Types provide Library scalar identities.
5. Static and Self distinguish Library Callable invocation.

These contracts retain real TTX Type, Layout, Addressable, and Callable edges.
They do not redeclare those shared owners.

Library source order will not determine binding. Its future Dialect may reserve
stable semantic identities before definitions, initializers, and executable
bodies are completed. It must finish those same objects rather than retain
Cursor positions or build a second declaration graph.

The exact durable executable body contract and the complete Library Dialect are
not implemented. Their absence does not reopen TTX or justify a placeholder
intermediate representation.

## CPU compilation

Library owns the reusable CPU compilation path. It may lower completed CPU facts
retained by Library, App, or Scene Monographs without converting those owners
into Library source.

Target lowering may derive sizes, alignments, offsets, pointer forms, register
classes, calling convention carriers, and relocations. Those are target facts,
not TTX semantic identities.

The current executable Library surface is the source independent
`Library::Assembler::x86_64`. Future compiler work must make target decisions
before asking that assembler to encode instructions.

Linker owns source independent objects, symbols, relocations, target formats,
and native archive construction. Library does not absorb Linker merely because
it supplies object input.

The accepted native terminal is `Linker::Object::Module`, a coherent owner of
sections, symbols, and relocations. Library, App, and Scene may each submit
completed CPU facts to the Library compiler, which lowers them into Object
Modules without converting their Monographs into Library source.

Package Archive is the separate durable semantic terminal. It never contains
Linker object bytes. Linker input and product policy never become Package
semantic state.

## Puffer and terminal production

Bazel supplies exact source and resource inputs, root semantic name, Package
identity and pinned version, dependency products, and declared output paths.
Puffer selects one statically compiled toolchain composition and constructs one
Workspace.

Library mode installs Package and Library. Binary mode additionally installs
App, and installs Scene or other concrete Dialects only when their real compile
path is part of that selected toolchain. An uninstalled authored Dialect
receives the ordinary unknown Dialect diagnostic.

After Workspace completion, Puffer renders accumulated diagnostics and exits
nonzero before terminal work when any error exists. On success it asks the
concrete Monograph owners for their typed products. It does not introduce a
universal terminal base, opaque product registry, or cross owner variant.

Package facts encode a Package Archive. Completed CPU facts lower to Linker
Object Modules. Puffer loads dependency native products only for the link
phase, asks Linker for the requested native product, and writes only the
declared outputs.

The accepted Linker products are System V static binary archives, ELF shared
libraries, and complete ELF executables. Final ELF linkage is performed in
repository code. A host linker may independently consume a generated static
archive as acceptance evidence, but it is not the production implementation of
an executable product.

The current Puffer target does not implement this compile transaction. It
remains the planned application orchestration boundary.

## Concrete Dialect responsibilities

Planned top level Dialects follow the same Environment installed Dialect and
Monograph lifecycle:

1. Library owns ordinary CPU definitions, values, Callables, and bodies.
2. Render owns render values, resources, and required Stage contracts.
3. Shader owns exact Render implementation and Shader Stage bodies.
4. App owns startup profiles, generated platform entry semantics, Program and
   Scene lifecycle policy, and Scene transitions.
5. Scene owns state, signals, retained declared children, render submission
   facts, and prepare, pause, resume, update, and release roles.
6. Graphics owns language neutral retained graphics children and ordered
   submission facts without becoming a top level source Dialect by default.

Foreign remains embedded syntax for CPU capable parent Dialects. It is not an
installed top level Dialect and does not create an independent Monograph.

App owns `Windowed`, `Terminal`, and `Headless` startup profiles. There is no
Runtime Package. Linked support libraries, generated entry code, package
configuration, and lifecycle policy together produce runtime behavior.

The accepted App lifecycle inventory is Program and Scene. Program retains one
direct `Library::Language::Callables::Static` edge that takes no parameters and
returns Void. Generated platform entry code calls it once. Command line
arguments remain queryable process state rather than injected Callable
parameters. Scene retains direct Self role edges and App owned transition
policy. Neither lifecycle depends on a function named `main`.

App owns Scene transitions:

```text
push     pause current, prepare pushed
pop      release current, resume prior
replace  release current, prepare replacement
exit     release current without resume
```

Each live Scene constructs and attaches its declared children in authored order
before `prepare`. After `update`, visible attached graphics children are
collected automatically in retained tree order. Authored Scene update code
mutates state and never performs render submission. App applies transitions
only after the update and submission facts for that frame are stable.

Scene `release` runs before automatic reverse order destruction of the retained
child subtree. Declared children are nonnull and retain their identity for the
complete Scene lifetime. Dynamic attachment and queued individual release are
separate planned work.

## Embedded resources

Every authored source and embedded resource is intended to live beneath one
package root. Another location is reached only through an exact Package
Dependency.

The accepted operand is package root relative:

```ttx
const file_header : Fixed[Unsigned_8, 64] =
  $[resources/table.bin]:[0, 64];
```

Package Storage and future resource consumers follow six requirements.

1. Reads remain confined to one opened package root.
2. Absolute and escaping routes are rejected.
3. An empty file remains distinct from a read failure.
4. Repeated logical resource reads are deduplicated.
5. Constant folding may retain only reachable byte slices.
6. Resolution never falls back to the process working directory or the
   containing source directory.

Package Storage implements confined root reads, route rejection, empty success,
successful read caching, and fallback absence. No current concrete Dialect
interprets an embedded resource operand or folds reachable byte slices.

## Durable package products

Package owns the planned `Package::Archive` envelope, reader, writer, and exact
repository selection. An Archive contains exact Package identity and version,
semantic member and Dialect names, concrete Dialect payload framing, dependency
requests, exported semantic routes, and native artifact and symbol locators.

Each installed concrete Dialect owns the versioned payload it encodes and
restores. Restored Monographs are allocated in the importing Workspace Arena.
Package validates the envelope without depending on Library, App, Scene,
Render, or Shader payload schemas.

An Archive contains no source bytes, source path as semantic identity, process
address, parser Cursor, filesystem handle, target cache, or Linker object bytes.
No Package Archive codec, exact repository, or source free restoration exists
in the current Package target.

## Current evidence boundary

The current tree provides the following implemented surfaces.

1. TTX provides lexical and closed semantic targets.
2. Language provides shared Comment and Dialect parsers.
3. Language provides the stateful Dialect and Monograph interfaces.
4. Environment provides Workspace installation, direct dispatch, confined
   local Package staging, retention, and authored source name lookup.
5. Package provides Dependency, Source, Monograph, complete manifest
   interpretation, and confined Storage.
6. Library provides language contracts and scalar Types.
7. Library and Shader provide CPU and SPIR V instruction assemblers.
8. Linker provides source independent linking machinery.
9. Puffer provides an LSP process but no compile orchestration.

That inventory does not prove complete parsing for Library, App, Scene, Render,
Shader, or Foreign. It also does not prove CPU semantic lowering, embedded
resource folding, durable package encoding, runtime execution, Puffer compile
orchestration, typed Object Module production, final native executable
emission, or source free restoration.

A fixture, README, target build, or test written beside an implementation is not
an independent semantic oracle.
