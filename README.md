# Perimortem

A "Game Engine" for projects on the verge of death, you just don't want
to admit it to yourself.

## About

What started as a playful fork of Godot ended up becoming a complete overhaul as I untangled the guts of Godot's legacy.
Inspired by Godot, Perimortem acts as a playground for something "different".

I would highly recommend you don't try to build anything with this. 😇

### How to build

Install the numerous unlisted dependencies then run bazel and hope for the best!

> bazel run //main:editor

### List of dependencies

This is mostly here in case I forget:
`pacman -S bazel clang llvm`

## Supported Editors

Perimortem has built in support for VSCode on linux. To run OOTB you'll need the following extensions:

* Bazel - The Bazel Team
* C/C++ - Microsoft
* CodeLLDB - Vadim Chugunov

If you are running on Arch you will need the closed source version of VSCode in order to run the C++ extensions.
If you are fine without it then go ham.

### Tetrodotoxin support

Perimortem support Tetrodotoxin in VSCode via an LSP (language server protocol) service.
You can build and launch a version of VSCode with the TTX extension using:

> debug Launch TTX Client

If you aren't using VSCode you can use `ttx-lang-server` in your engine of choice. The code for the server lives in `tetrodotoxin/lsp/server`, however if you want to build this project it just assumes you are using VSCode on Arch.

## OS Support

Arch Linux w/ Wayland

### Windows Support?

No. Why are you even using this? Go use a real engine.

## Disclaimer

I take no responsability for any damage reading the source of this codebase may do to you, emotionally, mentally, physically, financially, or romantically.
