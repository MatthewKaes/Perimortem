# Puffer

Puffer is how people and build systems enter Tetrodotoxin. The command line can
turn a Package into finished products, while the language server gives an
editor a live understanding of the same sources. A diagnostic, hover, build,
and Archive therefore begin from the same view of the program.

Behind that experience, Puffer assembles the requested languages and opens a
Workspace for them. It coordinates the journey without taking ownership away
from the parts that understand it best. Package still owns reproducible
composition, each Dialect still owns its language, and each Terminal or Linker
still owns the product it creates.

Build systems can use the command interface and editors can use the language
server. A product that needs a different session or interface can embed the
same Tetrodotoxin libraries directly and keep the same language, Package,
compiler, and Archive behavior.

## Application role

Puffer owns process, transport, request selection, editor sessions, and
presentation:

```text
request from editor or command line
-> Puffer session
-> Tetrodotoxin language and package contracts
-> Diagnostics or completed products
-> editor or terminal response
```

Open editor documents may contain unsaved bytes, so Puffer retains their text
and protocol identity for the session. Environment owns Workspace lifetime and
source completion. Package owns package selection and semantic Archives.
Language compilers own their native member products and Linker owns final native
composition. Puffer owns the build transaction that selects and coordinates
those components.

Command-line Package diagnostics present the physical Package root joined to
the source's logical path so terminals can open the reported file directly.
Editor sessions keep that logical path as their stable document key and let the
LSP map it through the retained Package root.

The LSP constructs one Environment Toolchain and lends it to every replacement
Workspace. A Package session walks the selected source graph, recursively loads
each unresolved exact Package Alias through Package Repository, detects active
cycles, and rebuilds the consumer after its dependencies complete. Repository
maps an exact identity and version to either a declared local source root or the
versioned installed layout. One shared Snapshot owner retains editor overlays
and unchanged filesystem bytes across those complete graph replacements. No
standard Package name is injected into an unrelated Package.

An installed source Package uses this layout:

```text
<packages-root>/<identity>/<major>.<minor>/package.ttx
```

The directory also carries imported sources, arbitrary embedded files,
`contract.txa`, `complete.txa`, `abi.manifest`, and target products beneath
`native/<artifact>/`. A local checkout can override one exact coordinate
without changing authored source:

```text
puffer --pipe=<socket-path> \
  --packages-root=<installed-root> \
  --package-source=Example.Math|1.0|/work/Example.Math
```

Package and application builds use the same Repository to acquire a missing
dependency Contract and ABI Manifest. Explicit build inputs remain valid and
take precedence for their exact coordinate, while the installed store lets the
same Puffer command run outside Bazel.

A local source mapping changes source acquisition only. It does not infer a
build request for that Package because artifact targets, native providers, and
selected Terminal products belong to the caller's build. The Package can
publish its completed products into the shared installation root or the caller
can continue supplying them explicitly.

Puffer's process model and user interface are optional. Another application can
reuse source interpretation, Package resolution, compilation, linking, and
Archive support without adopting either one.

Puffer can recover the layout of an unfinished TTX token stream without
requiring semantic completion:

```text
puffer -format source.ttx another.ttx
```

Formatting rewrites each source transactionally. The recovery path preserves
unknown and incomplete tokens and supplies placeholder source documentation
when the leading document is absent. Once a Monograph completes, the language
server hands the document to the formatting Terminal. That path applies the
canonical declaration and whitespace rules and can reflow ordinary comment
paragraphs. A bare `//` remains an intentional paragraph boundary.

Comment markers, hexadecimal widths, byte groups, and single Statement Blocks
receive one prescribed spelling. Packs and Layouts stay on one line through the
100 column limit, then place one top level entry on each line with a trailing
comma. Adjacent declarations and plain assignments align their `:` and `=`
columns only when the required padding is at most eight columns and the aligned
prefix remains short. Pack delimiters, Documentation, Attributes, and Blocks
end an alignment island, so an outer assignment never pads named entries inside
its Pack.

Control statements receive a Pack directly. A one-expression `if` condition
therefore omits optional outer parentheses while retaining parentheses needed
for precedence inside that expression. Empty and multiple-value Packs keep
their delimiters.

Within one scope paragraph, formatting accepts Definitions, ordinary
Statements, any number of compressed `:` Blocks, then at most one braced Block.
Returning to an earlier stage inserts one blank line. Documentation begins its
own stage before the item it describes.

An empty result Function does not retain redundant trailing `return;`
Statements. Formatting removes every consecutive trailing bare return from a
multiple Statement body. If that leaves no Statement, the canonical spelling is
`: return;`. An explicitly authored one line `: return;` is preserved. Returns
inside nested Blocks and content following an earlier return are not analyzed
or removed. The language server exposes the same formatter as standard
document formatting for open editor buffers.

## Terminal production

Puffer brings both composition sides together for one request. The installed
Dialects determine what its Toolchain can understand, while the selected
Terminal producers determine what the completed Workspace can produce. Puffer
coordinates that handoff without turning either side into its own model.

For a compilation request, Puffer opens the declared sources and dependencies
in a Workspace. Environment interprets and checks the complete source group. If
there are errors, Puffer shows them and does not produce partial output.

When the Workspace is ready, Puffer coordinates the requested products:

* The ABI Terminal publishes the shared C representation and language headers.
* The LLVM Terminal consumes that agreement and compiles Library meaning into
  CPU code.
* The Vulkan Terminal consumes Shader, Pipeline, and Library meaning and emits
  the SPIR-V module with its matching generated CPU description.
* Package produces a Complete or Contract Archive.
* Linker combines native member products into libraries and platform
  executables.

The request chooses the CPU target, host platform, graphics backend,
and Archive profile. Puffer passes those choices to the components that own the
formats, then writes or displays their results.

### Build ownership

Puffer is also the common build front door. Its build request reads the Package
description, resolves dependencies, opens one Workspace, selects the requested
Terminals and Linker, and publishes the completed product set transactionally.
That orchestration is the same whether the request begins in a terminal, a
larger build system, or an editor.

The Visual Studio Code extension remains a small client. It presents build
commands and progress, but sends the request to its bundled Puffer instead of
recreating Package discovery, target selection, caching, or linking in
TypeScript. A command line Puffer from a GitHub release exposes that same path
outside VS Code.

Bazel can integrate by declaring inputs and outputs around Puffer. It is not a
semantic owner and does not define a second Tetrodotoxin build model. This lets
the extension distribution eventually build a normal TTX project immediately,
while repositories that already use Bazel can keep their surrounding graph.

### Package and application products

A Package request is the complete source build. It imports each dependency
through its Contract Archive, opens the root Package once, walks its reachable
Type graph, and keeps that Workspace alive while every requested Terminal walks
it. Publicly reachable local Callables receive their deterministic ABI bindings
before any member is lowered, so separately compiled sources can call one
another without depending on compilation order. Library members become CPU
objects through the
[LLVM Terminal](../tetrodotoxin/terminal/llvm/README.md). Shader members become
validated SPIR V words and Linker places those words in the Package's read only
native data. Pipeline and other contract members contribute their durable meaning
without manufacturing an empty LLVM program.

This gives the build one natural publication barrier. If any source, Terminal,
or native provider fails, Puffer publishes none of the Package products. A
successful request emits Complete and Contract Archives, the native member
objects, one C interface, and the native ABI Manifest. A Package that selects a
C++ API also emits its canonical facade.

Debug selection belongs to the Package request. `none`, `line`, and `full`
preserve the same behavior, while the latter two add DWARF 5 source correlation
to CPU objects. Full debug identifies physical scalar, pointer, array, and
structure carriers as C11 so stock LLDB can reconstruct them. That compatibility
profile does not assign C syntax or semantics to authored TTX source.

The generated C header is the exact declaration surface for every fully public
Function. Independently compiled consumers provide the ABI proof. The ABI
Terminal derives generated names from semantic routes, while an optional legacy
symbol override can meet one existing platform spelling. C aggregate and
multiple result carriers follow the 64 bit x86 System V classification used by
the emitted interface. Internal TTX calls may use direct aggregate carriers
instead. Option carriers store
their payload and selected state inline with the same value semantics as
`Perimortem::Core::Option`. They add no allocation, shared identity, or retain
and release interface. Object carriers are opaque one word handles. Parameters
borrow them, results transfer one reservation, and the generated header exposes
the generic Perimortem retain and release entries for a host that keeps a result.

The build supplies Package-rooted `.ttx` candidates, while contextual Import
Types are the sole authority for semantic names and external graph edges.
Package coordinates those products without lowering a copied semantic graph. Compiled
Shader Programs remain independent SPIR V products inside the native Package
artifact rather than executable bodies inside its semantic Archive.

An application request is source free. It restores the root Complete Archive
and dependency Contract Archives, selects the App policy retained by the
requested member, and emits a small native entry object. The build toolchain
then links that entry with the Package and runtime native products.

## Restoring an Archive

Puffer first asks Package to check the Archive, its exact external Type graph,
and its selected profile. It then creates a new Workspace with the languages
named by the Archive. Package is restored first, each member is reconstructed,
and the recorded Import Types reacquire their real roots before graph completion.
Scene and Shader pass the surrounding context to their child layers.

The restored members still go through normal linking and finalization before
Puffer exposes them. Restoration skips reading and parsing source, but it does
not skip the checks that make the graph safe to use.

Archive bytes, LLVM IR, debug data, and native objects are finished outputs.
Puffer coordinates them without treating their records as live language
objects.

## Language server

Run Puffer over a local socket:

```text
puffer --pipe=<socket-path>
```

Puffer speaks the position encoding offered by the editor. UTF 8 matches TTX's
source bytes directly, while clients such as VS Code currently ask for UTF 16
coordinates. The translation stays at the protocol boundary where Puffer still
has the source text needed to perform it.

Once a document is open, the server provides:

* UTF 8 positions for clients that count bytes and UTF 16 compatibility for
  clients that count code units
* opening, replacing, and closing complete document text
* source diagnostics with authored ranges
* semantic hover with complete Callable signatures, documentation, declaration
  facts, Types, and constants
* parameter name inlay hints derived from each Call's retained input fitting
* go to definition for authored semantic identities across Package sources and
  for available source, Package, and embedded Resource inputs selected by the
  Workspace
* semantic tokens that recognize Generic formulas selected by the completed
  graph
* clean shutdown and exit handling

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents.

The Tetrodotoxin distribution builds Puffer with the Dialects that belong to
this platform. A project extending TTX builds Puffer with its additional
Dialect installed, then packages that Toolchain with its language extension.

See [Tetrodotoxin](../tetrodotoxin/README.md) for the host and its Dialects and
[TTX](../ttx/README.md) for the shared semantic vocabulary.
