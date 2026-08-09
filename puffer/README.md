# Puffer

Puffer is the command-line and language-server application for Tetrodotoxin. It
connects editor or terminal requests to the reusable Tetrodotoxin libraries and
presents their results.

## Language server

Run Puffer over a local socket:

```text
puffer --pipe=<socket-path>
```

The language server supports:

- initialization using UTF-16 document positions;
- opening, replacing, and closing complete document text;
- full-document semantic tokens for TTX lexical categories;
- clean shutdown and exit handling.

The [Tetrodotoxin TTX extension](../extension/README.md) packages and launches
this server for `.ttx` documents.

## Application role

Puffer owns the process and editor-facing parts of a request:

```text
editor or command-line request
-> Puffer session
-> Tetrodotoxin language and package APIs
-> diagnostics or completed products
-> editor or terminal response
```

Open editor documents may contain unsaved bytes, so Puffer retains their text
and protocol identity for the session. Reusable source interpretation, Package
resolution, Workspace lifetime, compilation, linking, and Archive behavior stay
in Tetrodotoxin and can be used by another application.

See [Tetrodotoxin](../tetrodotoxin/README.md) for the host architecture and
[TTX](../ttx/README.md) for the lexical and semantic vocabulary.
