# Tetrodotoxin

Tetrodotoxin is the reference host and compiler framework for TTX. It is
designed for source trees that contain several purpose specific languages.
Package manifests, reusable CPU libraries, applications, scenes, render
interfaces, shaders, and foreign declarations can refer to one another while
keeping the semantic model appropriate to each language.

This is useful when the languages in one product share identities and value
shape but do not share one useful AST. Generic tools can use their common TTX
contracts, while language specific tools continue through the concrete
language. The tradeoff is that each language must define the semantics,
completion rules, and Diagnostics that only it understands. A persistent
language must also define its durable reconstruction facts.

Five terms describe the architecture:

* **TTX** is the shared vocabulary for semantic identity and queries.
* A **Dialect** gives one source language its grammar and meaning.
* A **Monograph** is the retained semantic result of one source.
* A **Workspace** gives related Monographs and their borrowed edges one
  lifetime.
* A **Terminal product** has left that lifetime, such as LLVM IR, SPIR-V, an
  object module, editor data, or a Package Archive.

TTX remains independent of the host. Tetrodotoxin supplies the concrete
languages, Workspace lifetime, package transactions, compilers, linkers, and
application policy that turn its small vocabulary into a toolchain.

## Is Tetrodotoxin a fit for my project?

Tetrodotoxin is aimed at projects where several language shaped domains must
compose in one program and several consumers need to agree on their live
meaning. Compiler research, integrated application toolchains, editor services,
and systems that need source independent semantic restoration are natural use
cases.

The design is most useful when preserving each domain's model matters more than
providing one generic declaration tree. A Package member, a Library Type, an
App lifecycle, a Scene signal, and a Shader Stage can meet through TTX without
becoming variants of the same record.

That choice has costs. Adding a Dialect requires more than adding syntax. The
Dialect must construct its semantic objects, participate in Workspace
completion, expose useful queries, and report Diagnostics. When it participates
in Package Archive persistence, it must also define the payload from which it
can reconstruct its Monograph. Generic tools see the common TTX surface and use
the concrete Dialect for richer language details.

A conventional frontend is usually simpler when one language owns the entire
program. **LLVM is the stronger starting point** when the project begins with
lowered computation, optimization, and code generation. **MLIR is the stronger
fit** when extensible operation rewriting is the central abstraction. **Clang
is the stronger fit** when C or C++ compatibility and mature language tooling
are the product.

## A family of sources

Every top level source begins with authored Documentation and selects the
Dialect that owns its body:

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
the names other sources query. Moving a file does not silently change the
semantic route used by the program.

## Languages meet through explicit queries

Three semantic questions recur when Tetrodotoxin languages meet:

1. Which Addressable does an applicable Layout expose?
2. Which identity does an owner directed contextual route reach, and does it
   satisfy the category required by the consuming language?
3. Which Callable does a concrete language select, and how do values fit its
   parameter and result Layouts?

These questions use existing TTX contracts. Tetrodotoxin does not add another
Type, Addressable, or Callable category. Each concrete language assigns the
grammar and meaning that turns a shared contract into one of its operations.

Together the concrete objects form Tetrodotoxin's live multi domain semantic
IR. A Workspace owns one semantic island while each Dialect contributes objects
from its localized domain. Linking and finalization can connect those objects
and answer unresolved queries, but they never replace an identity that was
already returned successfully.

Library, for instance, expresses the three questions through Access operators:

```ttx
packet.width                   // select an Addressable from a Layout
Graphics::Image                // resolve a Type through context
packet -> resize(new_width)    // select and invoke a Callable
```

Keeping each complete type system with its Dialect lets every language use the
strongest semantic model for its domain.
These operators share TTX contracts while each Dialect keeps its own lookup,
visibility, overload, mutation, and evaluation rules. That is the practical
boundary between composition and a universal type system.

## Dialects

Tetrodotoxin provides several top level Dialects and one embedded language
fragment as building blocks for richer domain specific solutions:

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

Each top level Dialect owns its source grammar and constructs concrete
Monographs directly. Foreign is an embedded language fragment rather than an
installed top level Dialect.

[Graphics](graphics/README.md) is a language neutral runtime composition
boundary rather than a Dialect. It collects exact hosted Scene state into
stable frame submissions while Render, Shader, and the selected backend retain
their own semantics.

The [standard packages](../packages/ttx/README.md) provide ordinary Package and
Library definitions for Math, System, and Graphics. They are linked by authored
Package dependencies and do not become compiler builtins.

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
lifetime. It interprets a related group of authored sources and invokes their
Dialects through the same completion contract:

```text
source bytes
-> TTX Tokens
-> selected Dialect
-> interpret and retain authored identities
-> link contextual routes across the complete source group
-> finalize completed language facts
-> compilation, tooling, or durable output
```

Interpretation preserves authored identities even while some routes remain
unanswered. Linking connects those routes after the complete source group is
known. Finalization performs work that requires every linked declaration to be
available. Publication makes the group visible only after the complete
transaction succeeds.

Once the live graph is completed, tools can ask dynamic semantic questions and
Terminal producers can consume the same facts without translating the program
into another language's model. Library can lower completed CPU facts while
Shader lowers completed GPU facts. The queries remain live graph operations.
Only the independently consumable output crosses the Terminal boundary.

## Leaving the graph

When a compiler or tool produces an output that no longer needs the live
Workspace, Tetrodotoxin calls that output a Terminal product. Linker, for
example, owns native objects and executables. Shader owns GPU modules. Package
owns the semantic Archive.

Puffer is the user facing compiler driver and LSP application shell. It
coordinates each requested product and presents the result while Library,
Shader, Linker, and Package retain ownership of their formats and semantics.

Target Terminal products preserve the facts needed by their next consumer.
LLVM IR, SPIR-V, debug data, and native objects are not complete records of the
language graph that produced them. Package Archive serves a different purpose.
It keeps Package and Dialect reconstruction facts that let Environment
construct a fresh Workspace without source and run the ordinary link, finalize,
and publication barriers.

### Semantic reconstruction in durable formats

TTX semantic queries operate on a live graph, but plenty of useful workflows do
not fit in one process. Incremental builds, distribution, and debugging may all
need source independent semantic input later without retaining the original
Workspace.

Package Archive is Tetrodotoxin's canonical semantic reconstruction format. It
records sufficient Package and Dialect facts to construct a fresh graph. Its
records contain no live graph identities or runtime state.

Two graphs do not need the same memory representation to be equivalent. The
restored graph must reproduce every public observation promised by the format,
including names, categories, represented identity relationships, semantic
edges, order, Layout behavior, completion, and concrete Dialect facts. The
supporting graph shape remains opaque.

A reconstruction payload can be much smaller than a memory image because it
records only the owner defined facts needed to reproduce those observations.
Compactness is a format benefit rather than a semantic requirement. A
persistent Dialect participates by defining and validating a complete
reconstruction payload. Other Dialects do not have to be persistent.

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

Package paths are confined to one opened package root. An embedded operand such
as `$[resources/table.bin]` asks the source Package for retained bytes. The
consuming Dialect decides what those bytes mean, and empty content remains a
valid resource. A complete request resolves to a retained Resource or an owner
specific Error. It is a contextual transaction rather than Layout fitting.

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
