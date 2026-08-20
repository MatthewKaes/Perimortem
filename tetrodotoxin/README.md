# Tetrodotoxin

Tetrodotoxin is the reference host and compiler framework for TTX. It is
designed for projects that contain several purpose-specific languages.
Package manifests, reusable CPU libraries, applications, scenes, render
interfaces, shaders, and foreign declarations can refer to one another while
each language keeps the model that fits its job.

The languages share Types, values, and relationships through TTX instead of
being squeezed into one universal syntax tree. General tools use the shared TTX
surface, while language-aware tools can still see the richer details owned by a
particular language.

Five terms describe the architecture:

- **TTX** is the shared vocabulary used to describe program objects.
- A **Dialect** gives one source language its grammar and meaning.
- A **Monograph** is the retained result of one source. It may include child
  layers supplied by languages it builds on.
- A **Workspace** keeps related Monographs and their references alive together.
- A **Terminal product** is a finished output, such as LLVM IR, SPIR-V, an
  object module, editor data, or a Package Archive.

TTX remains independent of the host. Tetrodotoxin supplies the concrete
languages, Workspace lifetime, Package operations, compilers, linkers, and
application policy that turn its small vocabulary into a toolchain.

## Is Tetrodotoxin a fit for my project?

Tetrodotoxin is aimed at projects where several languages need to contribute to
one program. This includes compiler research, integrated application
toolchains, editor services, and systems that must rebuild language information
from an Archive when source is unavailable.

The design is most useful when preserving each domain's model matters more than
providing one generic declaration tree. A Package member, a Library Type, an
App lifecycle, a Scene signal, and a Shader Stage can meet through TTX without
becoming variants of the same record.

Adding a Dialect requires more than adding syntax. The Dialect must create its
language objects, help complete the Workspace, answer useful questions, and
report errors. If it supports Package Archives, it also defines the information
needed to rebuild its Monograph.

A conventional frontend is usually simpler when one language owns the whole
program. LLVM focuses on optimization and machine-code generation, MLIR on
extensible intermediate representations, and Clang on C and C++ compatibility.
Tetrodotoxin focuses on sharing language-level meaning before those lower-level
representations are chosen.

## A family of sources

Every top-level source begins with authored Documentation and selects the
Dialect that owns its body:

```ttx
// A reusable Library source.
dialect : Library;

public twice : func = [.value : Unsigned_64] -> Unsigned_64 {
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
the names other sources query. Moving a file does not silently change the
semantic route used by the program.

## Languages meet through explicit queries

Three questions recur when Tetrodotoxin languages meet:

1. Which Addressable does an applicable Layout expose?
2. Which object does a context route reach, and is it the kind of object the
   consuming language expects?
3. Which Callable does a language select, and how do values fit its
   parameter and result Layouts?

These questions preserve the existing TTX identities and edges. Host-neutral
Abstract supplies total query hooks, but it does not prescribe receiver roles,
visibility, scalar families, or defaults. Library refines Type and Addressable
once to own those CPU-language rules without creating a second semantic
identity or imposing them on another Dialect's Types.

Together, these objects form the live program in a Workspace. Each Dialect
contributes objects from its own language. Linking and finalization connect them
and answer routes that could not be resolved while the source was still being
read. An object that has already been returned never changes identity.

Library, for instance, expresses the three questions through Access operators:

```ttx
packet.width                   // select an Addressable from a Layout
Graphics::Image                // resolve a Type through context
packet -> resize(new_width)    // select and invoke a Callable
```

Keeping each Type system with its Dialect lets every language use rules suited
to its own domain. The operators retain TTX edges while Library's Type protocol
owns lookup, visibility, mutation, default construction, and Static and Self
receiver policy.

## Dialects

Tetrodotoxin provides several top-level Dialects and one embedded language
fragment as building blocks for richer domain-specific solutions:

* [Package](package/README.md) declares dependencies, names source members,
  provides confined resources, and defines durable Archives.
* [Library](library/README.md) defines reusable CPU Types, values, functions,
  expressions, Structs, Objects, and Enumerations.
* [App](app/README.md) describes startup and application lifecycle.
* [Scene](scene/README.md) describes scene state, signals, hosted graphics, and
  lifecycle roles.
* [Render](render/README.md) declares contracts for values and stages consumed
  by rendering.
* [Shader](shader/README.md) defines how GPU stages implement Render contracts.
* [Foreign](foreign/README.md) embeds an external ABI surface inside a source
  that supports CPU execution.

Each top-level Dialect owns its source grammar and constructs concrete
Monographs directly. Foreign is an embedded language fragment rather than an
installed top-level Dialect.

Some Dialects build on layers from another Dialect. Scene contains one Library
layer for its CPU state and functions. Shader contains a Library layer for CPU
helpers and a Render layer for GPU data. These children come from the same
Dialect instances already installed in the Workspace, so grammar and installed
dependencies stay consistent. Each child Monograph still owns its concrete
Types and canonical Generic materializations. The Dialect stores no semantic
identity.

Dependencies always point toward the lower-level language. Scene depends on
Library. Shader depends on Library and Render. App depends on Library and Scene.
The reverse dependencies are not allowed, and Shader does not depend on Vulkan.
The build graph follows the same direction.

[Graphics](graphics/README.md) is a language neutral runtime composition
boundary rather than a Dialect. It collects exact hosted Scene state into
stable frame submissions while Render, Shader, and the selected backend retain
their own semantics.

The [standard packages](../packages/ttx/README.md) provide ordinary Package and
Library definitions for Memory, Math, System, and Graphics. They are linked by
authored Package dependencies and do not become compiler builtins.

The repository publishes canonical grammar references for authored language
shape and parse order. The complete source entries are
[Package](package/grammar/Package.g4),
[Library](library/grammar/Library.g4), [App](app/grammar/App.g4),
[Scene](scene/grammar/Scene.g4), [Render](render/grammar/Render.g4), and
[Shader](shader/grammar/Shader.g4).
[Foreign](foreign/grammar/Foreign.g4) defines an embedded fragment.
[Tetrodotoxin](language/grammar/Tetrodotoxin.g4) contains grammar fragments
shared by several Dialects, and the
[TTX lexical grammar](../ttx/grammar/TTXLexer.g4) records their common
spellings.

These files are descriptive references. The toolchain does not generate or run
its parsers from them.

## One Workspace lifetime

Tetrodotoxin's Workspace manages several source languages in one shared
lifetime. A direct source is one complete Workspace transaction:

```text
source bytes
-> TTX Tokens
-> selected Dialect
-> direct Cursor, Associations, Documentation, Anchor, and context inputs
-> optional parse-valid Monograph in the source Arena
-> link with the source Cursor
-> finalize with the source Cursor
-> retain the Arena and its Monograph to Associations association
-> publish the completed Monograph
-> compilation, tooling, or durable output
```

Failure at any stage releases the local Arena, and Workspace never accumulates
an invalid source for later validation. A Package manifest supplies one fixed
Source table. Workspace interprets all of those members, links every member
before finalizing any member, and publishes only the completed Package root.
Workspace retains each successful Monograph's source transaction Arena
containing the authored bytes, source-backed values, Tokens, semantic graph,
and immutable Associations index. The operation Cursor completes before
publication and is not exposed as completed source state. Tools ask Workspace
for the exact completed Associations index. A compiler receives the exact
source facts and caller owned textual error sink required by its transaction.

Once the Workspace is complete, tools and compilers can use the same facts
without translating the program into another language's model. Library compiles
CPU layers with LLVM. Shader and Render produce
SPIR-V for the GPU. Linker combines native objects into ELF or PE programs.
These finished outputs no longer need the Workspace.

## Leaving the graph

Tetrodotoxin calls an output that no longer needs the live Workspace a Terminal
product. Linker owns native objects and executables, Shader owns GPU modules,
and Package owns the semantic Archive.

Puffer is the user-facing compiler driver and LSP application shell. It
coordinates each requested product and presents the result while Library,
Shader, Linker, and Package retain ownership of their formats and semantics.

Compiled products keep the facts needed by their next consumer. LLVM IR,
SPIR-V, debug data, and native objects cannot rebuild the complete language
model that produced them. A Package Archive serves a different purpose. It
keeps enough information for Package to rebuild its completed root inside a
fresh Workspace without source.

### Semantic reconstruction in durable formats

TTX queries normally use a live graph, but builds and distributed Packages need
a durable form. Package Archive records enough information to build a fresh
Workspace without reading the original source again.

The Complete profile keeps public and private declarations together with the
bodies needed to compile them again. The Interface profile keeps the public
contract and the locations of compiled artifacts, but leaves executable bodies
out. The same profile applies to child layers such as the Library layer inside
a Scene.

Restoration creates new objects rather than copying process memory. The new
graph must expose the same names, Types, relationships, Layouts, ordering, and
language behavior promised by the Archive. It does not need to use the same
addresses or internal data structures. Live runtime state and source-to-debug
mapping are not stored in either profile.

The [Tetrodotoxin design](tetrodotoxin_design.md) explains the Terminal and
reconstruction contract in detail.

## Packages and resources

The Tetrodotoxin toolchain always includes Package support. A Workspace that
interprets one standalone source does not need to install or use the Package
Dialect. Package enters a Workspace when a request composes a Package, acquires
its resources, or restores an Archive.

Package owns reproducible dependency selection, semantic source names,
confined Storage, resource acquisition, Archives, and Repository selection. It
keeps host paths and acquisition policy outside the consuming language.

Package paths stay beneath one opened Package root. An embedded operand such
as `$[resources/table.bin]` asks the source Package for retained bytes. The
consuming Dialect decides what those bytes mean, and an empty file is still a
valid Resource. A recognized request returns either the Resource or an Error
defined by Package.

Packages may also be distributed as Archives used without source. Package owns
the envelope and dependency inventory while each persistent Dialect owns the
payload needed to construct new Monographs.

## Where to go next

| I want to                                        | Start with                                                                      |
| ------------------------------------------------ | ------------------------------------------------------------------------------- |
| Understand the shared graph precisely            | [TTX design](../ttx/ttx_design.md) and [TTX semantics](../ttx/ttx_semantics.md) |
| Understand the host architecture and tradeoffs   | [Tetrodotoxin design](tetrodotoxin_design.md)                                   |
| Add another source language                      | [Language](language/README.md)                                                  |
| Embed source interpretation and graph lifetime   | [Environment](environment/README.md)                                            |
| Package or restore semantic programs             | [Package](package/README.md)                                                    |
| Define reusable CPU code                         | [Library](library/README.md)                                                    |
| Use the standard Math, System, and Graphics APIs  | [Standard packages](../packages/ttx/README.md)                                  |
| Work with GPU stages                             | [Render](render/README.md) and [Shader](shader/README.md)                       |
| Produce native objects and executables            | [Linker](linker/README.md)                                                      |
| Use the command line or editor application shell | [Puffer](../puffer/README.md)                                                   |
