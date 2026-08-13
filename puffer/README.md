# Puffer

Puffer is the user-facing compiler driver and LSP application shell for
Tetrodotoxin. It accepts command-line and language-server requests, invokes the
reusable Tetrodotoxin libraries, and presents Diagnostics or completed products
to the user.

It occupies the same user-facing role as the `clang` executable in an LLVM
toolchain. It is the program a user or build system invokes, but it does not own
every compilation stage or output format.

Build systems and editors can talk to Puffer through its command-line or
language-server protocol. Applications that need a different transport,
session model, or user interface can use the Tetrodotoxin libraries directly.
The same language, Package, compiler, and Archive behavior is available in both
forms.

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

Puffer's process model and user interface are optional. Another application can
reuse source interpretation, Package resolution, compilation, linking, and
Archive support without adopting either one.

## Terminal production

For a compilation request, Puffer opens the declared sources and dependencies
in a Workspace. Environment interprets and checks the complete source group. If
there are errors, Puffer shows them and does not produce partial output.

When the Workspace is ready, Puffer coordinates the requested products:

- Library compiles CPU code with LLVM or the direct native compiler.
- Shader produces SPIR-V for the GPU.
- Linker produces ELF programs for Linux or PE programs for Windows.
- Package produces a Complete or Interface Archive.

The request chooses the compiler, CPU target, host platform, graphics backend,
and Archive profile. Puffer passes those choices to the components that own the
formats, then writes or displays their results.

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

The language server supports:

- initialization using UTF-16 document positions
- opening, replacing, and closing complete document text
- semantic tokens for complete TTX documents
- clean shutdown and exit handling

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents.

See [Tetrodotoxin](../tetrodotoxin/README.md) for the host and its Dialects and
[TTX](../ttx/README.md) for the shared semantic vocabulary.
