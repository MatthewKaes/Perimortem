# Tetrodotoxin

Tetrodotoxin is the reference host for TTX. It combines a small shared semantic
vocabulary with concrete source Dialects for packages, reusable CPU code,
applications, scenes, rendering, shaders, and foreign interfaces.

TTX describes identities and Layouts. Tetrodotoxin gives those facts language
meaning, keeps them alive in a Workspace, resolves packages, and passes
completed programs to compilers and linkers.

## A source family

Every source begins with required Documentation and selects the Dialect that
owns its body:

```ttx
// A reusable Library source.
dialect : Library;

public func twice[.value : Unsigned_64] -> Unsigned_64 {
  return value * 2;
}
```

A Package gives source files semantic names independently from their paths:

```ttx
// The package manifest.
dialect : Package;

resolve System : Perimortem.System = "1.0";
source Utilities from "utilities.ttx";
source Main from "main.ttx";
```

The filename locates input beneath the package root. `Utilities` and `Main` are
the identities other sources query.

## Access is explicit

TTX punctuation selects separate semantic domains throughout Tetrodotoxin:

```ttx
packet.width                   // Addressable in a named Layout
Graphics::Image                // Type through contextual resolution
packet -> resize(new_width)    // Callable invocation
packet.[width, height]         // named selection and repacking
access[index]                  // optional reference access
bytes:[index]                  // safe element value
bytes:[0, 64]                  // safe ranged value
```

The distinction remains visible across packages and `using` declarations.
`::` can cross Alias, Package, Monograph, source, and Type contexts without
converting those contexts into Types or Expressions.

## Dialects

- [Package](package/README.md) declares dependencies, names source members,
  provides confined resources, and defines durable Archives.
- [Library](library/README.md) defines reusable CPU Types, values, functions,
  expressions, Structs, Objects, and Enumerations.
- [App](app/README.md) describes startup and application lifecycle.
- [Scene](scene/README.md) describes scene state, signals, children, and
  lifecycle roles.
- [Render](render/README.md) declares render-facing value and stage contracts.
- [Shader](shader/README.md) implements Render contracts for GPU stages.
- [Foreign](foreign/README.md) embeds an external ABI surface inside a
  CPU-capable source.

### Grammar prototypes

Each Dialect provides a ANTLR4 grammer as a prototype source reference for custom parser:

- [Package](package/grammar/Package.g4)
- [Library](library/grammar/Library.g4)
- [App](app/grammar/App.g4)
- [Scene](scene/grammar/Scene.g4)
- [Render](render/grammar/Render.g4)
  [Shader](shader/grammar/Shader.g4)
- [Foreign](foreign/grammar/Foreign.g4)
- [Tetrodotoxin](language/grammar/Tetrodotoxin.g4)

[TTX lexer prototype](../ttx/grammar/TTXLexer.g4) records their common spellings used across the grammer family.

For a formal implementation the shared [Language](language/README.md) explains how a Dialect
produces a Monograph while [Environment](environment/README.md) explains how a
Workspace installs Dialects and retains their results. The rest of the
Tetrodotoxin grammer is build off of those two TTX Abstract Machines.

## Semantic lifecycle

A Workspace interprets a group of sources as one semantic island:

```text
source bytes
-> TTX Tokens
-> selected Dialect
-> retained Monograph
-> contextual resolution
-> link the complete source group
-> finalize completed language facts
-> compilation, tooling, or durable output
```

Interpretation preserves authored identities even when their Type routes are
not complete yet. Linking connects those routes after the source group is
known. Finalization performs work that requires every linked declaration to be
available. A failed group is not published as a completed program.

## Packages and resources

Package paths are confined to one opened package root. An embedded operand such
as `$[resources/table.bin]` asks the source Package for retained bytes; the
consuming Dialect decides what those bytes mean. Empty content remains a valid
resource, and paths never become semantic source names implicitly.

Packages may also be stored as source-free Archives. Package owns the envelope
and dependency inventory while each concrete Dialect owns the payload needed to
restore its own Monographs.

## Tooling boundary

Concrete semantic owners provide completed facts to their consumers. Library
lowering handles CPU code, Shader lowering handles GPU code, and Linker handles
object formats and final native products. Package identity and source semantics
remain separate from target addresses, relocations, and runtime storage.

See [Tetrodotoxin design](tetrodotoxin_design.md) for the host architecture and
[TTX semantics](../ttx/ttx_semantics.md) for the normative shared model.
