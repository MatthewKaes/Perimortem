# Package

Package is the reproducible unit that connects Tetrodotoxin source names, pinned
dependencies, confined resources, semantic Archives, and native artifacts. It
combines a concrete top level Dialect with the services that acquire and retain
the inputs named by that Dialect. It overlaps with a package manifest and a
compiled module cache, but its identity comes from explicit semantic
coordinates rather than a directory convention.

Use Package when source files need stable names across moves, dependency and
artifact selection must be deterministic, or a completed semantic program must
be restored without source. This explicit model asks authors to declare routes
and versions rather than relying on directory discovery. A persistent Dialect
must also define the payload that reconstructs its own semantic facts.

Package support is part of the Tetrodotoxin toolchain. A Workspace interpreting
one standalone source does not need to install or use the Package Dialect. A
package compilation, resource request, or Archive restoration installs Package
with the concrete Dialects named by that transaction.

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

* `LocalName` is the Alias used inside this Package and follows Type spelling.
* `External.Package.Identity` is the durable Package coordinate.
* `Major.Minor` is the exact pinned version.

Dots inside the external Package identity are manifest coordinate syntax, not
Library Address access. Package selection uses the complete identity and pinned
version. It does not infer a dependency from a filesystem location.

The local Alias participates in ordinary contextual access:

```ttx
Graphics::Image
System::Terminal
```

Each `::` step asks the selected Abstract context for the next identity. A Type
position proves the final Type, while Package and `using` grammar prove the
final context they require. Package, Monograph, Alias, source, and Type
contexts remain their exact categories rather than being converted into a common
Package Type.

## Sources

`source` binds a semantic route to one path beneath the Package root:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
```

`Scenes::Splash` is the semantic route queried by other sources. It resolves
through the Package context to the retained source binding. The quoted path
only locates bytes. A filename, directory name, or manifest order never creates
a semantic name implicitly.

Source routes use segments with Type spelling joined by `::`. They participate
in the same owner directed contextual resolution used by Library Type positions
and `using` routes. Each consumer proves the category it requires.

Paths are normalized relative to the opened Package root. Empty, rooted,
escaping, or invalid paths are rejected. Two authored paths that normalize to
the same route identify the same input and therefore cannot declare two Sources.

## Package context

The Package Monograph exposes dependency and Source bindings as exact TTX Alias
edges. A successful Source Alias targets the Monograph produced by that source's
own Dialect. A dependency Alias targets the restored Package context.

Contextual lookup returns those retained identities. It does not copy Library
Types, App lifecycle facts, or Shader declarations into a separate Package
model.

Members local to a Package remain there unless another language
explicitly imports or publishes them.

## Embedded resources

An embedded resource operand names bytes beneath the exact source Package root:

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

A recognized request resolves to either one retained Resource or one owner
specific Error identity. This is an owner directed contextual transaction, not
Layout fitting. The result says whether Package acquired the requested bytes.
It does not decide how a consuming language represents or uses them.

The logical path remains confinement and diagnostic data. It never becomes a
Source name or exported semantic identity.

## Archive

A Package Archive contains enough Package and Dialect reconstruction facts to
rebuild a Package's semantic graph in a fresh Workspace without reparsing
source. It is a reconstruction format rather than a memory image.

The Archive avoids source acquisition, lexing, and parsing during restoration.
It still requires complete format validation and the ordinary Workspace
completion barriers before the reconstructed graph can be published.

Archive is the canonical semantic Terminal product for Tetrodotoxin use without
source. Its Terminal role places it outside the live TTX graph. Package owns
this format because it already knows the Package identity, dependencies,
members, Dialect names, and native locators that bind the transaction together.
Linker continues to own native object bytes.

Live semantic identity depends on a Workspace and its concrete language owners.
The Archive records the stable Package and Dialect facts that a new Workspace
needs to construct a graph with the observable relationships defined by those
owners. It carries reconstruction facts rather than live graph identities or
runtime state.

It contains:

1. exact Package identity and version
2. ordered dependency requests
3. ordered semantic member names, Dialect names, and opaque Dialect payloads
4. ordered native artifact identifiers
5. exported semantic routes with artifact and symbol locators

Each concrete Dialect owns the payload needed to construct a new Monograph.
Package owns the envelope and relationships between records without
interpreting those payloads.

Persistence is optional for a Dialect. A persistent Dialect defines and
validates a complete reconstruction payload. A Dialect that is always
interpreted from source does not need an Archive payload.

A payload may be much smaller than a memory image because it records only the
owner facts needed to reproduce the observations promised by the format.
Compactness is a useful property of the format, not the persistence contract.

The Archive bytes are not themselves semantic identities. A caller cannot ask
them to resolve a name, prove a Type, or fit a Layout. Those operations become
available only after Package validates the envelope. Environment then selects
each installed Dialect, asks it to construct a new Monograph from the member
payload, and completes the new Workspace.

## Repository selection

A Repository maps explicit build declarations to Package products. Semantic
selection uses exact Package identity and version. Native selection uses an
artifact identifier declared by the selected Archive.

Repository selection does not scan directories or derive identity from paths.
Archive locations, native artifact locations, and output routes remain explicit
declarations. A semantic Archive can be selected without reading any native
artifact.

Output routes are relative, normalized, nonescaping paths. Archive and native
outputs share the Package identity, version, and artifact key while remaining
different product kinds.

## Restoration

Environment restores the Package root from Archive metadata, selects each
installed Dialect named by a member record, and asks that Dialect to construct a
new Monograph from its payload. Restored Monographs then pass through the same
link and finalize lifecycle as authored source.

Restoration creates fresh process objects. Equivalent restoration reproduces
every public observation promised by the Archive. Those observations include
exact names, categories, represented identity relations, semantic edges,
ordering, Layout behavior, completion, and concrete Dialect facts. Internal
graph shape and process addresses may differ. An old Reference is never
recovered.

LLVM IR, an object module, and an executable are separate target Terminal
products. They cannot replace a Dialect payload because target lowering has
discarded language semantic facts. The Archive may retain native artifact and
symbol locators, but the native bytes remain products of Linker and the selected
compiler path.

See [Environment](../environment/README.md) for Workspace import and
[Library](../library/README.md) for `using` and Resource consumption. The
[standard packages](../../packages/ttx/README.md) are ordinary Package products
that apply these contracts.
