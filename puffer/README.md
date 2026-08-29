# Puffer

Puffer is the small process boundary into Tetrodotoxin. It receives either one
source or one language-server pipe, constructs the real TTX graph, and delegates
products to the owners that understand them. It owns no Package request model,
artifact inventory, target graph, provider map, or semantic sidecar.

```text
puffer <one-source.ttx>
puffer -lsp=<pipe-name>
```

A source command may add:

```text
-package_repository=<read-only-root>
-terminal_repository=<output-root>
-dump_graph
-generate_cxx
```

The terminal repository defaults to the current directory. Dependency lookup
checks completed Package products there first and then the optional read-only
Package repository. Publication writes only to the terminal repository.

## Source sessions and products

One source creates one Workspace transaction. If it is a Package source, its
authored declaration supplies the exact identity and version:

```ttx
package(.name = "Example.Memory", .version = "1.0");
```

Workspace discovers every source and Package edge from common Import Aliases.
Puffer supplies no source list, dependency table, coordinate override, ABI
manifest, native provider, or Bazel artifact path. Missing Package coordinates
are acquired from the two repository roots and restored through their real
Dialects before the source completes.

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

## Independent Terminals

Graph text and C++ production are independent observations of the same graph.
`-dump_graph` writes versioned, dialect-neutral graph text to stdout while
diagnostics remain on stderr. It can describe the strongest retained partial
graph and Puffer still returns failure when compilation did not complete.

`-generate_cxx` asks the completed graph for canonical `api.hpp`, `api.cpp`, and
any required ABI siblings. A graph without that producer fails explicitly.
Default and C++ products never run over incomplete meaning.

## Build-tool integration

Bazel has one generic rule:

```starlark
ttx_source(
    name = "memory",
    src = "package.ttx",
    deps = [":core"],
    generate_cxx = False,
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
