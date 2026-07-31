# Tetrodotoxin Design

Tetrodotoxin is the concrete host for TTX. TTX defines lexical bytecode and the
closed target independent semantic substrate. Tetrodotoxin defines the source
Dialects, construction Environment, generators, runtime policy, and package
policy that use it.

Live owner boundaries appear before planned products. Grammar and acceptance
data do not make a planned product part of the current contract.

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

The flow is a staged implementation contract. Puffer does not acquire
language, package, compiler, or linker policy by coordinating their owners.
The current Puffer target exposes only its LSP process.

Language defines no concrete source grammar. It provides the stateful Dialect
interface, its common Monograph root, and deterministic parser fragments.

Package supplies the first concrete Dialect and Monograph shape. Its
implemented contract interprets dependency requests and exact Source name to
path bindings into a Package Monograph.

Environment owns Workspace orchestration and the composed Dialects, Retention,
and Resolution objects that host interpretation. Together they own graph
allocation, Dialect lifetime, imported Monograph lifetime, exact authored
source name lookup, staged source order, retained source bytes, ordered
semantic completion, dependency restoration, and the TTX registry supplied to
each Dialect.

Library owns CPU language semantics that are not universal TTX facts. Its
current target also owns native x86_64 instruction assembly. A future Library
Dialect and compiler extend Library without copying the TTX graph.

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
source local Abstract interpretation context
```

Interpretation returns either no result or one
`Language::Dialect::Monograph&`. The concrete Monograph is an Abstract and is
constructed in the supplied Arena. It retains the opening Documentation and its
host Dialect so later queries use the same semantic context that created it.
The installed Dialect continues to retain the Workspace wide registry supplied
at construction. The fourth interpretation argument instead selects Workspace
for direct sources and an ownerless root, or the exact owning Package Monograph
for a staged member.

Language owns two shared parser fragments today:

1. `Parser::Comment` greedily consumes adjacent comment lines and preserves
   empty authored lines in one Documentation Block.
2. `Parser::Dialect` consumes the universal Dialect instruction and returns the
   exact authored Dialect name.

Concrete body grammar remains on the concrete Dialect. Shared spelling alone
does not justify moving a semantic parser into Language.

The accepted lifecycle adds two owner neutral operations to the common
Monograph and Dialect boundary:

1. Environment Retention invokes one ordered Monograph post pass after local
   staging and dependency restoration drain. The operation returns failure
   without receiving a textual error sink.
2. A concrete Dialect encodes and restores its own opaque precompiled Monograph
   payload through the importing Workspace Arena.

Language owns only that dispatch shape. It does not define a Package Archive,
terminal registry, concrete payload schema, or cross owner product variant.
Concrete post pass owners log graph details that would be lost on return.
Retention keeps the authored input identity needed to turn a returned failure
into a user facing diagnostic. Workspace stages source and Resolution drains
dependencies before Retention begins completion.

## Environment transaction

`Environment::Workspace` is the public lifetime and dispatch owner for one
semantic construction environment. It owns one Arena, exact global authored
source lookup, and import orchestration. `Environment::Dialects` owns installed
names, concrete Dialect instances, and exact dispatch. `Environment::Retention`
owns Monograph lifetime, authored origin, discovery order, and completion.
`Environment::Resolution` owns exact dependency traversal and restored Package
cache state while borrowing the other Environment owners explicitly.

Dialect installation is typed:

```text
workspace.install_dialect<Package::Dialect>("Package")
```

Dialects constructs a distinct instance for each successful installation.
Workspace declaration order destroys Resolution, then Retention and its
Monographs, then Dialects and its instances before releasing their shared
Arena. That ordering keeps Dialect state alive while retained Monographs can
refer to their host.

The accepted production transaction is staged:

```text
Workspace stages an explicit root semantic name and source path
-> Package Storage reads one confined, same opened object
-> Workspace retains the bytes
-> Workspace parses Documentation and the universal Dialect declaration
-> Workspace dispatches the remaining Cursor to the exact installed Dialect
   with Workspace or the exact owning Package as interpretation context
-> the Dialect constructs its real Monograph in the Workspace Arena
-> Retention keeps it and Workspace publishes the authored semantic name
-> Environment Resolution resolves exact dependency Archives
-> Package Source bindings stage more inputs in authored order
-> after all staging and restoration drain, Retention runs post pass in
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
Arena. Direct import returns the published Monograph Option. Package import
additionally receives the explicit root identity, Version, and Repository.
Source staging failure completes the retained prefix and returns no result
before Resolution begins. Resolution returns either the root Monograph or a
populated Repository `SelectionError`. It preserves an exact selection category
and uses `Unknown` when another diagnosed resolution or post pass failure has
no Repository category. Package local members are not published in the
Workspace source map. Exact Archive dependencies restore into the Workspace
Arena before every retained Monograph runs post pass once in discovery order.

Textual errors require authored text. `Ttx::Lexical::Errors::Report` receives
an explicit source name, source body, and `Ttx::Lexical::Span` from the owner
of that text. Context free filesystem, Archive, and Repository validation
instead logs its exact local names, values, offsets, and transaction stage
through `Diagnostics::Log`, then returns failure. Resolution or Puffer owns the
later user facing error because only that layer can attach the failure to an
authored Dependency, Source, or compile request. Neither channel replaces the
other.

## Package Dialect

The Package body has two ordered regions. Dependency declarations such as Math
and Graphics precede Source bindings such as Scenes::Splash, Scenes::Title, and
Main.

`Package::Language::Dependency` is an exact authored request containing its
local name, package name, and pinned version. It is not the resolved external
package.

`Package::Language::Source` is an exact authored binding from one semantic name
to one package path. Package Storage opens the path beneath the package root
and Workspace stages the member under the local name. No path segment,
filename, or file order derives semantic identity.

`Package::Language::Parser::Name` consumes the shared contiguous qualified name
grammar used by Dependency and Source statements. TTX Lexicon remains the sole
owner of each Type segment and exact separator Code spelling.

`Package::Language::Monograph` retains opening Documentation, ordered
Dependency requests, aligned authored statement Spans, and ordered Source
bindings. It owns one exact Package local scope whose real
`Ttx::Model::Alias` edges target completed source Monographs and restored
Package roots in the same Workspace Arena. Exact lookup returns the Alias edge
itself or shared Invalid. Qualified names remain one opaque key, and Package
does not split a route or derive it from a source path.

Source parsing returns only Source while exposing its complete successful
statement Span as separate transaction output. Package Dialect uses that Span
to diagnose a Source semantic name colliding with a Dependency alias. Failed
parsing leaves the output invalid, and no Source range becomes durable
Monograph state.

Authored Source and Dependency inventories reserve one shared name scope.
Source free roots retain ordered Dependencies with no authored Spans or Source
paths and accept validated Archive member names during restoration. Duplicate,
undeclared, direct cycle, and cross kind binding attempts fail before another
Alias is allocated, preserving the first edge. The Monograph retains no
filesystem handle, fetched product, Archive bytes, Repository state, source
text, path, Token, diagnostic state, target artifact, or archive record.

The current Package target contains the complete authored manifest
interpretation and confined Storage. Namespace `Package::Archive` owns Format 1
facts on the value class `Archive`, validation and materialization on `Reader`,
and deterministic encoding on `Writer`. Namespace `Package::Repository` owns
the concrete `Repository` transaction, its exact Input and Artifact
declarations, separately borrowed native artifact paths, and normalized
declared-only Archive and native output declarations. Environment Resolution
consumes those existing owners for dependency restoration. Application selection
remains future Package assembly work.

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

Library contracts retain real TTX Type, Layout, Addressable, and Callable
edges. They do not redeclare those shared owners.

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
and native archive construction. Supplying object input does not transfer that
ownership to Library.

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
declared outputs. Package Repository validates those exact semantic, native,
and publication declarations without scanning for inputs, loading native
bytes, or writing outputs itself.

The accepted Linker products are System V static binary archives, ELF shared
libraries, and complete ELF executables. Final ELF linkage is performed in
repository code. A host linker may independently consume a generated static
archive as acceptance evidence, but it is not the production implementation of
an executable product.

The current Puffer target does not implement the compile transaction. Puffer
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
$[resources/table.bin]:[0, 64]
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

Namespace `Package::Archive` owns the implemented Format 1 contracts:
`Package::Archive::Archive` retains the completed facts,
`Package::Archive::Reader` validates and materializes the envelope, and
`Package::Archive::Writer` emits it canonically. Namespace
`Package::Repository` owns the `Repository`, `Input`, `Artifact`, and `Output`
classes. Repository borrows explicit input declarations under its caller Arena
lifetime, asks `System::Path` to construct its derived normalized output routes
directly in that Arena, and selects Archives by exact identity and pinned
Version. Archive and native output inventories use the same identity, Version,
and artifact ID key while remaining separate operations. An Archive contains
exact Package identity and version,
semantic member and Dialect names, concrete Dialect payload framing, dependency
requests, exported semantic routes, and native artifact and symbol locators.
`Archive::Sections` and `Archive::header_size` are the shared public section
vocabulary and fixed header size consumed by Reader and Writer.

Each installed concrete Dialect owns the versioned payload it encodes and
restores. Restored Monographs are allocated in the importing Workspace Arena.
Package validates the envelope without depending on Library, App, Scene,
Render, or Shader payload schemas.

An Archive contains no source bytes, source path as semantic identity, process
address, parser Cursor, filesystem handle, target cache, or Linker object bytes.
The current codec validates complete envelopes before retaining typed record
ranges in the caller Arena and emits one deterministic canonical encoding. The
input bytes remain borrowed for that Arena lifetime, so an Arena backed file
read reaches Archive without another copy. Archive itself remains a regular
value over stable views. Reader logs exact framing and relationship failures
without constructing a textual error for binary input. Repository caches only
successfully decoded Archives, exposes native filesystem paths without reading
them, and normalizes declared-only publication routes without materializing
output. Its logs preserve both declarations for duplicate keys and route
collisions. Package does not coordinate restoration. Environment Resolution
reconstructs each Package root from the Archive envelope and calls the
installed concrete Dialect for every opaque member payload.

## Current evidence boundary

The current tree provides the following implemented surfaces.

1. TTX provides lexical and closed semantic targets.
2. Language provides shared Comment and Dialect parsers.
3. Language provides the stateful Dialect and Monograph interfaces.
4. Environment provides Workspace import orchestration, Dialect installation
   and dispatch, Monograph Retention, source free Package Resolution, Package
   local binding, ordered completion, and authored root name lookup.
5. Package provides Dependency, Source, Monograph, complete manifest
   interpretation, one exact Alias backed Package scope, confined Storage,
   Archive Format 1, and exact Repository selection.
6. Library provides language contracts and scalar Types.
7. Library and Shader provide CPU and SPIR V instruction assemblers.
8. Linker provides source independent linking machinery.
9. Puffer provides an LSP process but no compile orchestration.

That inventory does not prove complete parsing for Library, App, Scene, Render,
Shader, or Foreign. It also does not prove CPU semantic lowering, embedded
resource folding, runtime execution, Puffer compile orchestration or physical
publication, typed Object Module production, final native executable emission,
or Library, App, Scene, Render, Shader, or Foreign payload restoration.

A fixture, README, target build, or test written beside an implementation is not
an independent semantic oracle.
