# Linker

Linker builds native libraries and programs from compiled object modules. It
works with sections, symbols, relocations, archives, and platform imports. It
does not read source code or depend on TTX Types and Workspace identities.

Tetrodotoxin's Linker is intended for reproducible builds with a closed set of
declared inputs. Projects that need the full feature set of an established
platform linker can still use that linker instead.

## Object modules

A compiler gives Linker an object module containing native sections, symbols,
and relocation requests:

* A section owns bytes and their placement requirements.
* A symbol names a defined or required native range.
* A relocation names the symbol whose address must be written at a particular
  location.

Linker checks these relationships before it writes an object format. The module
may come from Library's LLVM compiler, Library's direct native compiler, another
language compiler, or a declared ELF or COFF file. LLVM is one possible
producer, not a dependency of Linker.

## CPU targets and platform hosts

The CPU target defines the instruction set, data layout, calling convention,
relocation kinds, and native value representation. x86-64 System V and x86-64
Win64 are different targets even though both use the x86-64 instruction set.

The platform host defines how the operating system loads and starts the
program. Linux uses ELF together with its loader and shared-library rules.
Windows uses PE together with its loader and import rules. Window systems and
event loops belong to the host runtime, not to the CPU compiler.

The compiler selects a CPU target before it creates an object. The final link
also selects a matching host. Linker rejects a target and host that do not fit
together.

## Archives and symbol resolution

A System V archive is an ordered collection of object files with an index of
their global symbols. Linker reads archive members only when an unresolved
symbol needs one of their definitions. It repeats this process until no more
members are needed.

Two selected strong definitions of the same symbol are an error. A required
symbol that remains unresolved is also an error unless the build declares it as
a dynamic import. Every relocation must fit its target encoding and point to a
valid output range.

## Platform imports

A Linux build declares its shared libraries and imported symbols. Linker uses
those declarations to write the ELF loader, dependency, symbol, string, and
relocation records. It does not search arbitrary host directories or guess a
library from a missing symbol.

A Windows build follows the same principle with COFF objects, archives, and PE
imports. The Windows path has its own format and loader rules rather than
reusing ELF behavior under different names.

## Executable production

An executable request names the CPU target, platform host, entry symbol,
generated objects, object files, archives, runtime inputs, and dynamic
dependencies. Linker resolves the symbols, lays out the image, applies the
relocations, and writes an ELF or PE executable.

The executable contains the native facts required by the operating system. It
is not a serialized copy of the Workspace that produced it. Debug data may map
native locations back to source, but neither the executable nor that mapping
can recreate the original TTX graph.

Puffer coordinates the request and publishes the result. See
[Puffer](../../puffer/README.md) for the application boundary and
[Package](../package/README.md) for declared native artifacts.
