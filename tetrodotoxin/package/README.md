# Package

Package gives a group of Tetrodotoxin sources one durable identity. Its manifest
pins dependencies, assigns semantic names to source files, confines resource
paths, and describes the semantic and native products that can be restored
without source.

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

- `LocalName` is the Type-shaped Alias used inside this Package.
- `External.Package.Identity` is the durable Package coordinate.
- `Major.Minor` is the exact pinned version.

Dots inside the external Package identity are manifest coordinate syntax, not
Library Address access. Package selection uses the complete identity and pinned
version; it does not infer a dependency from a filesystem location.

The local Alias participates in ordinary contextual access:

```ttx
Graphics::Image
System::Terminal
```

Each `::` step asks the selected Abstract context for the next identity. A Type
position proves the terminal Type, while Package and `using` grammar prove the
terminal context they require. Package, Monograph, Alias, source, and Type
contexts remain their real categories rather than being converted into a common
Package Type.

## Sources

`source` binds a semantic route to one path beneath the Package root:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
```

`Scenes::Splash` is the identity used by other source. The quoted path only
locates bytes. A filename, directory name, or manifest order never creates a
semantic name implicitly.

Source routes use Type-shaped segments joined by `::`. They participate in the
same contextual TypeAccess model used by Library Types and `using` routes.

Paths are normalized relative to the opened Package root. Empty, rooted,
escaping, or invalid paths are rejected. Two authored paths that normalize to
the same route identify the same input and therefore cannot declare two Sources.

## Package context

The Package Monograph exposes dependency and Source bindings as real TTX Alias
edges. A successful Source Alias targets the Monograph produced by that source's
own Dialect; a dependency Alias targets the restored Package context.

Contextual lookup returns those retained identities. It does not copy Library
Types, App lifecycle facts, or Shader declarations into a separate Package
model.

Package-local members remain inside their Package unless another language
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

Package returns a language-neutral Resource containing stable bytes. The
consumer assigns meaning: Library may construct a Bytes Constant, Shader may
construct shader data, and another Dialect may define another interpretation.

The logical path remains confinement and diagnostic data. It never becomes a
Source name or exported semantic identity.

## Archive

A Package Archive is the durable semantic product used to restore a Package
without authored source. It contains:

1. exact Package identity and version;
2. ordered dependency requests;
3. ordered semantic member names, Dialect names, and opaque Dialect payloads;
4. ordered native artifact identifiers;
5. exported semantic routes with artifact and symbol locators.

Each concrete Dialect owns the payload needed to restore its Monograph. Package
owns the envelope and relationships between records without interpreting those
payloads.

An Archive contains no live process address, parser position, filesystem
handle, source path as semantic identity, target cache, or Linker object bytes.

## Archive Format 1

Format 1 uses fixed-width little-endian integers. The file begins with a twelve
byte header:

| Offset | Width | Value |
| --- | ---: | --- |
| 0 | 4 | ASCII `TTXA` |
| 4 | 2 | format value `1` |
| 6 | 2 | reserved flags `0` |
| 8 | 4 | complete body size |

The body is a tagged field envelope. Each field begins with a 16-bit tag,
16-bit flags, and 32-bit payload size. Flag bit zero marks a required field;
other bits are reserved.

Six fields are required once each in this order:

| Tag | Payload |
| ---: | --- |
| 1 | Package identity string |
| 2 | 16-bit major and 16-bit minor version |
| 3 | dependency list |
| 4 | member list |
| 5 | native artifact list |
| 6 | export list |

Strings and opaque payloads begin with a 32-bit byte count. Lists begin with a
32-bit entry count, and each entry begins with a 32-bit record size. Every field
and record is consumed exactly; missing, repeated, reordered, truncated, or
trailing data is invalid.

Known fields carry the required bit. An unknown required field is invalid. An
unknown optional field may be skipped when its complete payload lies within the
declared body.

Package identities use dot-separated Type-shaped segments. Member routes and
dependency aliases use `::`-separated Type-shaped segments. Dialect names use
one Type-shaped segment. Package and dependency version `0.0` is reserved.

Canonical encoding preserves declared list order and produces identical bytes
for equivalent Archive facts.

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
installed Dialect named by a member record, and asks that Dialect to recreate
its semantic payload. Restored Monographs then pass through the same link and
finalize lifecycle as authored source.

See [Environment](../environment/README.md) for Workspace import and
[Library](../library/README.md) for `using` and Resource consumption.
