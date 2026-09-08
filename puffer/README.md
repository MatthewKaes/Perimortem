# Puffer

Puffer takes a source file or local LSP socket path as its first argument:

```sh
puffer build.ttx -help
puffer /path/to/editor.sock
```

The entry converts arguments after the source path to Perimortem byte views and
passes them unchanged to Build's constructor. Build owns their interpretation.
A source invocation registers the caller-owned Build instance in its Toolchain
and calls `toolchain.process(source, errors)` with a caller-owned accumulator.
The engine reads the documentation comment and `dialect : Name;` header, finds
the installed Dialect by name, and passes it the remaining source unchanged.

Toolchain retains processed source bytes and Monographs until destruction.
It does not invoke Workspace, acquire Packages, schedule linking or finalization,
or select terminal generators. Build orchestration and plugin loading belong to
the next Build Dialect stage. The former Package CLI and native application
commands are deliberately regressed during this stage.
Build currently reports that execution is unimplemented. Its old Environment
parser and product request records have been removed for the new Build design.

A Unix socket path enters the retained LSP implementation. The TODO at this
entry records its future move to an LSP Dialect. That existing subsystem still
owns its language support and repository setup, independently of the minimal
source bootstrap. The editor launcher must supply the socket as the first
argument. The previous `-lsp=` entry flag is not accepted.

Build and package the executable from the repository root:

```sh
bazel build --config=debug //puffer:puffer
puffer/package.sh --debug
bazel run --config=debug //validation:unit_tests -- silent
```

The package script installs Puffer in `.bin/puffer-sdk/bin` and exposes the
standard source repository for the retained LSP. It uses ordinary Bazel without
job or memory limits. `//validation:unit_tests` constructs its own repository
fixture and does not require Puffer to generate application or Package products.
