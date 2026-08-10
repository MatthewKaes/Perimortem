# Linker

Linker is Tetrodotoxin's source independent native product owner. It receives
object modules and declared native inputs, resolves their symbols and
relocations, and emits object libraries or platform executables. It knows
nothing about source grammar, TTX Types, Monographs, or Workspace identity.

Use Linker when a Tetrodotoxin build needs a reproducible native product from a
closed set of declared inputs. A platform linker is the better choice when a
project values the full compatibility surface of an established native
toolchain more than Tetrodotoxin's smaller ownership model. Linker does not try
to reproduce every linker script, object format, or platform convention.

## Source independent objects

A compiler gives Linker a typed object module containing sections, symbols, and
relocation requests. The module describes the native facts Linker needs without
retaining the semantic graph that produced them.

Sections own their target bytes and placement requirements. Symbols name
defined or required native ranges. Relocations name one exact symbol and the
location whose target representation must be patched. Linker validates these
relationships before it emits a format.

The same object model can receive a module produced directly by a language
compiler or read one from a declared ELF object. Reading an object is a format
operation. It never attempts to reconstruct source Types or treat debug records
as semantic authority.

## Archives and resolution

System V archives are ordered collections of object members with an index of
their global definitions. Linker preserves declared input order and extracts an
archive member only when an unresolved symbol requires one of its definitions.
Extraction continues until no new required member can satisfy another symbol.

Two strong definitions for the same selected symbol are an error. A required
symbol that remains unresolved is also an error unless the executable request
declares it as a dynamic import. Every relocation must name an admitted symbol,
fit its target encoding, and address a valid output range. These checks belong
to Linker rather than to the compiler that requested the relocation.

## Dynamic ELF inputs

A Linux executable can declare dynamic libraries and imported symbols as part
of its toolchain and Package inputs. Linker records only the dynamic dependency
names and ELF tables needed by the platform loader. It does not search ambient
host directories or infer a dependency from a missing symbol.

The selected toolchain supplies the platform entry objects, runtime libraries,
dynamic loader identity, and target profile. Package can retain native artifact
locators, but it does not own symbol resolution, relocation policy, or native
bytes.

## Executable production

An executable request provides one exact entry symbol and a closed ordered set
of generated objects, object files, archives, runtime inputs, and dynamic
dependencies. Linker resolves that set, lays out the target segments, applies
relocations, and emits one platform executable.

The executable is a Terminal product. Its bytes preserve the native facts
required by the operating system, not the semantic meaning of the Workspace
that produced it. Debug data can correlate native locations with durable source
facts, but neither the executable nor that correlation data can be fed back as
the original TTX graph.

Puffer may request and publish the result, while Linker retains ownership of
the format and its validation. See [Puffer](../../puffer/README.md) for that
application boundary and [Package](../package/README.md) for declared native
artifact relationships.
