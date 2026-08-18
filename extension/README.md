# Tetrodotoxin TTX

Tetrodotoxin TTX adds editor support for `.ttx` source files.

![TTX highlighting preview](media/ttx-preview.png)

## Features

- `.ttx` file association and a Tetrodotoxin file icon
- TextMate syntax highlighting for comments, modifiers, attributes, Types,
  Addressables, Callables, literals, operators, and punctuation
- optional semantic highlighting from the bundled language server
- full-document synchronization for open files
- source diagnostics with editor ranges
- hover for documentation, declaration Types, and folded constants
- format requests that preserve the authored source
- CodeLLDB breakpoint enablement and TTX carrier summaries

The bundled color theme gives TTX categories distinct defaults while respecting
editor customization.

## Semantic highlighting

Semantic highlighting is disabled by default so the TextMate colors remain
predictable. Enable it in VS Code settings when the language server should
provide token categories:

```json
{
  "tetrodotoxin.semanticHighlighting.enabled": true
}
```

Semantic tokens follow TTX's separate source categories. In particular,
Addressables selected with `.`, Types selected through `::`, and Callables
invoked with `->` remain distinct.

The current server derives shared lexical categories and the authored Dialect
declaration. It does not yet interpret a complete semantic graph for editor
highlighting, so semantic tokens remain opt-in.

## Debugging

Puffer emits DWARF 5 with C11 physical carrier Types so stock LLDB can bind TTX
source breakpoints and materialize stack variables. The extension packages LLDB
Python summaries and synthetic children for TTX values. A CodeLLDB launch opts
in by including Tetrodotoxin in `sourceLanguages`:

```json
{
  "type": "lldb",
  "request": "launch",
  "sourceLanguages": ["tetrodotoxin"]
}
```

The extension then imports the packaged formatter without making the language
server responsible for runtime state. Debugger configuration and formatter
wiring are kept separate from the language-client lifecycle inside the
extension.

## Language server

The Linux extension package includes the `puffer` language-server binary and
starts it for Tetrodotoxin documents. Puffer tracks open document text and
provides diagnostics, hover, and full semantic-token responses over the Language
Server Protocol. This binary carries Perimortem's fixed Dialect set. Projects
with additional Dialects build and package their own extended Puffer. A future
tutorial will cover that workflow.

Run `./extension/package.sh` from the repository to build Puffer and create the
versioned VSIX beneath `.vscode`. Pass `--install` only when the new package
should replace the installed extension.

See [Puffer](../puffer/README.md) for its command-line interface,
[TTX](../ttx/README.md) for the shared language model, and
[Tetrodotoxin](../tetrodotoxin/README.md) for the concrete Dialects.
