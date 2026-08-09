# Environment

Environment hosts Tetrodotoxin source. Its public `Workspace` represents one
semantic island: a set of installed Dialects, interpreted or restored
Monographs, their shared TTX edges, and the source origins used for diagnostics.

## Workspace lifetime

A Workspace keeps every installed Dialect and retained Monograph alive while
their semantic identities can be queried. This common lifetime lets a Package
Alias, Library Type, Function Signature, or cross-source import borrow the exact
identity produced by another owner.

Each Workspace is independent. Installing the same Dialect in another Workspace
creates another language environment, while immutable language identities may
be shared when their concrete Dialect defines them that way.

## Dialect installation

A tool installs the concrete Dialects accepted by one invocation. The exact
installed name is the name authored after `dialect`:

```ttx
dialect : Library;
```

Unknown names are source errors. A source never selects a Dialect through a
closed package-kind enum or a filename convention.

## Direct source import

Direct import supplies three independent facts:

- the semantic name used for Workspace lookup;
- the diagnostic path presented to the user;
- the source bytes interpreted by the selected Dialect.

Environment reads the common envelope, selects the Dialect, and retains the
returned Monograph under the authored semantic name. The path describes origin;
it does not create semantic identity.

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

Environment asks Package Storage for each declared path, interprets the member
with the selected Dialect, restores exact dependency Archives, and binds the
resulting Monographs through their Package contexts. Package-local names remain
inside that Package rather than entering the Workspace root automatically.

## Linking and publication

Interpretation can reserve declarations before all cross-source routes are
known. Environment therefore completes a retained source group in two barriers:

```text
interpret and retain the complete group
-> link every Monograph
-> finalize every Monograph when all links succeed
-> publish completed root names
```

This ordering allows forward references and dependency cycles that the concrete
languages can resolve while preventing a partially completed group from being
published as terminal input.

## Contextual lookup

Workspace is an ordinary TTX Abstract context. Looking up an exact imported root
name returns its retained Monograph; a missing name returns TTX `Invalid`.

Deeper `::` access is interpreted by the returned Abstract contexts. Environment
does not require every Monograph to expose a Type or one common child model.

## Diagnostics

Environment retains the source name and body associated with each authored
Monograph. A concrete language records Diagnostics against its semantic facts;
Environment combines them with that retained origin when presenting errors.

Package paths, Archive bytes, and Repository requests can fail without an
authored Token. Their owners preserve the domain-specific cause, and the caller
attaches it to an authored dependency or compile request when such a source
location exists.

## Boundaries

Environment owns semantic lifetime, source dispatch, group completion, and root
publication. Package owns path confinement and durable package products.
Concrete Dialects own source grammar and language semantics. Compilers and
linkers consume completed Monographs without becoming part of Workspace lookup.

See [Language](../language/README.md) for Dialect and Monograph contracts and
[Package](../package/README.md) for package source and resources.
