# Puffer

Puffer is Tetrodotoxin's command-line compiler and language-server host. It
loads complete TTX source files, evaluates Boot preambles, resolves source and
package imports, runs selected body ISAs, and asks the compiler toolchain to
produce durable artifacts.

Puffer owns source orchestration. The reusable package reader and writer live
in [`../archiver`](../archiver/) under `Tetrodotoxin::Archiver`. They know the
Puffer Buffer format but do not depend on the CLI, resolver, filesystem, or
compiler transaction.

## Compilation Transaction

A build starts with an explicit package or unit Route, dependency buffers, and
source paths. The Toolchain supplies the ClassDB, ISA registry, and terminal
targets. One Compiler borrows those immutable schemas and owns every Abstract,
Route, Layout, Address, diagnostic, and output created during the transaction.

A standalone Library build resolves each supplied source root. A package build
requires one `package.ttx` root and walks its complete source closure. Package
imports come from buffers registered by the caller. Resolution never guesses a
package source path when a buffer is missing.

Each selected ISA constructs or enriches Abstract-derived objects in the same
Compiler-owned graph. Puffer does not collect a second Type tree, pointer-keyed
Implementation table, or publication projection. Terminal planners consume the
same Type, Callable, Layout, Address, and ISA contracts that resolution
published.

The transaction returns completed artifacts. Puffer does not retain a previous
build's Compiler objects or local handles.

## Public Routes And ABI

Puffer preserves the authored route through every import, group, Alias, Type,
and Callable query. A canonical Type may have several routes; canonicalization
does not erase the route used to reach it. Package publication explicitly
selects public routes from the authored package surface.

Free and Self callables occupy registered contract layers:

```text
Widget / Callable.Free / open
Widget / Callable.Self / open
```

The route already records the invocation distinction. Puffer does not
synthesize `.Type` or `.Addressable` suffixes, infer a surface from a parameter,
or choose the lexicographically first Alias.

Public and internal machine names are reversible encodings of selected Routes.
They do not hash package names, signatures, canonical Type names, or content.
Explicit route segments carry package and ABI versions when incompatible
versions must coexist.

Only public and exposed Callable Addresses enter the package surface. Private
Addresses remain local to the compiler product. Restored packages, foreign
runtimes, and local bodies expose the same Address contract, so Puffer does not
branch on the producer.

## Package Products

The package builder presents the Archiver with:

- the authored package Route and explicit version
- dependency aliases, Routes, and versions
- required ClassDB schema Routes
- the reachable Abstract graph and contract-qualified edges
- authored and public Resolution routes
- recursive Layouts
- public Callable Addresses
- terminal products produced by ISA lowerers.

The resulting `.puffer` file is a Tetrodotoxin package snapshot. It is not an
object-file extension and it is not the C++ ABI. A companion native archive may
contain machine code, while generated language interfaces are independent
terminal projections over the same routes and Layout-described calls.

An authored package identity can name an output directory:

```text
Perimortem.Math/binary_archive.puffer
Perimortem.Math/x86_64.a
Perimortem.Math/cpp_abi.hpp
```

These paths describe package products rather than becoming semantic identity.
Another build system may publish the same package Route and version.

## Resolution Ownership

The resolver owns the source and package dependency graph for one workspace.
Boot evaluates the preamble, the resolver binds imports, and the selected body
ISA evaluates the remaining bytecode. A source record becomes a valid cache hit
only after that work produces a complete root Abstract. Failed evaluation
retains an Invalid root for diagnostics but cannot expose stale semantic state.

Updating a source invalidates every consumer that may retain handles into its
Compiler boundary. Re-evaluation creates a new graph; process addresses never
serve as durable package identity.

Package buffers are registered by package Route and explicit version. Restore
allocates their Abstract graph inside the current Compiler, reconnects imports,
and publishes their public routes into the same resolution graph as source
records. Unknown schemas, corrupt edges, or incompatible versions produce an
Invalid package root rather than a partially null DAG.

The archive remains the owner of serialization rules. See
[`../archiver/README.md`](../archiver/README.md) for the durable graph and
compatibility contract.
