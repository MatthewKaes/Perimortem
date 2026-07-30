# Package

`tetrodotoxin/package` owns the authored Package Dialect and confined Package
storage. The current target contains the concrete Package body transaction,
stateless statement parsers on the exact Dependency and Source values, the
Package Monograph, and one opened root input owner.

Package depends on Language, TTX, and Perimortem. Environment is intended to
install `Package::Dialect` and retain each interpreted Package Monograph.

## Authored grammar

A Package body contains ordered dependency requests followed by ordered Source
bindings:

```ttx
resolve Math : Perimortem.Math = "1.0";
resolve Graphics : Perimortem.Graphics = "1.0";
resolve System : Perimortem.System = "1.0";

source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
source Main from "main.ttx";
```

Each `resolve` declaration records the local Type shaped name used by this
package, the exact external package name, and the pinned version.

Local semantic names use exact contiguous `Type (:: Type)*` grammar. External
Package names use exact contiguous `Type (. Type)*` grammar. Spacing inside
either qualified name is invalid and is never projected out of semantic text.
The pinned version must be a closed quoted canonical `Major.Minor` value.

`Package::Language::Dependency` is that request, not the fetched package or a
semantic resolution. Its stateless `parse` factory consumes one complete
Resolve statement and returns only a complete Dependency.

After the dependency region, each `source` declaration binds an exact authored
Type shaped semantic name to one package path. The left side is the name used
for cross Source resolution. The right side is only the location opened beneath
the package root. A filename never creates a semantic name implicitly.

`Package::Language::Source` retains that exact pair, and the Package Monograph
retains the Source values in authored order. Its path is delimiter free and
lexically normalized through `System::Path`. Package interpretation copies the
normalized bytes into the graph Arena and does not open the path.

The stateless Source `parse` factory owns that one complete statement. It
retains no Cursor, Token, bookmark, or partial declaration.

Dependencies are optional and must precede Sources. At least one Source is
required. Duplicate Dependency local aliases, duplicate Source semantic names,
and duplicate normalized Source paths are independent Package errors.

## Dialect and Monograph

`Package::Dialect` derives from `Language::Dialect`. Environment selects its
stateful instance after parsing `dialect : Package;` and passes the same forward
Cursor, opening Documentation, graph Arena, and shared registry to
`interpret`.

Dialect selects the Resolve or Source parser, enforces the Dependency before
Source body region, checks duplicates across completed values, requires at
least one Source, and constructs the final Monograph only after the complete
body transaction succeeds.

The implemented interpretation contract constructs one
`Package::Language::Monograph` in the Environment Arena after a successful
transaction. The Monograph retains its opening Documentation, its host Package
Dialect, ordered exact Dependency requests, and ordered Source bindings.

It retains no filesystem handle, downloaded dependency, opened member
Monograph, compiler product, or archive entry.

Any failed Package transaction constructs and publishes no Package Monograph.
Interpretation continues across recoverable statement failures so independent
diagnostics remain visible. Existing diagnostics from another source do not
decide the Package transaction.

## Package root policy

`Package::Storage` is the physical companion to the authored Package model. A
`Package::Language::Source` retains one semantic name and normalized logical
route. Storage resolves only that route into a diagnostic path and content
bytes. It never interprets the bytes or derives semantic identity from a
filesystem name.

Storage borrows the Workspace Arena and owns exactly one opened
`System::File::Root`. Its generic read operation serves Sources and embedded
resources. Each logical route is normalized once at the Package boundary. The
canonical relative form is then the cache key, diagnostic path, and confined
read route. Absolute, rooted, escaping, empty, and NUL bearing routes are
rejected.

Only successful reads enter the managed cache. Each canonical route constructs
one public `Storage::Content` value in the supplied Arena. Content contains
only the retained diagnostic path and byte views. Equivalent normalized routes
return the same Content reference. Distinct routes remain distinct even when
their contents or filesystem object are equal. Empty bytes remain a successful
retained value. Later file mutation, replacement, removal, root pathname
movement, caller route mutation, cache growth, and Storage movement do not
change existing Content.

Storage lives while one physical Package can still be read. It may close after
Workspace staging because every returned Content belongs to the Workspace
Arena and remains valid for that semantic island lifetime. Workspace pairs the
Source semantic name with Content when it imports the Source. Storage does not
search the process working directory or resolve relative to a containing
Source. Content outside the opened root is available only through an exact
resolved Dependency.

The current Monograph model still retains authored Source bindings only.
Package Storage does not import members, construct Library Constants,
interpret semantic facts, resolve dependencies, or select an App.

`main.ttx` is only a filename convention. Future package assembly selects the
sole completed App Monograph regardless of its local Source name or member
filename.

## Archive and repository

Package owns the planned `Package::Archive` envelope, reader, writer, and exact
repository selection. The Archive is the durable semantic terminal for source
free restoration. It is distinct from every Linker native product.

An Archive contains:

1. exact Package identity and pinned version;
2. exact semantic member names and concrete Dialect names;
3. opaque versioned payloads encoded and restored by each concrete Dialect;
4. dependency requests needed to rebuild the imported Package root;
5. exported semantic routes and their native artifact and symbol locators.

It contains no source bytes, source path as semantic identity, process address,
parser state, filesystem handle, target cache, or Linker object bytes.

Restoration validates the envelope, then asks the installed concrete Dialect to
allocate and restore its real Monograph in the importing Workspace Arena.
Package never depends on a concrete payload schema.

The exact Repository selects only explicitly supplied products by Package
identity and pinned version. It provides semantic Archives to Workspace and
native product paths to Puffer as separate values. It does not scan the current
directory, fetch a latest version, or load native bytes during semantic
restoration.

No Archive codec, exact Repository, publication path, or source free
restoration exists in the current Package target. A Package Monograph currently
proves only the intended authored manifest result shape.
