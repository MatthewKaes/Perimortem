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

The replacement writer accepts a complete and validated
`Model::Package::Resolved`. The
Package already owns its canonical definition table; Writer validates and emits
those IDs instead of rediscovering object ownership by searching export trees.
String and physical section compaction remain private to the format version.

The writer does not discover package identity, select public names, infer Alias
targets, reconstruct Type shape from backend records, or publish private
Callables. Those decisions belong to the systems that own the graph and package
surface.

## Reading

The reader validates the header, format version, table bounds, references, and
every invariant required before publication. It reserves stable local objects
when the chosen format requires a multi-stage restore, then connects and
validates the complete graph before exposing a `Model::Package::Precompiled`.
That object implements the same `Model::Package::Resolved` contract used by the writer and
by source-backed packages, but it does not implement
`Model::Package::Interpreted`.

Failure produces Invalid through the package-owning boundary. Consumers never
observe a partially connected graph or null semantic references.

Restored terminal products are available through the narrow
`Model::Package::Compiled` Package capability. The concrete
`Model::Terminal` owns one relative logical output path and opaque bytes;
neither the capability nor `Precompiled` exposes archive tables or filesystem
policy.

## Format 16

The first current-model format uses independently bounded Manifest, string,
graph, and terminal sections. Manifest identity bytes live directly in the
Manifest section, so repository indexing never parses or allocates the
graph-wide string table. `System::Version` owns a four-byte Major.Minor
value with two `Unsigned_16` components. Either component may be zero, while
0.0 is the null value and cannot identify a Manifest or dependency. Major and
Minor are encoded independently with compact unsigned integers, so small
versions do not pay even the fixed four-byte in-memory cost on the wire.

Namespace and Alias records use the Package's canonical local definition IDs.
A cross-package Alias reference stores the Manifest dependency ordinal plus the
target Package's definition ID; a reference to the dependency Package root
stores only the ordinal. Writer builds one arena-backed reverse index over the
direct dependency definitions, then resolves each Alias target in expected
`O(1)` time. The full pass is `O(dependency definitions + local aliases)` and
allocates no per-reference paths or search vectors.

Definition IDs are package-local coordinates, not global or source-level
identity. Source-backed Packages assign them by canonical named graph traversal,
and restored Packages retain the encoded order. Public visibility still comes
only from Exports. A Package can retain definitions that are absent from its
export scope so another package can preserve a canonical Type edge without
making that Type publicly discoverable by name.

The initial graph deliberately accepts Package, Namespace, Alias, visible
Documentation, dependency closure, and terminal products. Type, Layout,
Addressable, Callable, Constant, Generic, and Dialect extension persistence
remain unsupported and make Writer reject the package.

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

## Current migration boundary

The active reader and writer no longer accept a root Type, caller-built Type
table, Member or Function copies, built-in catalogue, ABI linkage side table,
Boot reconstruction, or source Record. The old files remain historical
migration evidence outside the active Archiver target. Future format work
extends the reachable current-model graph from concrete owner contracts; it
does not adapt those rejected inputs back into Writer.
