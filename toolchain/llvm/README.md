# Contained LLVM SDK

The LLVM backend needs headers and a runtime that agree exactly. Depending on
whatever LLVM happens to be installed on a development machine would make that
agreement difficult to reproduce, so Tetrodotoxin keeps one reviewed SDK pair
behind this small Bazel facade.

`MODULE.bazel` creates a pinned external repository from matching Arch Linux
`llvm` and `llvm-libs` packages. `//toolchain/llvm:sdk` is the only local target
that exposes them, and only `//backend:llvm` can consume it. Library and the
other semantic systems therefore build without inheriting LLVM headers, flags,
or link dependencies.

This facade is intentionally narrower than a registered C or C++ toolchain. It
supplies the API and `libLLVM.so.22.1` needed by the in process backend without
changing ordinary compile actions elsewhere in the repository.

The repository rule checks the immutable archive hashes, both package names and
versions, the public LLVM header version, and the versioned runtime object. A
mistaken package pair is caught during repository setup, where the relationship
is easiest to understand, rather than later as a surprising ABI failure.

The current SDK supports Linux x86 64. A future host or LLVM revision can add
another reviewed package pair while keeping host discovery and ambient
toolchain configuration out of ordinary builds.

## Updating the pin

An update keeps the development headers and runtime together:

1. Select matching `llvm` and `llvm-libs` x86 64 packages from the immutable
   Arch package archive.
2. Record both filenames and SHA 256 values in
   [`MANIFEST.md`](MANIFEST.md), then update the repository URLs and expected
   metadata in `repository.bzl`.
3. Update the expected header version, runtime soname, repository name, and
   `MODULE.bazel` facade as one reviewed change.
4. Build `//toolchain/llvm:sdk` and `//backend:llvm`, then rerun the object, C
   ABI, DWARF, reproducibility, and containment evidence.

The backend also compares the loaded runtime version with its compile time
headers for every request. That final check keeps a replaced shared object from
quietly changing the toolchain beneath an otherwise reproducible build.
