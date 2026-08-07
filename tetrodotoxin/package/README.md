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
Resolve statement and returns only a complete Dependency. The parser also
exposes the exact successfully consumed `Ttx::Lexical::Span` to Package
Dialect. Dependency retains no Token, Span, or source provenance.

After the dependency region, each `source` declaration binds an exact authored
Type shaped semantic name to one package path. The left side is the name used
for cross Source resolution. The right side is only the location opened beneath
the package root. A filename never creates a semantic name implicitly.

`Package::Language::Source` retains that exact pair, and the Package Monograph
retains the Source values in authored order. Its path is delimiter free and
lexically normalized through `System::Path`. Package interpretation copies the
normalized bytes into the graph Arena and does not open the path.

The stateless Source `parse` factory owns that one complete statement and
returns only a complete Source. A separate output `Ttx::Lexical::Span` covers
the successfully consumed Source through its EndStatement Token and remains
invalid on failure. That range is used for transaction diagnostics and is not
retained on Source or Monograph.

Dependencies are optional and must precede Sources. At least one Source is
required. Duplicate Dependency local aliases, duplicate Source semantic names,
and duplicate normalized Source paths are independent Package errors. A
Dependency local alias colliding with a Source semantic name rejects the
complete transaction over the full offending Source statement.

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
Dialect, ordered exact Dependency requests, one aligned lexical Span per
request, and ordered Source bindings. Its exact Package local scope binds
completed source Monographs and restored Package roots through real
`Ttx::Model::Alias` objects allocated in that same Arena.

Authored construction rejects a Dependency and span count mismatch before
publishing a Monograph. The explicit source free construction path accepts no
spans or Source bindings and therefore exposes an empty span inventory without
asking Workspace to infer provenance from its size.

`bind_member` accepts an authored Source name or one source free Archive member
name and its completed Monograph. `bind_dependency` accepts one retained
Dependency request and its completed Package root. Both operations reject empty
names, duplicates, direct cycles, undeclared authored member names, undeclared
Dependency requests, and cross-kind collisions before allocating another
Alias. The first valid edge therefore remains stable.

`get_members` exposes successful member bindings in authored staging or source
free Archive restoration order. Each nonnull Reference borrows the exact
Arena-owned Alias also returned by `resolve_context`. Dependency Aliases remain
queryable through that exact lookup but do not enter the member view. The view
contains no declaration flattening, transitive dependency import, or
publication policy.

For an ordinary semantic name, `resolve_context` performs one exact byte lookup
and returns the stored Alias edge itself. Missing, partial, differently
qualified, and alternate spelling queries return the shared TTX Invalid
identity. It does not split `::`, infer a hierarchy, normalize a name, or copy
a target into another semantic model.

The complete `$[...]` spelling is reserved before that ordinary map lookup. It
is a contextual resource instruction rather than a Package member name.
Package gives its interior logical route to the Package-owned resource
transaction and returns the stable `Tetrodotoxin::Language::Resource` or
owner-specific `Tetrodotoxin::Language::Error` Abstract selected by that
transaction. Malformed Embedded syntax never reaches Package resolution.

The Monograph retains no filesystem handle, downloaded dependency product,
Archive bytes, Repository state, source text, path, Token, diagnostic state,
compiler product, or archive entry.

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

The approved read boundary returns
`Utility::Result<Content&, Storage::Failure>`. Failure owns one allocation-free
`System::Path` value when lexical normalization could establish a route and one
nested `Failure::Error`. It never retains a view into a temporary Path. Input
with no valid normalized route remains an explicit invalid-route failure rather
than receiving an invented path. Result exposes exactly the stable Content
reference or one owner-specific Failure, and failed reads remain retryable
rather than entering the successful Content cache. Failure::Error distinguishes
InvalidRoute from Unreadable. The current Root capability exposes no narrower
physical cause, so Storage does not manufacture missing, directory, or symlink
categories.

Only successful reads enter the managed cache. After a cache miss succeeds,
`Path::normalize` constructs the stable canonical route directly in the
supplied Arena and Storage constructs one public `Package::Content` over that
route and the read bytes. Cache hits and failed reads consume no additional
path storage. Equivalent normalized routes return the same Content reference.
Distinct routes remain distinct even when their contents or filesystem object
are equal. Empty bytes remain a successful retained value. Later file
mutation, replacement, removal, root pathname movement, caller route mutation,
cache growth, and Storage movement do not change existing Content.

Storage lives while one physical Package can still be read. It may close after
Workspace completes authored source interpretation and resource resolution
because every returned Content belongs to the Workspace Arena and remains
valid for that semantic island lifetime. Workspace pairs the Source semantic
name with Content when it imports the Source. Storage does not search the
process working directory or resolve relative to a containing Source. Content
outside the opened root is available only through an exact resolved
Dependency.

Package Storage does not import members, construct Library Constants,
interpret semantic facts, resolve dependencies, or select an App. Workspace
will bind staged members and restored dependency roots through the Package
Monograph operations. Those Package local edges require no publication in
Workspace's independent source map.

## Contextual resources

An authored Package resource transaction starts pending because its Monograph
is constructed by the Package Dialect while Workspace owns the physical
Storage. Workspace connects the transaction once after manifest interpretation
and keeps that borrow active while it interprets the Package member Sources. A
complete `$[...]` request gives its interior route to Storage and caches one
stable Abstract identity per equivalent owner-normalized request in the
Workspace Arena. A successful read, including zero bytes, constructs Language
Resource. A recognized confinement or acquisition failure constructs a
Package-owned error identity implementing Language Error. When invalid input
has no normalized route, its exact complete instruction is the failure key.
Repeating the same query returns that Resource or Error rather than reopening
the file or allocating another semantic result.

The logical route is cache, confined input, and diagnostic data. It never
becomes a Source or Package member semantic name, and equal bytes reached
through distinct routes do not collapse their Resource identities. Workspace
seals every transaction before dependency resolution can link or finalize the
retained range. Sealing permanently removes the Storage borrow while cached
identities remain valid and new requests return Invalid. Source-free Package
restoration publishes an already sealed transaction. Package Monograph retains
no filesystem handle.

Concrete consumers own the meaning of Resource bytes. Library Literal
constructs the complete base Bytes Constant. Library Expression may fold a
following slice into a Constant containing only the reachable result. Shader
may construct a shader-specific fact. Package constructs neither and persists
no root, route cache, unused bytes, or resource acquisition machinery into
Archive.

`main.ttx` is only a filename convention. Future package assembly selects the
sole completed App Monograph regardless of its local Source name or member
filename.

## Archive and repository

Namespace `Package::Archive` owns the durable `Archive` value, Format 1
`Reader`, its stable `Archive::ReadError`, and canonical `Writer`.
`Package::Archive::Archive` is the semantic terminal for later source free
restoration and is distinct from every Linker native product.

An Archive contains:

1. exact Package identity and pinned version;
2. ordered exact dependency requests;
3. ordered semantic member names, concrete Dialect names, and opaque payloads;
4. ordered logical native artifact IDs;
5. ordered exported semantic routes and their artifact and symbol locators.

It contains no source bytes, source path as semantic identity, process address,
parser state, lexical Span, filesystem handle, target cache, or Linker object
bytes.

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
through `Diagnostics::Log` and returns
`Utility::Result<Archive, Archive::ReadError>`. Result exposes exactly the
Archive or its typed rejection. `Archive::ReadError::UnsupportedFormat`
identifies a readable envelope header with a format revision other than 1.
Empty input and every other malformed or semantically invalid Format 1 input
select `Archive::ReadError::InvalidFormat`. The error contains no offset, tag,
value, or inventory detail because those facts remain in the Debug record.
Reader does not construct a textual source error because binary Archive bytes
provide no authored token context. A later
Workspace restoration transaction will attach that failure to the authored
Dependency request before selecting the installed Dialect and calling its
`restore` operation.

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
artifact IDs, and export semantic routes are each unique in their inventory. A
Dependency alias must not equal a member semantic name because both names
occupy the same Package scope. Every artifact ID, export semantic route, export
artifact ID, and symbol locator is nonempty and contains no NUL byte. Every
export references an artifact ID declared in the same Archive. Equal member
payload bytes remain independent member facts.

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
the decoded identity and Version before caching the successful Archive. It
returns `Utility::Result<const Archive&, SelectionError>`, distinguishing
undeclared, unreadable, invalid format, unsupported format, and Package key
mismatch outcomes plus an unknown Reader fallback. A valid semantic Archive
requires no native declaration and remains cached after a later native failure.
`SelectionError` uses `Unsigned_8` storage, reserves the all ones value for
`Unknown`, and starts ordinary recovery categories at zero.

Native selection first consumes that typed semantic result, then requires
exactly one declared native path for every ordered Archive artifact ID. Missing,
duplicate, or unknown mappings select `ArtifactMismatch`; an undeclared
requested artifact in an otherwise complete inventory selects
`ArtifactNotDeclared`. Native selection returns
`Utility::Result<View::Bytes, SelectionError>` and never reads native bytes.

Reader retains exact format and semantic detail in Debug evidence. Repository
emits one Info record for each selected error with the requested key, selected
Archive location, and every relevant artifact declaration fact available at
that boundary. Propagating a semantic failure through native selection retains
its category and record without another diagnostic. Every unselected declared
file remains inert.

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
