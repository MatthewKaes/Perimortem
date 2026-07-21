# Perimortem

Perimortem is a general-purpose runtime library for performance-sensitive
applications and engines. It provides the low-level data, memory, system,
serialization, compression, image, and rendering components needed to build a
runtime without imposing an application language or scene model.

The library is intentionally independent from Tetrodotoxin and TTX. This
repository develops all three together, but their dependency direction is
strict:

```text
Tetrodotoxin -> TTX -> Perimortem
```

The arrows point toward dependencies. Tetrodotoxin uses TTX as its semantic
model, and both use Perimortem as runtime infrastructure. Perimortem does not
know about TTX source, compiler artifacts, editor documents, or Tetrodotoxin
application concepts. This allows the runtime to remain useful to ordinary C++
programs and to other language frontends.

## Runtime design

Perimortem separates portable domain data from operating-system integration
and concrete rendering APIs.

```text
application or runtime composition
|-- System
`-- Vulkan -> Graphics

Graphics -> Compression
Graphics -> Memory
System   -> Memory
Memory   -> Core
```

`Core` owns the small data, view, algorithm, reader, writer, diagnostics, and
threading primitives used throughout the runtime. `Memory` owns allocation and
managed or dynamic storage. `Compression` and `Serialization` build format
algorithms on those foundations without becoming general object models.

`System` owns operating-system concerns such as files, arguments, random and
identity services, input, windows, platform events, and application lifecycle.
The current window implementation uses Wayland. A Windows implementation
belongs behind the same System responsibility rather than inside Graphics or
Vulkan.

`Graphics` owns backend-independent concepts such as pixels, decoded images,
and render descriptions. It does not own windows, devices, presentation,
application lifecycle, or Vulkan objects. A C++ application and a compiled TTX
application must be able to produce the same Graphics data.

[`Graphics::Render`](perimortem/graphics/render/) is the boundary for pipeline
descriptions. Its `Program` record borrows shader modules, host-input layouts,
descriptor locations, and reflection data long enough for a backend to build
its own program. It does not retain backend resources or per-draw values such
as host-input bytes and vertex counts. This keeps pipeline description, backend
lifetime, and command submission as separate responsibilities.

`Vulkan` is a concrete rendering backend. It depends on Graphics and translates
Graphics data into devices, surfaces, swapchains, pipelines, commands, and
synchronization. The application-facing runtime coordinates native System
window handles with the selected backend. Graphics never dispatches to Vulkan,
and Vulkan never receives TTX or editor objects.

This dependency direction leaves room for another backend without creating a
backend registry inside Graphics. The composition layer chooses the backend it
actually uses.

## Building and validation

Perimortem uses Bazel with a pinned, hermetic LLVM/Clang 22.1.4 toolchain on
x86-64 Linux and Windows. This ensures all builds use the same bootstrapping
toolchain. All you need is to have Bazel install and build the repository with:

```sh
bazel build ...
```

Bazel selects the distribution for the host operating system and downloads clang
on the first build so a system install of clang is not required. The extracted
tools live in Bazel's externals and the downloaded archive is retained in Bazel's
repository cache.


After any changes format all C++ sources with:

```sh
./.vscode/format.sh
```

The script currently runs `clang-format` for C++, but will also format TTX and 
other types in the future.

Run the complete unit-test suite with:

```sh
bazel run //validation:unit_tests --config=debug
```

The small C++ composition example at
[`apps/perimortem/basic_window`](apps/perimortem/basic_window) creates a System
window and a Vulkan renderer without involving TTX. It is the runtime-side
reference path for bringing up the corresponding Tetrodotoxin application.

### Test Apps

Build and run it with:

```sh
bazel run //apps/perimortem/basic_window
```

## Tetrodotoxin tooling

Tetrodotoxin is the compiler and toolchain developed alongside Perimortem. Its
language and compiler design are documented in [`tetrodotoxin/README.md`](tetrodotoxin/README.md),
while the semantic data model for the tool chain is documented in [`ttx/README.md`](ttx/README.md).


### Bootstrapping

`bazel build ...` will boot strap the Tetrodotoxin toolchain and compiler, `puffer`,
using the hermetically fetched `clang`.

The repository includes a VS Code extension backed by `puffer` to provide a TTX language
server. It requires nodejs to build but can be and install it with:

```sh
./tetrodotoxin/lsp/package.sh --install
```

Editors that support LSP over a Unix-domain socket can run `puffer --pipe=<socket>`
directly.

## Project status

Perimortem is an active research and development project rather than a
production supported runtime. Hermetic Clang builds are configured for Linux
and Windows, but Linux and Wayland remain the current runtime focus. Native
Windows support and additional rendering backends are on the roadmap.

If you are interested in low-level performance engineering, Agner Fog's
[optimization manuals](https://www.agner.org/optimize/) are an excellent
reference.
