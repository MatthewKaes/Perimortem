# Puffer

Puffer is Tetrodotoxin's command-line compiler and language-server host. It
loads complete TTX source files, evaluates their Boot preambles, resolves
source and package imports, runs the selected body ISAs, and asks the terminal
toolchain for durable products.

Puffer owns source orchestration. The reusable package reader and writer live
in [`../package`](../package/) under `Tetrodotoxin::Archiver`; they do not
depend on the Puffer CLI, resolver, or filesystem.

## Package compilation

A package build starts from one `package.ttx` root. Puffer resolves its private
source closure and registers dependency `.puffer` buffers supplied by the
caller. Package imports are resolved by manifest name and version. They do not
fall back to guessed source paths outside the current project roots.

After evaluation, terminal production lowers every eligible record. The
package builder then assembles:

- a manifest containing package identity and authored import names
- the resolved external packages needed by type references
- the root TTX type and every reachable local type
- grouped terminal byte products
- function linkage

The result is one `.puffer` file called a Puffer Buffer. The buffer is a
Tetrodotoxin package snapshot, not an object-file extension and not a public C
or C++ ABI format.

## Durable package model

`Archiver::Package` contains a required `Manifest`, the root `Ttx::Type`, the
local type table, terminal products, and function linkage. The manifest is the one
owner of package name, version, and imports. A package does not retain the path
from which its buffer was loaded.

An import is an `Archiver::Dependency`:

```text
local name | source package name | package version
```

The local name preserves the source-visible alias. The source name and version
identify the dependency package. An `Archiver::Reference` is a resolved package
used while writing or restoring cross-package type ids; it borrows the package
instead of copying its identity and types.

A terminal is an opaque triple:

```text
group | path | content
```

The group and path identify a product to the consuming toolchain. The binary
format does not reserve particular terminal names or interpret the content.

## Binary conventions

All fixed-width integers are little endian. Variable counts and byte lengths
use an unsigned base-128 integer: seven payload bits per byte and bit 7 set when
another byte follows.

The descriptions below use this notation:

```text
u8, u32, u64     fixed-width little-endian integers
count            unsigned base-128 integer
bytes            count followed by that many bytes
version          u64 high followed by u64 low
type_ref         tagged type reference described below
```

There is no alignment padding between fields or tables.

## Header and table directory

Format version 10 begins with a 24-byte header and a 32-byte table directory.
Table offsets are absolute offsets from the beginning of the buffer.

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `TTXP` |
| 4 | 4 | format version, currently `10` |
| 8 | 8 | package version high word |
| 16 | 8 | package version low word |
| 24 | 8 | Manifest table offset |
| 32 | 8 | References table offset |
| 40 | 8 | Package table offset |
| 48 | 8 | Linkages table offset |
| 56 | variable | first table data |

Offsets must be at least 56, strictly increasing, and inside the buffer. A
table ends at the next table offset; Linkages ends at the end of the buffer.
This lets a reader seek to any table without parsing or caching the preceding
tables.

The two version words are deterministic content stamps. The writer hashes the
bytes from the Manifest table through the end of the buffer for the high word,
then hashes the bytes from the Package table through the end for the low word.
`Version` reserves the low bit as the set marker when the header is read. The
reader currently uses this stamp for package identity and version matching; it
does not recompute the hashes as an integrity check.

## Manifest table

```text
bytes package_name
version standard_type_table_version
count import_count
repeat import_count:
  bytes local_name
  bytes source_package_name
  version dependency_version
```

The standard type-table version identifies the exact built-in TTX type table
used by the writer. Restore rejects a buffer produced against a different
table. The package version is not repeated here; it comes from the fixed
header.

## References table

```text
count reference_count
repeat reference_count:
  bytes source_package_name
  version package_version
```

The entry position is the archive-local reference id used by package type
references. Restore matches each entry against the available packages by name
and version, so caller vector order has no meaning.

This table is a restore dictionary, not the package's authored import list. A
dependency records source visibility; a reference records every external type
table needed to restore canonical pointers.

## Package table

```text
count root_type_id
count local_type_count
repeat local_type_count:
  serialized_type
count terminal_count
repeat terminal_count:
  bytes group
  bytes path
  bytes content
```

`root_type_id` and nested-type ids index the local type table. The reader
allocates every local `Ttx::Type` slot before restoring any body. Forward
references, aliases, recursion, and nested types can therefore recover stable
address identity.

A serialized type is:

```text
bytes name
documentation
attributes
type_ref alias_parent
members
count nested_type_count
repeat nested_type_count:
  count local_type_id
count function_count
repeat function_count:
  bytes name
  documentation
  members parameters
  members results
```

The shared records are:

```text
documentation:
  count line_count
  repeat line_count: bytes line

attributes:
  count attribute_count
  repeat attribute_count:
    bytes key
    bytes value

members:
  count member_count
  repeat member_count:
    bytes name
    type_ref type
    u8 defaulted
    documentation
    attributes
```

`defaulted` is written as zero or one. A member type must resolve to a real
type. The nullable type-reference form is only valid where the data model has a
nullable edge, currently a type with no alias parent.

## Type references

Every type edge starts with one `u8` kind:

| Kind | Value | Payload | Meaning |
| --- | ---: | --- | --- |
| None | 0 | none | No type on a nullable edge. |
| Builtin | 1 | `bytes type_name` | Entry in the standard TTX type table. |
| Local | 2 | `count type_id` | Entry in this Package table's local types. |
| Package | 3 | `count reference_id`, `count type_id` | Type in a package named by the References table. |

Buffers never serialize process pointers. Local and external ids are resolved
back to the real arena-backed TTX objects during restore.

## Linkages table

```text
count linkage_count
repeat linkage_count:
  type_ref owner
  count function_index
  bytes linkage_symbol
```

The owner must restore to a real type. `function_index` selects an entry from
that owner's canonical `get_functions()` table. This keeps symbol publication
attached to the TTX function it implements without storing dialect names,
implementation payload bytes, or registry callbacks.

## Reading and compatibility

`Archiver::Reader` borrows the source buffer and retains no decoded state.
`read_manifest()` and `read_package()` are safe to issue independently because
each call validates the fixed header and seeks through the directory. Returned
objects and their tables are allocated in the caller's arena.

Restore validates:

- magic, exact format version, directory bounds, and table order
- complete consumption of every selected table
- standard type-table version
- dependency package name and version
- local, package, function, and terminal bounds
- non-null member types and valid linkage symbols

Format version 10 has no backward-compatibility reader. Changing a field or
adding a durable table requires a format-version change. A new table receives a
fixed directory slot so unrelated readers can continue to seek directly rather
than scanning variable data.
