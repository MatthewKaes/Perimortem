# Puffer

Puffer is how people and build systems enter Tetrodotoxin. The command line can
turn a Package into finished products, while the language server gives an
editor a live understanding of the same sources. A diagnostic, hover, build,
and Archive therefore begin from the same view of the program.

Behind that experience, Puffer assembles the requested languages and opens a
Workspace for them. It coordinates the journey without taking ownership away
from the parts that understand it best. Package still owns reproducible
composition, each Dialect still owns its language, and each backend or Linker
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
Language compilers own their native member products, while the selected build
toolchain owns static archives and final executable linking.

The LSP constructs one Environment Toolchain and lends it to every replacement
Workspace. A Package session reads the selected manifest, recursively loads
each exact dependency from the configured Package root, detects active cycles,
and imports the consumer only after its dependencies complete. One shared
Snapshot owner retains editor overlays and unchanged filesystem bytes across
those complete graph replacements. No standard Package name is injected into
an unrelated Package.

Puffer's process model and user interface are optional. Another application can
reuse source interpretation, Package resolution, compilation, linking, and
Archive support without adopting either one.

Puffer can canonically format any TTX token stream without requiring semantic
completion:

```text
puffer -format source.ttx another.ttx
```

Formatting rewrites each source transactionally. It preserves unknown and
incomplete tokens, applies the same declaration and whitespace rules to every
equivalent token stream, and supplies placeholder source documentation when the
leading document is absent. Comment markers, hexadecimal widths, byte groups,
and single Statement Blocks receive one prescribed spelling. Packs and Layouts
stay on one line through the 100 column limit, then place one top level entry on
each line with a trailing comma. Adjacent declarations and plain assignments
align their `:` and `=` columns only when the required padding is at most eight
columns and the aligned prefix remains short. Documentation, Attributes, and
Blocks end an alignment island.

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

* The LLVM backend compiles completed Library meaning into CPU code.
* Shader produces SPIR-V for the GPU.
* Package produces a Complete or Interface Archive.
* The build toolchain combines native member products into libraries and
  platform executables.

The request chooses the CPU target, host platform, graphics backend,
and Archive profile. Puffer passes those choices to the components that own the
formats, then writes or displays their results.

### Compile one Library source

The shortest way to try native Library code is to compile one standalone source.
This path skips Package restoration and keeps the request focused on the source
in front of you. Puffer selects the command, presents diagnostics, and publishes
the requested files together. The
[LLVM backend](../backend/llvm/README.md) performs lowering and returns only
completed products. Generated Objects use the linked Perimortem reference
counted runtime surface.

```text
puffer -library \
  -backend=llvm \
  -target=x86_64-sysv \
  -debug=none \
  -name=Example \
  example.ttx \
  -ir=example.ll \
  -object=example.o \
  -header=example.h
```

`-debug` accepts `none`, `line`, or `full`. All three modes preserve runtime
behavior. The latter two emit DWARF 5 source correlation from the request's
source path and Anchors, while `none` emits no debug sections. A failed
source, backend, or staging request publishes none of the requested final files.
Full debug identifies its physical scalar, pointer, array, and structure
carriers as C11 so stock LLDB can reconstruct them. That compatibility profile
does not assign C syntax or semantics to the authored TTX source.

The generated C header is the matching declaration surface for Functions that
explicitly request an external C interface with `@abi("C")`. Independently
compiled consumers provide the ABI proof. TTX calls need no ABI Attribute. The
header publishes readable generated TTX names unless an optional `@symbol`
requests one exact external spelling. C aggregate and multiple result carriers
follow the 64 bit x86 System V classification used by the emitted interface. Internal
TTX calls may use direct aggregate carriers instead. Option carriers store
their payload and selected state inline with the same value semantics as
`Perimortem::Core::Option`. They add no allocation, shared identity, or retain
and release interface. Object carriers are opaque one word handles. Parameters
borrow them, results transfer one reservation, and the generated header exposes
the generic Perimortem retain and release entries for a host that keeps a result.

### Package and application requests

A Package request imports every dependency through its Interface Archive,
imports the root source Package once, and compiles each declared member into an
independent native object. It emits the root Complete and Interface Archives,
one combined C and C++ declaration header, one native ABI Manifest, and the
member products declared by the build action. The build supplies manifest
rooted `.ttx` candidates, while the Package Source table remains the sole
authority for their semantic member names and paths. Package coordinates those
products without lowering a copied
semantic graph.

An application request is source free. It restores the root Complete Archive
and dependency Interface Archives, selects the App policy retained by the
requested member, and emits a small native entry object. The build toolchain
then links that entry with the Package and runtime native products.

## Restoring an Archive

Puffer first asks Package to check the Archive, its dependencies, and its
selected profile. It then creates a new Workspace with the languages named by
the Archive. Package is restored first so every member receives the same import
and resource context. Scene and Shader pass that context to their child layers.

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
* go to definition for authored semantic identities across Package sources
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
