# Package Parser

`Tetrodotoxin::Parser::Package` owns the transaction that starts at a package
root. Its `Source` parser consumes that root's `package.ttx`; its `Workspace`
pins the root used to load the explicitly declared package members.

The parser does not double as the package data model. Successful parsing
constructs `Tetrodotoxin::Model::Package::Source` in the Cursor arena, then
returns a reference to that model. The `Parser::Package::Source` class retains
no declarations or transaction state.

## Source contract

A Package document has this order:

```text
required opening Documentation
dialect : Package;
zero or more resolve declarations
zero or more source declarations
end of document
```

The accepted declarations are:

```ttx
resolve Math : Perimortem.Math = "1.0";
source "library/types.ttx";
```

A resolution contains one unique local Alias, one exact package name, and one
canonical non-null Major.Minor Version. The package name is
`Type("." Type)*`. Version text is parsed as two unsigned components and never
enters the Real model. Leading zeroes and `"0.0"` are rejected.

Member routes are quoted, normalized package-relative paths. Empty routes,
absolute routes, empty segments, backslashes, embedded nulls, unresolved
parent escapes, trailing slashes, and duplicate normalized routes are rejected.
Resolutions must precede members.

## Result

Successful parsing returns a `Model::Package::Source` that borrows:

- the opening Documentation;
- ordered `Concept::Package::Resolution` values; and
- ordered normalized member-route views.

There is no Member wrapper because a route view already carries the complete
model fact. There is no body-token index, Cursor bookmark, filesystem path,
opened file, Environment, or resolved Package graph in the result.

Package bodies are currently rejected. When Package body evaluation exists, it
must consume the body directly into semantic state in the same Cursor
transaction. It must not add a token bookmark for a later parser pass.

## Package resolution

The root `package.ttx` is the authored input that resolves one package
abstraction. `Model::Package::Source` records its exact dependency requests and
member routes. The later construction walk resolves those requests, loads those
members from the same Workspace, evaluates their Dialects, and publishes a
`Model::Package::Resolved` graph.

Parser owns the root transaction and token consumption. Model owns the parsed
Package Source, Environment, member Source lifetime, and resolved Package
graph. Archiver, Compiler, and Linker own their respective durable and terminal
products.

Puffer is one application client. Parser and Model code never depend on Puffer
process, protocol, editor-session, or presentation state.

## Workspace

`Tetrodotoxin::Parser::Package::Workspace` pins one opened package-root directory
capability. It accepts only normalized relative routes and performs one
confined same-opened-object transaction:

```text
normalized relative route
-> kernel-enforced beneath-root open
-> classify the opened object
-> read that same regular file completely
-> owning byte snapshot or exact failure
```

The closed failures are missing, non-file, unreadable, and outside-root. A
zero-length regular file is a successful byte snapshot. Failure retains no
partial bytes, selected path, or operating-system handle.

Workspace does not validate one pathname and reopen another. It never falls
back to the process working directory or a Source directory. Internal symlinks
are accepted only when the confined open proves the selected object remains
beneath the pinned root.

## Current surface

The active `//tetrodotoxin:parser` target contains both `Source` and
`Workspace`. The active `//tetrodotoxin:model` target contains the parsed
`Package::Source` and the resolved package capabilities. Repository selection,
Environment assembly, resource caching, recursive restoration, and full
materialization remain unimplemented. Target existence, parsing tests, and
confined-read tests do not establish those later behaviors.
