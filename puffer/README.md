# Puffer

Puffer is the user facing compiler driver and LSP application shell for
Tetrodotoxin. It accepts command line and language server requests, invokes the
reusable Tetrodotoxin libraries, and presents Diagnostics or completed products
to the user.

It occupies the same user facing role as the `clang` executable in an LLVM
toolchain. It is the program a user or build system invokes, but it does not own
every compilation stage or output format.

Build systems and editors can talk to Puffer through its command line or
language server protocol. Applications that need a different transport,
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

* Library compiles CPU code with LLVM.
* Shader produces SPIR V for the GPU.
* Linker produces ELF programs for Linux or PE programs for Windows.
* Package produces a Complete or Interface Archive.

The request chooses the CPU target, host platform, graphics backend,
and Archive profile. Puffer passes those choices to the components that own the
formats, then writes or displays their results.

### Direct Library LLVM request

The direct native path accepts one completed standalone Library source. It does
not start Package restoration or manufacture publication between Monographs.
Puffer owns command selection, stderr request errors, source diagnostic
presentation, and atomic publication. The backend in [`llvm/`](llvm/README.md)
owns LLVM lowering and returns completed bytes only. Generated Objects use the
linked Perimortem reference counted runtime surface.

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

* initialization using UTF 16 document positions
* opening, replacing, and closing complete document text
* source diagnostics with authored ranges
* semantic hover for documentation, declaration facts, Types, and constants
* semantic tokens for complete TTX documents
* clean shutdown and exit handling

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents.

The Perimortem distribution builds Puffer with the fixed set of Dialects owned
by this project. It is not a universal host that discovers arbitrary Dialects at
runtime. A project extending TTX builds its own Puffer with its additional
Dialect installed, then packages that binary with its language extension. A
future tutorial will walk through that source and extension customization.

See [Tetrodotoxin](../tetrodotoxin/README.md) for the host and its Dialects and
[TTX](../ttx/README.md) for the shared semantic vocabulary.
