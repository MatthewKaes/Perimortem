# Tetrodotoxin

Tetrodotoxin is the core toolchain used by Perimortem for development.

For the TTX source language, start with [ttx_design.md](ttx_design.md). The
more detailed compiler and type-system notes live in
[ttx_semantics.md](ttx_semantics.md).

## About

Tetrodotoxin provides the entire standalone toolchain for supporting TTX in Perimortem.
Tetrodotoxin is inspired by LLVM by aims to light weight and optimized for Perimortem. 

## Parser

Tetrodotoxin provides a optimized frontend for converting TTX files quickly into an AST out of the box.
Still a [ttx grammer](ttx.g4) is provided as a reference for custom parser or for generating a parser
using Antlr or another parser generator.

## Compiler

Tetrodotoxin also provides a standalone compiler out of the box as well.
The compiler aims to target `SPIR-V` and `x86_64` targets which currently covers all of the major backends
that Tetrodotoxin is currently planning on supporting.

Tetrodotoxin produces standard object files as intermediaries using a C ABI so it's easy to integrate with
`clang` or another C/C++ toolchain if desired.
