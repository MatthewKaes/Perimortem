# Perimortem

A high performance library and engine to serve as a collection of over optimized solutions to common
game engine problems.

## About

Perimortem serves as a highly opinionated engine + toolchain with a focus performance from the ground
up. This includes both runtime _AND_ build performance.

My background is in Performance Engineering (embedded, Cloud, GPUs, FPGA / ISA design, ect.), and over
my career I've grown a love for creating optimized solutions for target problems.
Perimortem is my attempt at a from scratch collections of highly optimized E2E solutions to a number of
problems in the game engine / app development space.

The core engine includes it's own memory model and standard library, built from the ground up in C++26
without leveraging STL headers to keep the build snappy.
Perimortem can be integrated with projects that use the STL but cross support is ill advised for both
build and runtime performance reasons.

Currently the project targets x64_86 and Linux as it's primary optimization targets. Windows support will
be brought on eventually, while ARM is less lower on the priority list.

Tetrodotoxin contains the prototype of a self hosted compiler from the ground up that emits object files
directly which are compatable with the clang and lld. ELF is the only supported target with support for
DWARF for debugging planned. Codegen only emits x64_86 machine code.

### Third-party dependencies

Perimortem aims to limit it's dependencies as much as possible and everything has been built black-box.

Third-party dependencies are currently limited to:
* `Bazel` (build system)

The toolchain is configured for `clang` as the main compiler.

### How to build

To perform a full debug build simply run:

> bazel clean
> bazel build ...

### How to Test

You can run the standard unit tests with:

> bazel run tests:unit_tests

### List of dependencies

To bootstrap for Arch linux simply run the following:

`pacman -S bazel clang llvm`

For Windows and other distros you'll need to install `bazel` and `clang` via your dedicated package manager.

## Supported Editors

Perimortem has built in support for VSCode. To run OOTB you'll need the following extensions:

* `Bazel - The Bazel Team`
* `C/C++ - Microsoft`
* `CodeLLDB - Vadim Chugunov`

If you are running on Arch you will need the closed source version of VSCode in order to run the C++ extensions.

### Tetrodotoxin Toolchain Support

Perimortem support Tetrodotoxin in VSCode via an Lsp (language server protocol) service.
You can build and launch a version of VSCode with the TTX extension using:

> debug Launch TTX Client

If you aren't using VSCode you can use `ttx-lang-server` in your engine of choice. The code for the server lives in `tetrodotoxin/lsp/server`, however if you want to build this project it just assumes you are using VSCode on Arch.

If you are looking for the Tetrodotoxin spec check out its dedicated [README](https://github.com/MatthewKaes/Perimortem/blob/main/tetrodotoxin/README.md).

### OS Supported

* Arch Linux (X and Wayland)

Windows support is in the works.

## Shout-outs

If you are interested in performance engineering, go read [Agner Fog's](https://www.agner.org/optimize/) optimization manuals.

## Disclaimer

As this project and the code herewithin is currently not meant for production.

I take no responsibility for any damage reading the source of this codebase may do to you, emotionally, mentally, physically, financially, or romantically.
