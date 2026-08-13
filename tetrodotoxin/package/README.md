# Package

Package gives Tetrodotoxin sources stable names, pinned dependencies, confined
resources, and durable build products. It fills a role similar to a package
manifest and compiled module cache, but it never derives identity from a folder
layout.

Authors declare source routes and dependency versions explicitly. A source can
then move to another file without changing the name used by the rest of the
program. The same Package can also be restored from an Archive when the original
source is not available.

Package support is part of the Tetrodotoxin toolchain. A Workspace interpreting
one standalone source does not need to install or use the Package Dialect. A
package compilation, resource request, or Archive restoration installs Package
with the languages used by that operation.

Canonical grammar reference: [Package.g4](grammar/Package.g4).

## Manifest

A Package source begins with the common Dialect envelope and contains
dependencies followed by source bindings:

```ttx
// Scene Lifetime package.
dialect : Package;

resolve Math : Perimortem.Math = "1.0";
resolve Graphics : Perimortem.Graphics = "1.0";
resolve System : Perimortem.System = "1.0";

source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
source Main from "main.ttx";
```

Dependencies are optional and precede Sources. Every Package contains at least
one Source. Local dependency names and Source routes share one Package context,
so each authored route must be unique.

## Dependencies

`resolve` records three facts:

```ttx
resolve LocalName : External.Package.Identity = "Major.Minor";
```

- `LocalName` is the Alias used inside this Package and follows Type spelling.
- `External.Package.Identity` is the durable Package coordinate.
- `Major.Minor` is the pinned version.

Dots inside the external Package identity are manifest coordinate syntax, not
Library Address access. Package selection uses the complete identity and pinned
version. It does not infer a dependency from a filesystem location.

The local Alias participates in ordinary contextual access:

```ttx
Graphics::Image
System::Terminal
```

Each `::` step asks the selected context for the next object. A Type position
requires the route to end at a Type, while Package and `using` declarations
require the kind of context they can import. Package does not convert these
different objects into one common Package Type.

## Sources

`source` binds a semantic route to one path beneath the Package root:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
```

`Scenes::Splash` is the semantic route queried by other sources. It resolves
through the Package context to the retained source binding. The quoted path
only locates bytes. A filename, directory name, or manifest order never creates
a semantic name implicitly.

Source routes use Type-style segments joined by `::`. Library Type positions
and `using` declarations follow the route and then check that it names the kind
of object they require.

Paths are normalized relative to the opened Package root. Empty, rooted,
escaping, or invalid paths are rejected. Two authored paths that normalize to
the same route identify the same input and therefore cannot declare two Sources.

## Package context

The Package Monograph exposes dependencies and Sources through TTX Aliases. A
Source Alias points to the Monograph produced by that source's language. A
dependency Alias points to the restored Package context.

Contextual lookup returns those retained identities. It does not copy Library
Types, App lifecycle facts, or Shader declarations into a separate Package
model.

Members local to a Package remain there unless another language
explicitly imports or publishes them.

## Embedded resources

An embedded resource operand names bytes beneath the source Package root:

```ttx
$[resources/icon.png]
$[resources/table.bin]:[0, 64]
```

Resource acquisition follows these rules:

1. Reads remain confined to the opened Package root.
2. Absolute and escaping routes are rejected.
3. Empty content is a successful Resource rather than a read failure.
4. Equivalent normalized routes return the same retained result.
5. Resolution never falls back to the process working directory or the
   containing source directory.

Package returns a Resource containing stable bytes without assigning them
language meaning. The consumer assigns meaning. Library may construct a Bytes
Constant, Shader may construct shader data, and another Dialect may define
another interpretation.

A recognized request returns either one retained Resource or an Error defined by
Package. The result says whether Package acquired the requested bytes. It does
not decide how another language represents or uses them.

The logical path remains confinement and diagnostic data. It never becomes a
Source name or exported semantic identity.

## Archive

A Package Archive lets Tetrodotoxin rebuild a Package in a new Workspace
without reading and parsing the original source. It stores durable language
facts, not a copy of process memory.

Package owns the Archive because it already knows the Package identity,
dependencies, members, languages, and native artifacts that belong together.
Each language owns the data for its own members. Linker continues to own native
object and executable bytes.

It contains:

1. Package identity and version
2. ordered dependency requests
3. the selected Archive profile
4. ordered member names, Dialect names, and language-owned member data
5. ordered native artifact identifiers
6. exported semantic routes with artifact and symbol locators

Package arranges these records but does not interpret language-owned member
data.

Package admits two profiles:

- `Complete` stores public and private declarations together with executable
  bodies. It can be restored and compiled again without source.
- `Interface` stores public Types, Layouts, Fields, Callable signatures,
  constants, ABI requests, relationships, and compiled artifact locations. It
  leaves executable bodies out.

The selected profile also applies to child layers. For example, a Complete
Scene contains Complete data for its Library child. The outer language stores
the child section, while the child's language reads and checks it.

Neither profile stores parser state, temporary caches, compiler IR, live
objects, or process addresses. Debug symbols and source mapping are separate
outputs. A language that is always read from source does not need to support
Archives.

Archive bytes are not live TTX objects. Name lookup, Type checks, and Layout
fitting become available only after Environment restores the members and
finishes the new Workspace.

## Repository selection

A Repository maps explicit build declarations to Package products. Language
selection uses Package identity and version. Native selection uses an
artifact identifier declared by the selected Archive.

Repository selection does not scan directories or derive identity from paths.
Archive locations, native artifact locations, and output routes remain explicit
declarations. A semantic Archive can be selected without reading any native
artifact.

Output routes are relative, normalized, nonescaping paths. Archive and native
outputs share the Package identity, version, and artifact key while remaining
different product kinds.

## Restoration

Environment creates the Package before it restores any member. This gives every
language the same context for imports and resources. Scene and Shader pass that
context to their child layers. The Workspace supplies the installed language
dependencies separately.

Every restored member goes through normal linking and finalization. If a child
layer fails, its outer member fails as well. Nothing is published until the
complete Package is valid.

Restoration creates new objects. They must expose the same names, Types,
relationships, ordering, Layout behavior, and language facts promised by the
Archive. Their memory addresses and internal storage may differ, and old
references are never revived.

LLVM IR, object modules, and executables are compiled outputs. They cannot
replace a language's Archive data because compilation has already discarded
facts that matter to the source language. The Archive may name native artifacts
and symbols, but Linker and the selected compiler still produce their bytes.

See [Environment](../environment/README.md) for Workspace import and
[Library](../library/README.md) for `using` and Resource consumption. The
[standard packages](../../packages/ttx/README.md) are ordinary Package products
that apply these contracts.
