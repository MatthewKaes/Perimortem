# Contained LLVM and LLD SDK

Puffer brings its native compiler with it. A developer who installs the
Tetrodotoxin extension should not also need to discover a compatible LLVM or
linker on the host, and a repository build should not spend hours rebuilding
those tools from source.

Tetrodotoxin therefore consumes one reviewed set of prebuilt LLVM and LLD
development packages. `MODULE.bazel` creates a pinned external repository from
the matching LLVM 22 packages described in [`MANIFEST.md`](MANIFEST.md). The
packages provide static LLVM components together with the ELF and COFF LLD
drivers. LLVM and LLD source trees never enter the Bazel graph.

`//toolchain/llvm:sdk` exposes the LLVM headers and the component closure needed
by the x86 64 CPU Terminal. `//toolchain/llvm:lld` adds the ELF and COFF linker
drivers over that same LLVM SDK. Both facades are private to Terminal producers,
so Library, Shader, and the other semantic systems do not inherit native tool
implementation details.

The LLVM Terminal initializes only the x86 target selected by its current
request. That keeps the static link honest: adding another CPU target requires
adding its target archives and initialization as one visible change rather than
quietly carrying every LLVM backend in Puffer.

The repository rule verifies immutable download hashes, exact package metadata,
the LLVM header version, and representative static archives. It currently uses
`bsdtar` to open the Debian package envelopes. This affects repository builds
only. A packaged Puffer binary does not extract or discover an SDK at runtime.

This facade describes Linux x86 64. A Windows distribution receives its own
reviewed LLVM and LLD archive set, keeping host selection explicit at the
distribution boundary.

## Updating the pin

An update keeps LLVM and LLD together:

1. Select one qualified LLVM release with matching development and LLD
   packages.
2. Record every filename and SHA 256 value in
   [`MANIFEST.md`](MANIFEST.md), then update the repository URLs and expected
   metadata in `repository.bzl`.
3. Derive the LLVM component closure with the matching `llvm-config` and review
   any new system libraries.
4. Build Puffer and inspect its dynamic dependency table. It should contain no
   LLVM or LLD shared object.
5. Run the object, C ABI, DWARF, reproducibility, and Package product evidence.

The release packaging pass will pin the remaining C++ runtime and LLVM support
libraries against a portable Linux sysroot. That is a distribution concern,
not a reason to return LLVM itself to a shared host dependency.
