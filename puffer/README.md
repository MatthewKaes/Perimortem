# Puffer

Puffer is a command-line and language-server application for Tetrodotoxin. It
owns process startup, protocol transport, editor document sessions, request
selection, and presentation of Tetrodotoxin results.

Reusable language and package behavior remains in the Tetrodotoxin library.
Puffer does not own source loading, filesystem confinement, package resolution,
repositories, workspaces, package construction, semantic materialization,
compilation, linking, or archival.

## Application boundary

```text
command line or editor request
-> Puffer process and session state
-> Tetrodotoxin package and language APIs
-> Tetrodotoxin diagnostics or completed products
-> Puffer protocol or terminal presentation
```

Puffer may retain unsaved editor bytes and protocol identifiers because those
facts belong to an editor session. It passes the selected input to
Tetrodotoxin, but it does not reinterpret Token streams, assemble an
Environment, search a repository, or create a second Package model.

The dependency direction is one way: Puffer depends on Tetrodotoxin.
Tetrodotoxin never borrows a Puffer capability and never names a Puffer type in
a reusable library contract.

## Active application surface

The application targets are:

```text
//puffer:lsp
//puffer:puffer
```

Target existence establishes only that the application surface builds. It does
not prove Package construction, semantic evaluation, terminal production, or
source-free restoration.
