# LLVM and LLD SDK manifest

| Component | Pinned artifact | SHA 256 |
| --- | --- | --- |
| LLVM headers and static archives | `llvm-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb` | `d3aa93146a17065a12428a0db4f94eaf77475e25030784235396dc67b841cd01` |
| ELF and COFF LLD archives | `liblld-22_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb` | `006d5dab6de3fda52ee1e1379d39e8a4d49a650a43f8812d1cc5870a89694118` |
| LLD headers | `liblld-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb` | `d4c0e4728208eba7d4068250bc46dd3aa2380d8c918d19810463b22d4a0b37ed` |
| LTO optimization support | `libpolly-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb` | `fc32d319eb3113898d3ad9246d46a0605ce1fdabfa4993294464640649cbd9e9` |

The artifacts come from the LLVM project's qualified Ubuntu 22.04 repository
under `https://apt.llvm.org/jammy/`. They share the exact package version
`1:22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81`.
The repository rule checks every archive hash, package name and version, the
public LLVM version header, and representative LLVM, ELF LLD, and COFF LLD
archives after extraction.

LLVM and LLD are licensed under `Apache-2.0 WITH LLVM-exception`. Their package
copyright files remain available through `@llvm_22_1_8//:license`. Tetrodotoxin
keeps those upstream notices intact.
