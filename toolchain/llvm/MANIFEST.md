# LLVM SDK manifest

| Component | Pinned artifact | SHA-256 |
| --- | --- | --- |
| Headers and development files | `llvm-22.1.8-2-x86_64.pkg.tar.zst` | `df30290f4af86681bd06a5a48ffda1e78ce6d9abd911fb62faa053a750759ea5` |
| Runtime library | `llvm-libs-22.1.8-2-x86_64.pkg.tar.zst` | `ad4454d1b969192e7a878066400a583b984210322d67c6fa8958ad1175342658` |

Both artifacts come from the immutable Arch Linux package archive under
`https://archive.archlinux.org/packages/l/`. They are the matching x86-64 Linux
packages for LLVM `22.1.8`, Arch release `22.1.8-2`. The repository rule checks
the two `.PKGINFO` records, the public LLVM version header, and the exact
`libLLVM.so.22.1` runtime filename after hash-verified extraction.

LLVM is licensed under `Apache-2.0 WITH LLVM-exception`. The package license
files remain available as `@llvm_22_1_8//:license`; the upstream license and
packaging metadata are not copied or rewritten by Perimortem.
