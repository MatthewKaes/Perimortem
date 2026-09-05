# Puffer

Puffer is the small process boundary into Tetrodotoxin. It receives either one
source or one language-server pipe, constructs the real TTX graph, and delegates
products to the owners that understand them. It owns no Package request model,
artifact inventory, target graph, provider map, or semantic sidecar.

```text
puffer <one-source.ttx> [dialect-owned arguments...]
puffer -lsp=<pipe-name>
```

Puffer interprets the first source and raises every remaining byte into one
immutable invocation authority. The selected Dialect decides whether names such
as `release`, `graph`, or `profile=debug` have meaning. Puffer has no build,
run, graph-dump, binding, backend, profile, repository, or output switches.

## Source sessions and products

One source creates one Workspace transaction. If it is a Package source, its
authored declaration supplies the exact identity and version:

```ttx
package(.name = "Example.Memory", .version = "1.0");
```

Workspace discovers every source and Package edge from common Import Aliases.
Puffer supplies no source list, dependency table, coordinate override, ABI
manifest, native provider, or Bazel artifact path. The invocation Environment
owns reachable repositories and restores any source-free dependencies through
their real Dialects before the source completes.

After completion, the selected Dialect may return one named Pack of immutable
byte products. The Package producer emits exactly:

```text
<identity>/<major>.<minor>/package.ttxp
```

Foreign native symbols remain authored and unresolved in that product. The
eventual build tool links ordinary native libraries without laundering them
through Puffer.

Product publication validates every relative name, rejects escapes and
collisions, stages the complete Pack, and exposes no partial product set.

Invoking a Package root directly publishes its host-independent archive:

```text
puffer package.ttx
```

That archive is optional for a local Build. `puffer build.ttx` lets Build source
the live Package graph inside its Environment-owned child Workspace and request
native, Graph Text, binding, GPU, or archive products independently. Product
selection belongs to Build mappings rather than Puffer command modes.

## Build-tool integration

Bazel has one generic rule:

```starlark
ttx_source(
    name = "memory",
    src = "package.ttx",
    deps = [":core"],
)
```

Each dependency contributes only its complete product tree. The rule copies
those trees into an action-private repository, invokes the ordinary Puffer CLI,
and publishes its new product tree through `DefaultInfo`. There are no TTX
providers, output groups, exported filegroups, application rules, native symbol
inventories, or artifact arguments.

## Language server

Run Puffer over a local pipe:

```text
puffer -lsp=<pipe-name>
```

Puffer speaks the position encoding offered by the editor. UTF 8 matches TTX's
source bytes directly, while clients such as VS Code currently ask for UTF 16
coordinates. The translation stays at the protocol boundary where Puffer still
has the source text needed to perform it.

Once a document is open, the server provides:

* source diagnostics with authored ranges;
* generic completion, hover, and definition queries over exact Association
  identities and shared TTX contracts;
* parameter name inlay hints from the selected Callable Layout;
* semantic tokens over the retained source graph;
* clean shutdown and exit handling.

Hover and navigation do not reconstruct Library, Shader, Pipeline, Import,
Field, Local, or Constant-domain meaning. A new Dialect participates by exposing
real Abstract concepts and common TTX contracts.

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents. See [Tetrodotoxin](../tetrodotoxin/README.md)
for the host and [TTX](../ttx/README.md) for the shared semantic vocabulary.
