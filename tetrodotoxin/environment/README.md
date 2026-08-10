# Environment

Environment is Tetrodotoxin's compilation session and semantic lifetime
boundary. Its public `Workspace` groups the Dialects and source results that may
refer to one another, much as a compiler context keeps declarations alive for
one compilation.

The difference is that a Workspace can retain several independently modeled
languages. Package, Library, App, Scene, Render, and Shader Monographs keep
their own semantic shapes while borrowing exact TTX identities across the
common lifetime.

Together those concrete objects form the Workspace's live multi domain semantic
IR. The shared part is identity, category, resolution, and Layout rather than a
common AST or declaration model.

Use Environment when a tool interprets related sources, needs stable cross
language identity, drives grouped completion, presents source Diagnostics, or
restores a Package Archive. A consumer that receives only a completed Terminal
does not need to retain the Workspace. The cost of cheap borrowed identity is a
clear boundary: ordinary References cannot outlive or move between Workspaces.

## Workspace lifetime

A Workspace keeps every installed Dialect and retained Monograph alive while
their semantic identities can be queried. Package Aliases, Library Types,
Function Signatures, and source imports can therefore retain exact identities
produced by other owners.

Each Workspace is independent. Installing the same Dialect in another Workspace
creates another language environment and new Monograph identities. Tools may
correlate those objects through durable names defined by their owners.
Source independent semantic continuity comes from validated Archive facts, not
shared References or a global Type inventory.

References between ordinary Monographs remain inside this lifetime. They are
not durable identifiers and they are never written into a Package Archive. An
import without source creates new objects in the receiving Workspace and
reconnects them using Archive facts defined by their owners.

## Dialect installation

A tool installs the concrete Dialects accepted by one invocation. The exact
installed name is the name authored after `dialect`:

```ttx
//
dialect : Library;
```

Unknown names are source errors. A source never selects a Dialect through a
closed enum of Package kinds or a filename convention.

The Tetrodotoxin toolchain includes Package support, but Package is not
installed into every Workspace automatically. A standalone source request may
install only its selected Dialect. A Package compilation, resource request, or
Archive restoration installs Package together with the concrete Dialects named
by that transaction.

## Direct source import

Direct import supplies three independent facts:

* the semantic name used for Workspace lookup
* the diagnostic path presented to the user
* the source bytes interpreted by the selected Dialect

Environment reads the common envelope, selects the Dialect, and retains the
returned Monograph under the authored semantic name. The path describes origin.
It does not create semantic identity.

The envelope begins with required source Documentation. An explicit empty
comment is valid, but a missing comment is not. Environment passes that exact
Documentation through the selected Dialect and retains it with the Monograph.

## Package import

A Package import begins with one Package manifest and its confined root. The
Package names dependencies and Source members explicitly:

```ttx
resolve Graphics : Perimortem.Graphics = "1.0";
source Scenes::Splash from "scenes/splash.ttx";
```

Package acquires each declared path through its confined Storage. Environment
interprets the member with the selected Dialect, reconstructs exact dependencies
from their Archives, and binds the resulting Monographs through their Package
contexts. Names local to a Package remain inside that Package rather than
entering the Workspace root automatically.

## Linking and publication

Interpretation can reserve declarations before all routes between sources are
known. Environment therefore completes a retained source group in two barriers:

```text
interpret and retain the complete group
-> link every Monograph
-> finalize every Monograph when all links succeed
-> publish completed root names
```

This ordering allows forward references and dependency cycles that the concrete
languages can resolve while preventing a partially completed group from being
published as completed input.

An `Invalid` answer observed before these barriers finish is not a permanent
negative result. Immutable consumers begin after publication. A tool that
chooses to inspect a group during construction must not cache unanswered routes
across a link or finalize transition.

Archive restoration uses the same barriers. Environment does not deserialize
live objects or revive process addresses. It asks each installed Dialect to
construct a new Monograph from its validated payload, retains the complete
group, links every member, finalizes every member, and then publishes the
restored roots.

## Contextual lookup

Workspace is an ordinary TTX Abstract context. Looking up an exact imported root
name returns its retained Monograph. A missing name returns TTX `Invalid`.

Deeper `::` access is interpreted by the returned Abstract contexts. Environment
does not require every Monograph to expose a Type or one common member model.

## Diagnostics

Environment retains the source name and body associated with each authored
Monograph. A concrete language records Diagnostics against its semantic facts.
Environment combines them with that retained origin when presenting errors.

Package paths, Archive bytes, and Repository requests can fail without an
authored Token. Their owners preserve the cause belonging to that domain, and
the caller attaches it to an authored dependency or compile request when such a
source location exists.

## Boundaries

Environment owns semantic lifetime, source dispatch, group completion, and root
publication. Package owns path confinement and durable package products.
Concrete Dialects own source grammar and language semantics. Compilers and
linkers consume completed Monographs without becoming part of Workspace lookup.

See [Language](../language/README.md) for Dialect and Monograph contracts and
[Package](../package/README.md) for package source and resources.
