# Environment

Several languages can describe one program only if their results stay alive
long enough to meet. Environment provides that meeting place. Its `Workspace`
opens related sources, keeps their semantic objects alive, and gives references
between them one clear lifetime.

A Workspace can bring Package, Library, App, Scene, Pipeline, and Shader sources
together without asking any of them to become the others. TTX supplies the
shared identities, Types, Layouts, and Callables they use to cooperate.

Tools come here to complete direct source, interpret a Package source table,
report errors, or restore a Package Archive. Once an executable or another
Terminal product is complete, it can leave the Workspace and carry only the
finished representation it needs.

## Toolchain and Workspace lifetime

A Toolchain owns one immutable installed Dialect graph. The host constructs it
once, including every downward Dialect dependency, and passes it to each
Workspace. A Workspace owns only its Monographs and lookup state. Package
Aliases, Library Types, Function Signatures, and source imports can therefore
refer directly to objects produced by another language while the Toolchain
remains reusable across graph replacements.

This capitalized `Toolchain` owns the semantic side of composition. Puffer, a
build, or another host composes the complete toolchain by pairing those Dialects
with the Terminal producers requested for an operation. Producers begin after
Workspace completion, so their target and format state stays outside the
reusable Dialect graph.

Each Workspace is independent even when it borrows the same Dialect identities.
Interpreting the same source in another Workspace creates new Monograph
identities. Tools may correlate those objects through durable names defined by
their owners.
An Archive can rebuild equivalent source results in another Workspace. It does
not keep the original references alive or rely on a global Type list.

References between ordinary Monographs remain inside this lifetime. They are
not durable identifiers and they are never written into a Package Archive. An
import without source creates new objects in the receiving Workspace and
reconnects them using Archive facts defined by their owners.

## Dialect installation

A host installs the concrete Dialects accepted by one toolchain. The exact
installed name is the name authored after `dialect`:

```ttx
//
dialect : Library;
```

Unknown names are source errors. A source never selects a Dialect through a
closed enum of Package kinds or a filename convention.

Package support is not injected into every Toolchain automatically. A
standalone source tool may install only its selected Dialect. A Package
compiler, resource tool, or Archive restorer installs Package together with the
concrete Dialects it supports before creating any Workspace.

Some Dialects require another Dialect. Scene requires Library, while Shader
requires Library and Pipeline. Toolchain creates each dependency once and gives
the shared instance to every language that needs it. A missing dependency is a
configuration error, and dependency loops are rejected.

## Direct source import

Direct import supplies three independent facts:

* the stable name used for Workspace lookup
* the path shown in errors
* the source bytes read by the selected Dialect

Environment reads the common envelope, selects the installed Dialect, and
copies the path and source bytes into one source transaction Arena. It
constructs the Tokenizer, Associations index, and operation local Cursor there.
The selected Dialect receives that Cursor and returns an optional Monograph
reference. A returned Monograph is the strongest semantic root the operation
could establish. Reports added to the Cursor's Errors decide whether that root
can publish, while linking can still enrich the retained graph for tooling. The
path describes origin. It does not create semantic identity.

The envelope begins with required source Documentation. An explicit empty
comment is valid, but a missing comment is not. Environment passes that exact
source backed Documentation, its Anchor, the Cursor, and semantic context
directly to the selected Dialect. Source backed Comments and Attributes remain
valid because the Monograph and source bytes occupy the same Arena.

Workspace retains one source record containing the transaction Arena, exact
outer Monograph, Tokens, diagnostics, and immutable Associations index whenever
interpretation establishes that Monograph. Workspace uses the same Cursor to
link every fact the current graph can support. Finalization begins only when
interpretation and linking complete without errors. An incomplete result stays
available to editor queries but cannot enter a Terminal, Archive writer, or
another immutable Terminal product. A Package root enters through Package
import so Workspace can walk its complete reachable Type graph as one island.

## Package import

A Package import begins with one Package source and its confined root. Common
imports name every local source and external Package Type:

```ttx
private Splash : alias = source("scenes/splash.ttx");
public Graphics : alias =
    package(.name = "Perimortem.Graphics", .version = "1.0");
```

Workspace resolves each source path relative to its importer, canonicalizes it,
and reuses one cached file and Monograph when equivalent spellings reach the
same route. Each distinct source gets one transaction Arena and its concrete
Dialect. A source import may extend the same graph; a Package import terminates
the local walk at one exact completed Package fact.

The build request supplies completed Package facts before a consumer links.
Editor sessions may inspect Workspace's unresolved exact Package requests,
acquire them from one Repository, detect request cycles, and rebuild the island
in dependency order. Repository selects a caller supplied local root or the
versioned installed root for the exact requested coordinate. Workspace does not
invent a standard dependency set or derive a Package identity from a filesystem
path.

The host may retain immutable filesystem snapshots independently from any one
source graph transaction and lend them to replacement Workspaces. Package still
owns logical routing, confinement, and the decision to request one canonical
source or resource path. Snapshot storage keys the
resulting snapshot by the exact Package root and canonical route, retains the
bytes outside the replaceable graph Arenas, and records a fingerprint obtained
from the same opened filesystem object that supplied those bytes. A later full
graph replacement probes that fingerprint and reuses the immutable bytes only
when the opened object is unchanged. A changed object causes one complete reread
before the new graph can publish.

This cache is nonsemantic Workspace state. It never resolves a resource name,
keeps a rejected graph alive, or lets Library open a file. Replacing a document
still constructs a complete new graph transaction. Persistence avoids repeated
I/O rather than introducing an incremental semantic graph.

## Linking and publication

A direct source retains its strongest result before `interpret_source` returns:

```text
interpret to one optional Monograph in the source Arena
-> retain its lexical and semantic evidence
-> link the meaning available from the retained graph
-> finalize only a completely linked error free island
```

Workspace owns the one staged multiple source operation and the local candidate
Arena handles. It walks common external Type edges, retains every Monograph it
can create, and links the acyclic graph dependency first. This keeps the
strongest definitions and inferred Types available while the user edits.
Finalization waits until every member completes interpretation and linking
without errors. Only that completed island can enter Terminal production. The
Package Monograph owns its restricted Library export surface and no parallel
member inventory.

A Package Alias binds only an exact identity and version already completed in
the same Workspace. A terminal may acquire that product and rebuild the source
island, while Archive reconstruction remains an explicit source-free operation.

A Monograph may contain child layers from its dependencies. Environment keeps
and publishes the outer Monograph, while the outer language moves its children
through the same linking and finalization steps. Tools ask the outer Monograph
for a layer instead of looking for another Workspace name.

During Archive reconstruction, the Package Monograph is created before its
members. Every member receives that Package context, including the real Library
child inside Scene. Shader relationships resolve the separately restored
Library and Pipeline members through that same context. Language dependencies
still come from the Workspace. If a real child layer cannot be restored, its
outer member also fails.

## Contextual lookup

Workspace is an ordinary TTX Abstract context. Looking up an exact imported root
name returns its retained Monograph. A missing name returns TTX `Unknown` while
the graph may still acquire that Package.

Deeper `::` access is interpreted by the returned Abstract contexts. Environment
does not require every Monograph to expose a Type or one common member model.

## Failure reporting

Each authored source is paired with its text for the complete parse, link, and
finalize operation. The source and every fixed child layer write textual errors
through the matching operation local Cursor to the caller's textual error sink,
preserving order and exact authored locations. Successful publication retains
the Associations index beside the exact outer Monograph and does not expose the
spent Cursor. A later compiler receives the exact source path, source bytes, and
error sink needed for its own source attributed reports. A Monograph never
retains or serializes a Cursor.

Package paths, Archive bytes, Repository requests, and other source free system
or toolchain operations report through Perimortem Diagnostics. When an authored
Package request exists, Package instead reports the failure at that request's
Cursor location. No Environment Diagnostic object or throwaway Workspace error
collection is created.

## Boundaries

Environment owns semantic lifetime, source transactions, source dispatch,
source graph completion, canonical path reuse, and root publication. Package
owns its restricted export surface, path confinement, resources, and durable
products.
Concrete Dialects own source grammar and language semantics. Compilers and
linkers consume completed Monographs without becoming part of Workspace lookup.

See [Language](../language/README.md) for Dialect and Monograph contracts and
[Package](../package/README.md) for package source and resources.
