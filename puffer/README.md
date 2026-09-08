# Puffer

Puffer accepts one source file or one local LSP socket path:

```sh
puffer build.ttx
puffer /path/to/editor.sock
```

The entry loads arguments through Perimortem `System::Args`. A source invocation
registers only Build in its Toolchain and calls `toolchain.process(source)`.
The engine reads the documentation comment and `dialect : Name;` header, finds
the installed Dialect by name, and passes it the remaining source unchanged.

Toolchain retains processed source bytes and Monographs until destruction.
It does not invoke Workspace, acquire Packages, schedule linking or finalization,
or select terminal generators. Build orchestration and plugin loading belong to
the next Build Dialect stage. The former Package CLI and native application
commands are deliberately regressed during this stage.

A Unix socket path enters the retained LSP implementation. The TODO at this
entry records its future move to an LSP Dialect. That existing subsystem still
owns its language support and repository setup, independently of the minimal
source bootstrap. The editor launcher must supply the socket as the first and
only argument. The previous `-lsp=` and repository flags are not accepted.

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
