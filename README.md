# Perimortem

A "Game Engine" for developers that care more about building engines rather than using them.

## About

Inspired by Godot, Perimortem serves as a highly opinionated engine + toolchain with a focus
performance.

My background is in Performance Engineering (embedded, Cloud, GPUs, FPGA / ISA design, ect.) which
has led to a large amount of "Code Detective" work in arbitrary seemingly unrelated systems.
Perimortem acts as less of a game engine and more of a challenge framework for testing out ideas.
There are few projects that have the ability to cover as much technical breadth as a game engine.

While originally I was planning to contribute to Godot, I wanted to take a systems approach to
building an optimized system from the ground up (including the entire memory model).

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

Windows support is in the works but not a core priority yet.

## Shout-outs

If you are interested in performance engineering, go read [Agner Fog's](https://www.agner.org/optimize/) optimization manuals.

## Disclaimer

As this project and the code herewithin is not meant for production.

I take no responsibility for any damage reading the source of this codebase may do to you, emotionally, mentally, physically, financially, or romantically.