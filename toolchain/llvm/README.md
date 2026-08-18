# Contained LLVM SDK

This directory supplies the development API for Library's LLVM compiler. It is
not a registered C or C++ toolchain and it does not affect ordinary compile
actions. `MODULE.bazel` creates one pinned external repository from matching
Arch Linux `llvm` and `llvm-libs` packages, and `//toolchain/llvm:sdk` is the
only repository-local facade.

The facade is visible only to the `//tetrodotoxin` package. Library consumes it
as an `implementation_deps` dependency of `//tetrodotoxin:library_llvm`.
Consequently the LLVM headers and include directory participate in that
compiler's actions, but do not propagate through its public interface to
Puffer, the LSP library, or another consumer.
The imported `libLLVM.so.22.1` is still a link dependency, as required by the
in-process backend.

The repository rule deliberately verifies three independent facts after Bazel
checks the immutable archive hashes: both Arch package names and versions, the
LLVM public header version, and the versioned runtime object. This catches a
mistaken package pairing close to repository setup rather than as an ABI fault
while the backend is running.

This first SDK is intentionally limited to Linux x86-64. Adding another host or
LLVM revision requires a separately reviewed immutable package pair. It must not
turn this dependency into host discovery or a globally registered toolchain.

## Updating the pin

An update is one reviewed pair, never an independent header or runtime change:

1. Select matching `llvm` and `llvm-libs` x86-64 packages from the immutable
   Arch package archive.
2. Record both exact filenames and SHA-256 values in [`MANIFEST.md`](MANIFEST.md)
   and update the repository URLs and expected metadata in `repository.bzl`.
3. Update the expected public header version, runtime soname, repository name,
   and `MODULE.bazel` facade together.
4. Build `//toolchain/llvm:sdk` and `//tetrodotoxin:library_llvm`, then rerun the compiler
   object, C ABI, DWARF, reproducibility, and containment evidence.

The repository rule rejects a mismatched pair before compilation. The backend
also compares the linked runtime version with the compile time header version
on every request, so replacing only the shared object cannot silently pass.
