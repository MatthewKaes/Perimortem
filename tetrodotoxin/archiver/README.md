# Archiver

Archiver stores a resolved Tetrodotoxin package as a portable Abstract graph.
The Puffer Buffer preserves package identity, registered semantic contracts,
resolution routes, Layouts, callable Addresses, and terminal products without
serializing compiler pointers, C++ RTTI, or target-private objects.

The writer consumes a valid Compiler-owned graph and returns completed bytes.
The reader borrows those bytes and restores a complete graph inside the caller's
Compiler boundary. Corrupt or incompatible input produces a Package whose root
is Invalid. A partially initialized graph and a null root are not valid reader
results.

## Durable Package Model

A package snapshot contains:

- an authored package Route and explicit package version
- source-visible dependency aliases and dependency versions
- the ClassDB schema Routes required by its objects
- local Abstract objects and their class-qualified edges
- every public Resolution route into that graph
- concrete Layouts and their child Type references
- public Callable Addresses
- opaque terminal products.

The filesystem path from which the buffer was loaded is diagnostic context, not
package identity. Content hashes may be used as optional integrity checks, but
they do not name packages, Classes, Types, Callables, or exports.

A dependency records:

```text
local name | package route | package version
```

The local name reconstructs the source-visible import edge. The package Route
and version select the dependency supplying external objects. Dependency order
and archive-local ids have no durable meaning.

## Tables

The buffer uses a versioned header and independently addressable tables. A
conforming format provides these logical tables even if a physical encoder
combines immutable records:

| Table | Contents |
| ----- | -------- |
| Manifest | package Route, package version, dependencies, format requirements |
| Schemas | ClassDB schema Routes and required schema versions |
| Objects | local Abstract records keyed by archive-local object id |
| Edges | named, contract-qualified graph edges between objects |
| Routes | authored and public resolution routes |
| Layouts | fluid/concrete layouts and recursive entry descriptions |
| Addresses | public Callable endpoints and their linkage contracts |
| Terminals | opaque group/path/content products |

Archive-local ids are dense references assigned by one writer transaction. They
are never emitted as symbols or assumed stable after restore.

## Manifest

The manifest is the package identity and compatibility surface:

```text
route package_route
version package_version
version required_ttx_abi
count dependency_count
repeat dependency_count:
  bytes local_name
  route package_route
  version package_version
```

If several incompatible package versions coexist, the selected version is also
an explicit package Route segment when that package is published into a shared
resolution root. Versioning is not folded into a name hash.

## Schema Table

Each object names a registered Class by durable schema Route. The schema table
deduplicates those references:

```text
count schema_count
repeat schema_count:
  route class_route
  version class_version
  optional route parent_class_route
```

Examples include:

```text
Ttx1.Abstract.Type
Ttx1.Abstract.Type.Alias
Ttx1.Abstract.Callable.Free
Shader1.Abstract.Type.Stage
```

The receiving Toolchain resolves each Route through ClassDB and proves the
registered parent and version requirements before object construction. A loaded
ClassDB may use dense local indices, but those indices are never serialized.
Unknown schemas, incompatible ancestry, or unavailable required operations make
the package root Invalid.

## Object And Edge Tables

An object record stores common Abstract identity and the payload defined by its
registered Class:

```text
count schema_id
bytes local_name
documentation
attributes
bytes class_payload
```

The class payload is decoded through the registered schema operation. Type,
Alias, Generic, Callable, Free, Self, Address, and ISA-specific objects therefore
remain distinct objects instead of being flattened into one serialized Type
record with optional fields.

Graph ownership is encoded separately:

```text
count edge_count
repeat edge_count:
  count owner_object_id
  count contract_schema_id
  bytes child_name
  object_ref child
```

Names must be unique for each owner and contract layer. The same owner may
therefore publish `Free/open` and `Self/open`; it may not publish two Free
children named `open`. Alias and import edges can make one object reachable
through several Routes without copying the object.

An `object_ref` is one of:

| Kind | Payload | Meaning |
| ---- | ------- | ------- |
| Local | local object id | object in this buffer |
| Package | dependency id plus object id | object supplied by a dependency |
| Standard | schema-defined standard Route | object supplied by the receiving Toolchain |

There is no None object reference. Optional metadata uses an explicit optional
field in its owning schema. A semantic edge either designates an Abstract or the
owning object is Invalid.

## Route Table

A Route is a sequence of reversible steps:

```text
count step_count
repeat step_count:
  count contract_schema_id
  bytes name
```

The table records package roots, authored import/alias routes needed by tools,
and selected public routes. Canonical Type identity does not replace those
routes. Publication does not sort all aliases and choose one implicitly.

Linker-safe encoders may length-prefix or escape route segments, but they must be
reversible. Hashes are not a route encoding and are not serialized as semantic
identity.

## Layout Table

Layout is a recursive value/storage fact rather than an incidental list of Type
members:

```text
u8 kind                    fluid or concrete
count byte_size            concrete layouts
count alignment            concrete layouts
count entry_count
repeat entry_count:
  optional bytes name
  object_ref child_type
  count offset             concrete layouts
  u8 defaulted
  documentation
  attributes
```

A terminal concrete Layout has no entries. An aggregate has one or more entries
whose child references resolve to objects implementing Type. Restore validates
size, alignment, offsets, entry bounds, and Type ancestry before publishing the
Layout. This is the same recursive shape consumed by ABI lowering; the archive
does not substitute a source `@abi` number.

Callable records reference parameter and result Layouts. Self's parameter
Layout includes the receiver at entry zero. The archive never reconstructs or
removes the receiver based on a function table or parameter spelling.

## Addresses

The Address table connects public Callable objects to invocation endpoints:

```text
count address_count
repeat address_count:
  object_ref callable
  route public_route
  route address_class_route
  bytes address_payload
```

The address class describes whether the endpoint is a native symbol, imported
symbol, interpreted body, runtime callback, or another registered mechanism.
Native symbol payloads use the reversible public Route encoding defined by the
compiler. They are not signature hashes.

Only public and exposed Addresses enter a package snapshot. Private local
Addresses may remain in a companion machine archive but cannot become public
merely because another package restores the buffer. A declaration whose
endpoint is intentionally unresolved serializes an explicit unresolved Address
contract when permitted; it does not serialize a null pointer.

## Terminal Products

A terminal is opaque to Archiver:

```text
bytes group
bytes path
bytes content
```

The group and path identify a product to the consuming toolchain. Archiver
preserves linker archives, generated interfaces, shader modules, and other
terminal bytes without interpreting their internal format.

## Restore Transaction

Restore runs in phases so every published object is valid:

1. Validate the header, directory, table bounds, and exact format version.
2. Resolve every required schema Route through the receiving ClassDB.
3. Reserve all local object identities in the receiving Compiler arena.
4. Decode class payloads and Layouts through registered schema operations.
5. Connect object edges and dependency references.
6. Validate route uniqueness, Type ancestry, Alias acyclicity, Layouts, and
   Callable/Address compatibility.
7. Publish the package root and public Routes.

Any failure publishes an Invalid root retaining the failed archive Route and
diagnostic cause. No consumer observes half-connected objects.

## Compatibility

Changing a durable field, table, route rule, or schema payload requires a format
or schema version change. Readers reject incompatible required versions rather
than guessing how an older monolithic Type record maps onto the current
Abstract hierarchy.

The package format deliberately does not promise stability for:

- archive-local object, schema, dependency, or Layout ids
- loaded ClassDB indices
- compiler object addresses
- source-file paths
- map iteration order
- hashes of package contents or signatures.

Durable compatibility rests on explicit versions, schema Routes, package
Routes, and the registered operation contracts those routes name.
