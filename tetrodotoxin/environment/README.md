# Environment

Several languages can describe one program only if their results stay alive
long enough to meet. Environment provides that meeting place. Its `Workspace`
opens related sources, keeps their semantic objects alive, and gives references
between them one clear lifetime.

A Workspace can bring Package, Library, App, Scene, Pipeline, and Shader sources
together without asking any of them to become the others. TTX supplies the
shared identities, Types, Layouts, and Callables they use to cooperate.

Tools come here to validate direct source, follow a package's source imports,
report errors, or restore a Package Archive. Once an executable or another
Terminal product is complete, it can leave the Workspace and carry only the
finished representation it needs.

## Toolchain and Workspace lifetime

All installed languages enter Workspace through the same provider contract.
Workspace retains source generations and controls their publication, while each
provider owns its graph storage. Native C++ frontends can keep using Arenas and
Cursors without requiring foreign frontends to reproduce those objects.

Source lookup reaches a stable authority, whose current graph changes when that
source is replaced. Callers that retain producer identities across an edit keep
the corresponding source generations alive explicitly. Retaining the source
closure for an observation covers flow assembled from several sources. Once
those borrows end, releasing the generation lets its provider reclaim it.
Workspace does not keep an edit history solely to make stale pointers usable.

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

Workspace supplies retained input bytes to the installed provider and adopts
its returned source graph. A native provider constructs its Tokenizer, Cursor,
and semantic objects in provider owned storage. Foreign providers expose the
same root and typed source services without adopting that allocation strategy.
The path describes origin, while the stable Workspace route determines where
consumers ask for the current graph.

Native TTX sources begin with required documentation. An explicit empty comment
is valid, but a missing comment is not. The provider passes that documentation,
its anchor, and the source context to the concrete Dialect. Retaining the source
input keeps borrowed comments and attributes valid for the graph's lifetime.

Publishing a generation makes its current answers available to tooling. Even
when interpretation cannot establish a root, the provider can preserve useful
diagnostics behind that generation's source services. Read only validation then
determines whether the relationships required by an immutable product are
complete. Source publication and artifact publication are separate boundaries.

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
same route. Each distinct source gets its own provider owned generation. A
source import may extend the live graph, while a package import reaches an
exact supplied immutable package authority.

The build request supplies the packages needed by the requested production.
Editor sessions may inspect unresolved package requests and acquire them from
the configured repository. Consumers query the supplied authority again instead
of rebuilding their own graphs. Repository selects a caller supplied root or the
versioned installed root for the exact requested coordinate. Workspace does not
invent a standard dependency set or derive a Package identity from a filesystem
path.

The host may retain immutable filesystem snapshots independently from any one
source generation and lend them to later interpretations. Package still
owns logical routing, confinement, and the decision to request one canonical
source or resource path. Snapshot storage keys the
resulting snapshot by the exact Package root and canonical route, retains the
bytes outside the replaceable provider graphs, and records a fingerprint obtained
from the same opened filesystem object that supplied those bytes. A later source
replacement probes that fingerprint and reuses the immutable bytes only
when the opened object is unchanged. A changed object causes one complete reread
before the new graph can publish.

This cache is nonsemantic Workspace state. It never resolves a resource name,
keeps a rejected graph alive, or lets Library open a file. Replacing a document
constructs another generation for that source. Unrelated sources stay alive and
references requery the replaced authority. The byte cache avoids repeated I/O
without owning those semantic answers.

## Validation and publication

Interpretation creates one generation, dependency acquisition supplies its
authorities, and Workspace publishes it under the stable source name. Subsequent
queries can reach a dependency that was unavailable during interpretation.
Validation observes those current answers without changing the graph or adding
a second completion phase.

An immutable producer validates the source closure required for its output and
retains those generations until its borrowed identities are no longer needed.
A package's export surface describes what belongs to that closure without
copying Workspace's source inventory.

A Monograph may contain child layers from its dependencies. The outer provider
keeps those children alive and validates them through the same source services.
Tools ask the outer graph for the promised layer instead of inventing another
Workspace identity for it.

During Archive reconstruction, the Package Monograph is created before its
members. Every member receives that Package context, including the real Library
child inside Scene. Shader relationships resolve the separately restored
Library and Pipeline members through that same context. Language dependencies
still come from the Workspace. If a real child layer cannot be restored, its
outer member also fails.

## Contextual lookup

Workspace is an ordinary TTX Abstract context. Looking up an imported root name
reaches its stable source authority, which resolves to the currently retained
root. A missing source remains Unknown while it may still be supplied.

Deeper `::` access is interpreted by the returned Abstract contexts. Environment
does not require every Monograph to expose a Type or one common member model.

## Failure reporting

Each provider preserves the source information needed for its diagnostic and
association services. Native owners write reports through an operation local
Cursor, while consumers receive ordered messages, source spans, and exact
Abstract identities through the shared typed boundary. A later compiler obtains
the source information and reporting authority it needs without retaining the
parser's Cursor.

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
