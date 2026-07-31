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
`Package::Language::Parser::Name` owns both authored qualified name entry
points while the TTX Lexicon validates every Type segment and the exact
separator Code between segments.
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
resources. `System::Path` owns lexical normalization. Storage uses the
canonical relative form as the cache key, diagnostic path, and confined read
route. Absolute, rooted, escaping, empty, and NUL bearing routes are rejected.

Only successful reads enter the managed cache. After a cache miss succeeds,
`Path::normalize` constructs the stable canonical route directly in the
supplied Arena and Storage constructs one public `Storage::Content` over that
route and the read bytes. Cache hits and failed reads consume no additional
path storage. Equivalent normalized routes return the same Content reference.
Distinct routes remain distinct even when their contents or filesystem object
are equal. Empty bytes remain a successful retained value. Later file
mutation, replacement, removal, root pathname movement, caller route mutation,
cache growth, and Storage movement do not change existing Content.

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

Namespace `Package::Archive` owns the durable `Archive` value, Format 1
`Reader`, and canonical `Writer`. `Package::Archive::Archive` is the semantic
terminal for later source free restoration and is distinct from every Linker
native product.

An Archive contains:

1. exact Package identity and pinned version;
2. ordered exact dependency requests;
3. ordered semantic member names, concrete Dialect names, and opaque payloads;
4. ordered logical native artifact IDs;
5. ordered exported semantic routes and their artifact and symbol locators.

It contains no source bytes, source path as semantic identity, process address,
parser state, filesystem handle, target cache, or Linker object bytes.

Archive is a regular value over stable views supplied by its producer. Its
constructor preserves those views and their order without copying storage or
applying Format 1 validation. A direct producer obtains each opaque member
payload through its concrete `Language::Dialect::encode` operation and keeps
the described storage alive. Archive does not enumerate a Workspace, accept a
Package Monograph or Source, interpret a payload, or depend on a concrete
Dialect. An engaged empty payload remains a valid member payload.

The reader validates and materializes only. Its accepted input remains borrowed
while the typed record ranges live in the caller Arena. The caller keeps that
input valid until the Arena is reset or destroyed and keeps the Arena alive
while it holds the returned Archive value. A caller with shorter lived input
copies it into the Arena once before reading, while an Arena backed file read
passes its existing view directly. Reader logs the exact failing Format stage,
byte offset, section tag, invalid value, duplicate name, or unknown reference
through `Diagnostics::Log` and returns absence. It does not construct a textual
source error because binary Archive bytes provide no authored token context. A
later Workspace restoration transaction will attach that failure to the
authored Dependency request before selecting the installed Dialect and calling
its `restore` operation.

### Format 1

All unsigned integers are fixed width and little endian. The file starts with
this twelve byte header:

| Offset | Width | Value |
| --- | ---: | --- |
| 0 | 4 | ASCII `TTXA` |
| 4 | 2 | format value `1` |
| 6 | 2 | reserved flags `0` |
| 8 | 4 | complete body size |

The body is a tagged singleton field envelope. Every field starts with an
unsigned 16 bit tag, unsigned 16 bit flags, and unsigned 32 bit payload size.
Flag bit 0 marks a required field and every other bit is reserved.

The six known fields are required, occur exactly once, and occur in this
canonical order:

`Package::Archive::Archive::Sections` is the public source for these tags. Its
closed values use `Unsigned_8` in memory and are widened to the existing
unsigned 16 bit tag field on the wire. `Archive::header_size` publishes the
twelve byte fixed header size used by both Reader and Writer.

| Tag | Payload |
| ---: | --- |
| 1 | Package identity string |
| 2 | unsigned 16 bit major and unsigned 16 bit minor Package version |
| 3 | dependency list |
| 4 | member list |
| 5 | native artifact list |
| 6 | export list |

A missing, repeated, reordered, or incorrectly flagged known field rejects the
Archive. An unknown field with the required bit rejects. An unknown optional
field is skipped only when its complete declared payload remains inside the
body. The writer emits no unknown fields.

Strings and opaque member payloads start with an unsigned 32 bit byte size.
Every list starts with an unsigned 32 bit count. Each list entry then starts
with an unsigned 32 bit record size. A dependency record contains its local
alias string, external Package identity string, unsigned 16 bit major, and
unsigned 16 bit minor. A member record contains its semantic name string,
Dialect name string, and opaque payload bytes. An artifact record contains one
logical artifact ID string. An export record contains its semantic route
string, artifact ID string, and symbol locator string.

Every declared field, record, string, and payload is consumed exactly. The
header body size must describe the complete remaining input, so trailing bytes
reject. Counts, record sizes, string sizes, and payload sizes use unsigned 32
bit framing. Every addition, multiplication, allocation, and slice is checked
against that framing and the remaining input before it occurs.

Package identities use dot separated Type segments. Dependency aliases and
member semantic names use `::` separated Type segments. Dialect names contain
one Type segment. Each Type segment has the exact
`[A-Z][A-Za-z0-9_]*` byte shape. Export semantic routes remain opaque to
Package; their deeper legality belongs to their later semantic owner. Package
and dependency versions reject the reserved `0.0` value.

Dependencies may be empty. Native artifact and export inventories may be
empty. Members must not be empty. Dependency aliases, member semantic names,
artifact IDs, and export semantic routes are each unique in their inventory.
Every artifact ID, export semantic route, export artifact ID, and symbol
locator is nonempty and contains no NUL byte. Every export references an
artifact ID declared in the same Archive. Equal member payload bytes remain
independent member facts.

The writer accepts only a validated Archive. It preserves every supplied list
order, preserves zero length member payloads, checks the complete encoded size
before allocation, and always emits the header and six known fields above.
Equivalent facts therefore produce byte identical output regardless of their
original backing allocations.

### Repository and future restoration

Namespace `Package::Repository` owns the `Repository` transaction and its
`Input`, `Artifact`, and `Output` declaration values. One Repository borrows
explicit Bazel-supplied declarations whose lifetime maps to its caller Arena.
A caller with shorter-lived storage proxies those declarations into the Arena
before construction. Each Input declares an expected Package identity, pinned
Version, Archive filesystem location, and exact Artifact mappings from ID to
native filesystem location. Identity and Version are the Input lookup key;
input paths remain opaque locations and never supply semantic identity.
Duplicate exact input keys reject construction.

Exact Archive selection reads only the matching declaration into the caller
Arena and passes those stable bytes to `Archive::Reader`. Repository verifies
the decoded identity and Version and requires exactly one declared native path
for every ordered Archive artifact ID before caching the successful Archive.
Missing keys remain ordinary absence. A missing, empty, corrupt, or mismatched
selected input logs its exact Package key, Archive location, and failure stage,
while every unselected declaration remains inert. Native mapping failures name
the missing, duplicate, or unknown artifact ID and every available filesystem
location. Native lookup validates the semantic Archive and then returns only
the borrowed exact path. It never reads native bytes.

Archive and native inventories use the same Output value and exact Package
identity, Version, and artifact ID key while retaining separate lookup
operations. Repository rejects empty, rooted, escaping, NUL-bearing,
backslash-bearing, or oversized routes. `System::Path` constructs each accepted
slash-normalized relative route directly in the caller Arena. Repository
applies one key and route collision domain across both output kinds. Duplicate
and collision logs name both product keys, both authored routes, and the shared
normalized destination. Repository creates no directory and writes no product.
Puffer remains the future owner of physical publication and the future source
diagnostic for an invalid compile request.

Archive restoration and the source free Workspace transaction do not yet
exist. `Package::Dialect` encode and restore also remain future work because
the Package root will be reconstructed from Archive envelope metadata rather
than stored as a member payload.
