# Tetrodotoxin Design

Tetrodotoxin is the concrete host for TTX. TTX defines lexical bytecode and the
closed target independent semantic substrate. Tetrodotoxin defines the source
Dialects, construction Environment, generators, runtime policy, and package
policy that use it.

This document describes the live owner boundaries first. Planned products are
called out explicitly and do not become current contracts merely because their
grammar or acceptance data already exists.

## Owner graph

The active dependency direction is:

```text
Environment -> Package -> Language -> TTX -> Perimortem
Library -> TTX -> Perimortem
Shader -> Perimortem
Linker -> Perimortem
```

Language defines no concrete source grammar. It provides the stateful Dialect
interface, its common Monograph root, and deterministic parser fragments.

Package supplies the first concrete Dialect and Monograph shape. Its intended
contract interprets dependency requests and exact Source name to path bindings
into a Package Monograph, but the current interpreter is incomplete.

Environment owns the Workspace that installs concrete Dialects and hosts their
interpretation. It owns graph allocation, Dialect lifetime, imported Monograph
lifetime, exact authored source name lookup, and the TTX registry supplied to
each Dialect.

Library owns CPU language semantics that are not universal TTX facts. Its
current target also owns native x86_64 instruction assembly. A future Library
Dialect and compiler extend this owner without copying the TTX graph.

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

The intended source import is one forward transaction:

```text
Workspace::import_source(name, path, contents, documentation, errors)
-> reject a duplicate imported semantic name
-> tokenize the borrowed contents
-> require an opening comment
-> parse `dialect : Type;`
-> select the installed Dialect with that exact name
-> call Dialect::interpret with the same Cursor
-> retain the returned Monograph under the authored source name
```

`Workspace::resolve_context(name)` returns the retained Monograph or the TTX
Invalid object. The Workspace therefore supplies a total graph query without
introducing a nullable semantic edge. The path remains available for source
diagnostics but never becomes a semantic name implicitly.

Environment does not own concrete Package or Library grammar. It also does not
own filesystem confinement, target lowering, runtime state, archive encoding,
or an additional Namespace model. Its current import API still combines the
semantic name and diagnostic path in one `route` parameter, so this intended
separation remains unfinished.

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
to one package path. Future Package input opens the path beneath the package
root and imports the member under the local name. No path segment, filename, or
file order derives semantic identity.

`Package::Language::Monograph` retains opening Documentation, ordered
Dependency requests, and ordered Source bindings. It retains no filesystem
handle, fetched package, opened member source, target artifact, or archive
record.

The current Package target contains only the model and interpreter scaffold for
this authored manifest. The scaffold does not yet complete a valid
interpretation. Confined filesystem reads, dependency acquisition, application
selection, Distribution, Reader, Writer, and durable format `1` are later
Package work.

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

## Concrete Dialect responsibilities

Future top level Dialects follow the same Environment installed Dialect and
Monograph lifecycle:

1. Library owns ordinary CPU definitions, values, Callables, and bodies.
2. Render owns render values, resources, and required Stage contracts.
3. Shader owns exact Render implementation and Shader Stage bodies.
4. App owns startup profiles, generated platform entry semantics, lifecycle
   policy, and Scene transition policy.
5. Scene owns managed state, signals, render roots, and prepare, pause, resume,
   update, and release roles.

Foreign remains embedded syntax for CPU capable parent Dialects. It is not an
installed top level Dialect and does not create an independent Monograph.

App owns `Windowed`, `Terminal`, and `Headless` startup profiles. There is no
Runtime Package. Linked support libraries, generated entry code, package
configuration, and lifecycle policy together produce runtime behavior.

Managed and Unmanaged App lifecycles retain a direct
`Library::Language::Callables::Static` edge. Scene lifecycles retain direct
`Library::Language::Callables::Self` role edges. No lifecycle depends on a
function named `main`.

The Echo fixture introduces a Program lifecycle with one Static Callable that
takes no parameters and returns Void. Generated platform entry code calls it
once. Command line arguments remain queryable process state rather than
injected Callable parameters. The final lifecycle inventory still must decide
whether Program coexists with Managed and Unmanaged or supersedes one of those
older names.

App owns Scene transitions:

```text
push     pause current, prepare pushed
pop      release current, resume prior
replace  release current, prepare replacement
exit     release current without resume
```

## Embedded resources

Every authored source and embedded resource is intended to live beneath one
package root. Another location is reached only through an exact Package
Dependency.

The accepted operand is package root relative:

```ttx
const file_header : Fixed[Unsigned_8, 64] =
  $[resources/table.bin]:[0, 64];
```

Future Package input work has six requirements.

1. Reads remain confined to one opened package root.
2. Absolute and escaping routes are rejected.
3. An empty file remains distinct from a read failure.
4. Repeated logical resource reads are deduplicated.
5. Constant folding may retain only reachable byte slices.
6. Resolution never falls back to the process working directory or the
   containing source directory.

The current Dialect interface has no filesystem capability and the current
Package target has no confined Workspace. Resource loading is therefore not
implemented.

## Durable package products

Package will eventually own a generated Distribution and its Reader and Writer.
The development format remains `1` and has no backwards compatibility
requirement before release.

A future durable product may retain selected semantic and terminal entries, but
it must not serialize process addresses, parser Cursors, borrowed filesystem
handles, or target caches as semantic truth.

No Distribution, archive codec, package loader, package writer, or source free
semantic restoration exists in the current Package target.

## Current evidence boundary

The current tree provides the following implemented surfaces.

1. TTX provides lexical and closed semantic targets.
2. Language provides shared Comment and Dialect parsers.
3. Language provides the stateful Dialect and Monograph interfaces.
4. Environment provides Workspace installation, dispatch, retention, and
   authored source name lookup.
5. Package provides Dependency, Source, and Monograph models with an incomplete
   interpreter scaffold.
6. Library provides language contracts and scalar Types.
7. Library and Shader provide CPU and SPIR V instruction assemblers.
8. Linker provides source independent linking machinery.

That inventory does not prove complete parsing for Library, App, Scene, Render,
Shader, or Foreign. It also does not prove CPU semantic lowering, package
filesystem confinement, resource folding, durable package encoding, runtime
execution, or source free restoration.

A fixture, README, target build, or test written beside an implementation is not
an independent semantic oracle.
