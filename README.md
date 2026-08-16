# Perimortem

Perimortem is a general-purpose runtime library for performance-sensitive
applications and engines. It provides the low-level data, memory, system,
serialization, compression, image, and rendering components needed to build a
runtime without imposing an application language or scene model.

Perimortem stays independent from Tetrodotoxin and TTX. This repository develops
all three together, with dependencies flowing in one direction:

```text
Tetrodotoxin -> TTX -> Perimortem
```

The arrows point toward dependencies. Tetrodotoxin uses TTX to describe programs,
and both use Perimortem for runtime services. Perimortem does not know about TTX
source or Tetrodotoxin applications, so it remains useful to ordinary C++
programs and other language frontends.

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

`Core` provides the small data, view, algorithm, diagnostics, and threading
building blocks used throughout the runtime. [Memory](perimortem/memory/README.md)
provides allocation, managed Objects, Garbage Realms, and safe worker transfer.
`Compression` and `Serialization` build file-format algorithms on those
foundations.

[System](perimortem/system/README.md) owns operating-system concerns such as
files, arguments, random and identity services, input, windows, platform events,
and application lifecycle. CPU target contracts remain separate from Linux or
Windows host backends. Wayland and Win32 implementations belong behind the same
System responsibility rather than inside Graphics or Vulkan.

`Graphics` owns backend-independent concepts such as pixels, decoded images,
and render descriptions. It does not own windows, devices, presentation,
application lifecycle, or Vulkan objects. A C++ application and a compiled TTX
application must be able to produce the same Graphics data.

[`Graphics::Render`](perimortem/graphics/render/) describes a rendering
pipeline without creating backend resources. It provides the shaders, input
layouts, and bindings a backend needs to build its own program. Draw values and
backend lifetimes remain separate from that reusable description.

`Vulkan` is a concrete rendering backend. It depends on Graphics and translates
Graphics data into devices, surfaces, swapchains, pipelines, commands, and
synchronization. The application-facing runtime coordinates native System
window handles with the selected backend. Graphics never dispatches to Vulkan,
and Vulkan never receives TTX or editor objects.

This structure leaves room for other rendering backends. The application chooses
the backend it uses, while Graphics stays independent from that choice.

## Building and validation

The supported build target is x86-64 Linux with Clang and Bazel. The windowed
runtime requires Wayland, and the Vulkan backend requires a Vulkan loader and
driver.

Build the repository with:

```sh
bazel build //...
```

Run the complete unit-test suite with:

```sh
bazel run //validation:unit_tests --config=debug
```

Automation can reduce console spam with:

```sh
bazel run //validation:unit_tests --config=debug -- silent
```

`silent` still runs the complete suite and reports failures, final totals, and
total execution time.

The small C++ composition example at
[`apps/perimortem/basic_window`](apps/perimortem/basic_window) creates a System
window and a Vulkan renderer without involving TTX. It is the runtime-side
reference path for bringing up the corresponding Tetrodotoxin application.
Build and run it with:

```sh
bazel run //apps/perimortem/basic_window
```

## Tetrodotoxin tooling

Tetrodotoxin is the compiler and toolchain developed alongside Perimortem. Its
language and compiler design are documented in
[`tetrodotoxin/README.md`](tetrodotoxin/README.md), while the semantic data
model is documented in [`ttx/README.md`](ttx/README.md).

The repository includes a VS Code extension backed by the Puffer language
server. Build and install it with:

```sh
./tetrodotoxin/lsp/package.sh --install
```

Package the extension without installing it by omitting `--install`. Editors
that support LSP over a Unix-domain socket can run `puffer --pipe=<socket>`
directly.

## Support boundary

Perimortem is an active research and development project rather than a
production-supported runtime. Its supported environment is x86-64 Linux with
Wayland and Vulkan. Other operating systems and rendering backends remain
outside that support boundary.

If you are interested in low-level performance engineering, Agner Fog's
[optimization manuals](https://www.agner.org/optimize/) are an excellent
reference.
