# Puffer

Puffer is the user facing compiler driver and LSP application shell for
Tetrodotoxin. It accepts command line and language server requests, invokes the
reusable Tetrodotoxin libraries, and presents Diagnostics or completed products
to the user.

It occupies the same user facing role as the `clang` executable in an LLVM
toolchain. It is the program a user or build system invokes, but it does not own
every compilation stage or output format.

Use Puffer when its command line or editor protocol matches the tool being
built. Embed the Tetrodotoxin libraries directly when an application needs a
different transport, session model, build integration, or presentation layer.
The language, package, compiler, and archive behavior remains reusable in either
case.

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
Language compilers and Linker own their output formats.

This separation lets another application use the same source interpretation,
Package resolution, compilation, linking, and Archive behavior without adopting
Puffer's process model or user interface.

## Terminal production

For a compilation request, Puffer receives declared source and dependency
inputs together with the requested outputs. It constructs a Workspace with the
selected Dialects, lets Environment complete the graph, and stops before output
production when Diagnostics exist.

Puffer then asks the component responsible for each format to produce its typed
Terminal product. Library compilation lowers completed CPU facts to LLVM IR.
Shader compilation produces SPIR-V. Linker produces native objects and
executables. Package produces the semantic Archive. Puffer writes or presents
those products without introducing a common product registry or another
semantic graph.

## Source independent restoration

Puffer asks Package to validate the Archive envelope and dependency inventory.
It constructs a fresh Workspace with the required Dialects, then Environment
asks each Dialect to construct new Monographs from its payload. The restored
group passes through the ordinary link, finalize, and publication barriers
before Puffer presents it as a completed result.

This path avoids source acquisition, lexing, and parsing while retaining the
validation and completion work required to publish a trustworthy semantic
graph.

Archive bytes, LLVM IR, debug data, and native objects remain Terminal products.
Puffer never treats their records as live TTX identities. That distinction lets
Puffer coordinate several products without acquiring their semantic meaning.

## Language server

Run Puffer over a local socket:

```text
puffer --pipe=<socket-path>
```

The language server supports:

* initialization using UTF-16 document positions
* opening, replacing, and closing complete document text
* semantic tokens for complete TTX documents
* clean shutdown and exit handling

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents.

See [Tetrodotoxin](../tetrodotoxin/README.md) for the host and its Dialects and
[TTX](../ttx/README.md) for the shared semantic vocabulary.
