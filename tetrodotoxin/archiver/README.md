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
- its exported Package definitions and named semantic surface
- every typed Abstract edge reachable from that surface that the selected product
  requires
- the concrete Types, Layouts, Callables, and Dialect-owned facts on those edges
- opaque terminal products carried with the package.

Archiver does not preserve Source. Going from a resolved Source collection to a
Package is already a deliberate loss of authoring detail; compiling that Package
loses more. The restored graph retains enough reflection to inspect types,
compile consumers incrementally, and invoke exported addresses, but it cannot
claim that the original sources survived.

Alias targets, Structured Addressables, ABI linkage, executable bodies, and Dialect
extensions remain relationships owned by their concrete contracts.
Archiver preserves those reachable graph edges rather than projecting them into
a root Type table plus unrelated linkage records.

Archive-local indices may compact references inside one buffer. They are not
durable names and cannot escape as semantic identity. Process pointers, C++
RTTI, vtables, map order, and pointer-keyed implementation tables are never
serialized.

The representation of Dialect-defined contracts is still open. It must be designed
from the concrete Abstract extension model. Archiver must not invent a central
class repository merely to make arbitrary objects serializable.

## Writing

The replacement writer accepts a complete and validated `Model::Package`. It
may assign dense local indices, deduplicate immutable records, and choose a
compact physical layout. Those choices remain private to the format version.

The writer does not discover package identity, select public names, infer Alias
targets, reconstruct Type shape from backend records, or publish private
Callables. Those decisions belong to the systems that own the graph and package
surface.

## Reading

The reader validates the header, format version, table bounds, references, and
every invariant required before publication. It reserves stable local objects
when the chosen format requires a multi-stage restore, then connects and
validates the complete graph before exposing a `Model::Packages::Precompiled`.
That object implements the same `Model::Package` contract used by the writer and
by source-backed packages, but it does not implement
`Model::Packages::Interpreted`.

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

The exact encoding cannot be finalized before Generic, Dialect extension, and
foreign-boundary contracts are concrete. Until then, current reader and writer
code is migration evidence rather than a specification for the new model.

## Current Migration Delta

The checked-in reader and writer still encode the rejected split:

| Current code                                      | Required replacement                                  |
| ------------------------------------------------- | ----------------------------------------------------- |
| `Package` retains a root `Ttx::Type`              | restored object implements `Model::Package`           |
| writer receives a caller-built Type side table    | writer discovers edges from the Package export graph  |
| reader reserves only concrete `Ttx::Type` records | reader restores the contracts present in the graph    |
| ABI linkages occupy a separate table              | address and execution edges stay on their real owners |
| source records carry an `Implementation` table    | Dialects enrich the same reachable Abstract objects   |

Adapting Source or `Model::Package` back into the old `root_type` parameter
would preserve the wrong architecture behind a new name. The current
`Model::Packages::Precompiled` class establishes the receiving Abstract
contract, but the checked-in Archiver reader and writer have not yet migrated to
it. The next archive implementation begins only after executable, address, and
Dialect-owned contracts expose the edges that must survive restoration. Its first
slice should define how those concrete contracts encode and restore themselves,
then replace the root and traversal together.
