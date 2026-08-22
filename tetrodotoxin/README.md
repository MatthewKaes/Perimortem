# Tetrodotoxin

Tetrodotoxin raises several owned language models into one linked semantic
Workspace, using TTX as their shared graph vocabulary, then derives independent
Terminal products from that completed meaning.

Package manifests, reusable CPU libraries, application policy, scenes, render
contracts, shaders, and foreign interfaces can all describe one program while
keeping the model that fits their own domain. Raising means that their original
identities participate in shared contracts. It does not translate them into a
second universal language model.

Seven terms make the platform easier to navigate:

* **TTX** is the shared vocabulary for source locations, semantic identities,
  Types, values, Layouts, and Callables
* A **Dialect** gives one source language its grammar and meaning
* A **Monograph** is the retained result of one source and may include child
  layers from languages it builds on
* A **Workspace** keeps related Monographs and their references alive together
* A **Package** names the sources, dependencies, resources, and durable facts
  that make a project reproducible
* A **Terminal product** is a finished output such as native code, a GPU module,
  editor data, or a Package Archive
* **Puffer** is the user facing command and editor host for the complete
  Toolchain

## Why build on Tetrodotoxin?

Domain specific languages are most useful when they can preserve the concepts
their authors care about. They become expensive when every language also needs
its own package manager, editor protocol, build graph, compiler shell, and
runtime integration.

Tetrodotoxin separates those concerns. A Dialect owns the grammar and semantic
objects that make its language distinctive, then exposes common TTX contracts
where another language or tool needs to cooperate. A Package member, Library
Type, App lifecycle, Scene signal, and Shader Stage can therefore meet without
becoming variants of one generic declaration record.

Adding a Dialect means owning more than syntax. The Dialect creates its language
objects, contributes to Workspace completion, answers useful queries, and
reports source errors. In return it joins the same Package graph, editor
session, source documentation model, and Terminal production path as the
languages already installed in the Toolchain.

The responsibility split stays explicit:

* A **Dialect** owns its grammar and complete domain meaning.
* **TTX** owns only semantic questions genuinely shared across domains.
* **Workspace** owns lifetime, cross-Dialect linking, completion, and
  publication.
* A **Terminal producer** owns target specific derivation and lowering.

LLVM IR, SPIR-V, Package Archives, editor data, and executables are products of
the completed meaning. None becomes the semantic source of truth for the
languages that produced it.

This design earns its weight when one system contains several semantic domains
or when the same completed program feeds editors, compilers, Packages, and
runtimes. A project with one small language and one output may need less
machinery, while a growing family of languages gains a stable place to meet.

## A family of sources

Every top-level source begins with authored Documentation and selects the
Dialect that owns its body:

```ttx
// A reusable Library source.
dialect : Library;

public twice : func = [.value : U64] -> U64 {
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
Dialect instances already installed in the Workspace's borrowed Toolchain, so
grammar and installed dependencies stay consistent. Each child Monograph still
owns its concrete Types and canonical Generic materializations. The Dialect
stores no semantic identity.

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

The Complete profile keeps the public and private semantic contract. The
Interface profile keeps only the public contract required by dependent
consumers. Neither stores executable bodies. Compiled artifacts carry
execution, while source or a live Workspace remains the input for another
lowering. The same profile applies to child layers such as the Library layer
inside a Scene.

Restoration creates new objects rather than copying process memory. The new
graph must expose the same names, Types, relationships, Layouts, ordering, and
language behavior promised by the Archive. It does not need to use the same
addresses or internal data structures. Live runtime state and source-to-debug
mapping are not stored in either profile.

The [Tetrodotoxin design](tetrodotoxin_design.md) explains the Terminal and
reconstruction contract in detail.

## Packages and resources

Package support is available to a Tetrodotoxin Toolchain but is not installed
implicitly. A Toolchain used only for standalone sources may omit the Package
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
