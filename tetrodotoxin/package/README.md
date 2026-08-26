# Package

Package gives a connected source graph one public Type surface, identity,
version, confined resource domain, and durable Archive. It does not own a
second table of source or dependency meaning. Sources name the graph edges they
use, Workspace owns the resulting Monographs, and Terminal generation decides
how that graph becomes a product.

Canonical grammar reference: [Package.g4](grammar/Package.g4).

## Source imports

Imports belong to the common source envelope and are available to every
Dialect:

```ttx
public LocalName : alias = source("./local/path.ttx");
public OtherName : alias =
    package(.name = "External.Package", .version = "1.0");
```

Each declaration creates one ordinary TTX Alias in the importing source. A
source import binds that Alias to the imported Dialect's semantic root. A
Package import binds it to the exact restored Package export root. Consumers
then use ordinary context operations:

```ttx
OtherName::PublicType
LocalName -> static_callable()
```

The Monograph retains lifetime and source facts but never enters the authored
route. A root that has no intrinsic authored name receives its local name from
the importing Alias. The same source may therefore be imported under different
local names without adding a naming protocol to Monograph.

Library `using` remains a separate forwarding choice:

```ttx
public System : alias =
    package(.name = "Perimortem.System", .version = "1.0");

using System::Key;
```

`using` forwards the selected context's public names into the current Library
source. It does not copy declarations or create more Aliases. Imports are
established before Library members link, so formatting may place `using` after
the source's import declarations without changing meaning.

## Package source

A Package source is a restricted Library source. Common imports come first;
its body then contains only Type definitions, Aliases, and namespaces used to
manufacture the public joint surface:

```ttx
/// Example package.
dialect : Package;

public VectorSource : alias = source("vector.ttx");

public Vector : alias = VectorSource::Vector;

public Dynamic : namespace {
  public Bytes : alias = VectorSource::Bytes;
}
```

Package accepts no runtime Fields, Functions, or executable statements. Those
belong to ordinary Library, Scene, Shader, App, Render, or another concrete
source. An empty Package body is valid when its imported root is itself the
complete product surface, as in an application Package.

Package identity and version are product coordinates supplied by the terminal
request. A source imports that coordinate explicitly with `package(...)`; no
filesystem path or ambient repository name creates it.

## Workspace graph

Workspace starts with the Package source and walks each `source(...)` Alias.
Every imported file receives its own source transaction Arena, Token stream,
Associations, diagnostics, and concrete Monograph. New source imports extend
the same graph; a `package(...)` import terminates the local walk at one exact
Package identity and version that the terminal has already supplied.

All local paths are resolved relative to the source that authored them. The
shared Path owner canonicalizes `.` and `..` before Storage lookup. Equivalent
spellings therefore reuse one cached file and one semantic source identity:

```ttx
public First  : alias = source("./shared.ttx");
public Second : alias = source("folder/../shared.ttx");
```

Rooted paths and paths that escape the opened Package root are rejected. Source
cycles are rejected before linking. When the graph is acyclic, Workspace links
dependencies before importers and finalizes only after every retained source
has linked without errors. Incomplete graphs remain available to editor
tooling but cannot enter a Terminal producer.

## Embedded resources

Embedded paths are source-relative for the same reason source imports are:
each file is self-contained when it moves with its neighboring assets.

```ttx
$[resources/icon.png]
$[../resources/noise.png]
$[../resources/./table.bin]:[0, 64]
```

The Cursor carries the source's canonical logical path. Embedded parsing asks
the same Path owner used by source imports to resolve and confine the request,
then Package Storage caches the canonical route. Equivalent paths return the
same retained Resource, including when different sources reach it through
different relative spellings. Empty content is a successful Resource. Library
and other Dialects assign meaning to the returned bytes; Package does not.

## Source distribution

A Package installation is selected by exact identity and version. Its physical
directory has one stable shape:

```text
<repository>/<identity>/<major>.<minor>/
  package.ttx
  ...source files and resources...
  contract.txa
  complete.txa
  abi.manifest
  native/
    <artifact>/
      package.a
```

The source portion preserves the complete Package rooted file tree, not only
files whose names end in `.ttx`. Source imports decide which language files join
the semantic graph, while embedded operands decide which images, tables,
generated data, or other files become retained Resources. Keeping both beneath
the same installed root preserves relative paths and Package confinement.

Repository receives the coordinate from the importing source or build request.
A caller may map that exact coordinate to a local source root during
development. Otherwise Repository selects the versioned installed directory.
The mapping grants a physical location to an existing Package key; neither the
directory name nor `package.ttx` manufactures semantic identity or version.

Editor sessions interpret an installed or local source distribution directly.
Builds may instead select the Contract and ABI products beside that source.
Both paths enter the same Workspace owners, which keeps source distribution and
compiled distribution from becoming separate Package models.

## Archive

Archive Format 4 records the completed graph rather than recreating a manifest
table. It contains:

1. Package identity, version, and profile.
2. One restricted Library payload for the Package export surface.
3. One opaque payload for each source Monograph, keyed by its deterministic
   first route from the Package root rather than an intrinsic source name.
4. Every source and Package Import edge with importer, local Alias name,
   target, and exact Package version when applicable.
5. The canonical Resource closure.
6. Native artifact agreements and exported symbol routes.

The older Dependency section remains readable only for Archive Formats 2 and
3. Format 4 Package edges live exclusively in the Import graph, so restoration
does not construct a parallel dependency scope. Workspace restores every
member, binds the recorded Aliases to their real roots, orders the source graph,
and applies the same composition, linking, finalization, and publication
barriers as authored source.

Package arranges the envelope but never interprets a member's opaque payload.
Each persistent Dialect owns its own Complete and Contract representations.
Compiled CPU objects and SPIR-V modules remain Terminal products rather than
semantic Archive state.
