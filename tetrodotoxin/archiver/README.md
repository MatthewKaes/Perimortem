# Archiver

Archiver owns Tetrodotoxin's durable package buffer. The writer turns a complete
build product into bytes. The reader validates those bytes and reconstructs
package facts inside a new owner boundary.

The existing format predates the current Abstract model. Its old README treated
ClassDB schemas, allocated Routes, copied Layout entries, and address tables as
settled architecture. Those concepts are not part of the current TTX contract
and must not steer the replacement format.

## Confirmed requirements

A durable package needs enough information to restore:

- its authored package name and explicit version
- dependency names and versions
- the package's public semantic surface
- the concrete Types and Layout relationships required by that surface
- public Callable and ISA-owned facts required by consumers
- opaque terminal products carried with the package.

Archive-local indices may compact references inside one buffer. They are not
durable names and cannot escape as semantic identity. Process pointers, C++
RTTI, vtables, map order, and pointer-keyed implementation tables are never
serialized.

The representation of ISA-defined contracts is still open. It must be designed
from the concrete Abstract extension model. Archiver must not invent a central
class repository merely to make arbitrary objects serializable.

## Writing

The writer accepts a complete and validated package product. It may assign dense
local indices, deduplicate immutable records, and choose a compact physical
layout. Those choices remain private to the format version.

The writer does not discover package identity, select public names, infer Alias
targets, reconstruct Type shape from backend records, or publish private
Callables. Those decisions belong to the systems that own the graph and package
surface.

## Reading

The reader validates the header, format version, table bounds, references, and
every invariant required before publication. It reserves stable local objects
when the chosen format requires a multi-stage restore, then connects and
validates the complete graph before exposing its root.

Failure produces Invalid through the package-owning boundary. Consumers never
observe a partially connected graph or null semantic references.

## Layout and terminal facts

Structured Layouts restore references to their real Addressable objects. The
archive must not flatten those objects into copied member records containing
names, child Types, documentation, attributes, offsets, and target facts.

Terminal Types restore the semantic facts required by the receiving toolchain.
Aggregate size, alignment, offsets, ABI classification, and register placement
remain derived target products unless an explicitly versioned terminal artifact
owns them.

The exact encoding cannot be finalized before Generic, ISA extension, and
foreign-boundary contracts are concrete. Until then, current reader and writer
code is migration evidence rather than a specification for the new model.
