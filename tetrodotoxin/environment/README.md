# Environment

Environment manages a Tetrodotoxin compilation session. Its `Workspace` keeps
related languages and source results alive so they can safely refer to one
another.

A Workspace can contain Package, Library, App, Scene, Render, and Shader sources
at the same time. Each language keeps the model that fits its own job. TTX gives
them a shared way to refer to identities, Types, Layouts, and Callables without
forcing them into one syntax tree.

Tools use Environment to read a group of sources, connect references between
them, report source errors, or restore a Package Archive. A finished output such
as an executable no longer needs the Workspace. Ordinary references cannot be
moved to another Workspace or kept after their Workspace is destroyed.

## Workspace lifetime

A Workspace keeps every installed Dialect and Monograph alive while tools may
still inspect them. Package Aliases, Library Types, Function Signatures, and
source imports can therefore refer directly to objects produced by another
language.

Each Workspace is independent. Installing the same Dialect in another Workspace
creates another language environment and new Monograph identities. Tools may
correlate those objects through durable names defined by their owners.
An Archive can rebuild equivalent source results in another Workspace. It does
not keep the original references alive or rely on a global Type list.

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
by that request.

Some Dialects require another Dialect. Scene requires Library, while Shader
requires Library and Render. Environment creates each dependency once and gives
the shared instance to every language that needs it. A missing dependency is a
source error, and dependency loops are rejected.

## Direct source import

Direct import supplies three independent facts:

- the stable name used for Workspace lookup
- the path shown in errors
- the source bytes read by the selected Dialect

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

Package reads each declared path from its confined storage. Environment gives
the member to its selected Dialect, restores dependencies from their Archives,
and connects the resulting Monographs through their Package contexts. Names
local to a Package remain inside that Package rather than entering the Workspace
root automatically.

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

An `Invalid` answer observed before these stages finish is not a permanent
negative result. Read-only consumers begin after publication. A tool that
inspects a group while it is still being built must ask unresolved questions
again after linking or finalization.

Archive restoration uses the same barriers. Environment does not deserialize
live objects or revive process addresses. It asks each installed Dialect to
construct a new Monograph from its validated payload, retains the complete
group, links every member, finalizes every member, and then publishes the
restored roots.

A Monograph may contain child layers from its dependencies. Environment keeps
and publishes the outer Monograph, while the outer language moves its children
through the same linking and finalization steps. Tools ask the outer Monograph
for a layer instead of looking for another Workspace name.

During Archive restoration, Environment creates the Package Monograph before
restoring its members. Every member receives that Package context, including
child layers inside Scene and Shader. The language dependencies still come from
the Workspace. If a child layer cannot be restored, its outer member also
fails.

## Contextual lookup

Workspace is an ordinary TTX Abstract context. Looking up an exact imported root
name returns its retained Monograph. A missing name returns TTX `Invalid`.

Deeper `::` access is interpreted by the returned Abstract contexts. Environment
does not require every Monograph to expose a Type or one common member model.

## Diagnostics

Environment keeps the name and text of each authored source. The source and all
of its child layers write errors to one ordered diagnostic list. Messages stay
in the order they occurred, and errors cannot be hidden inside a child layer.
Environment adds the source location when it presents them.

Package paths, Archive bytes, and Repository requests can fail without an
authored Token. Their owners preserve the cause belonging to that domain, and
the caller attaches it to an authored dependency or compile request when such a
source location exists.

Perimortem process Diagnostics remain an emergency path for fatal host state.
They do not replace the source error list for ordinary parsing, language, or
restoration failures.

## Boundaries

Environment owns semantic lifetime, source dispatch, group completion, and root
publication. Package owns path confinement and durable package products.
Concrete Dialects own source grammar and language semantics. Compilers and
linkers consume completed Monographs without becoming part of Workspace lookup.

See [Language](../language/README.md) for Dialect and Monograph contracts and
[Package](../package/README.md) for package source and resources.
