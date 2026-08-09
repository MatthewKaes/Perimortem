# Tetrodotoxin TTX

Tetrodotoxin TTX adds editor support for `.ttx` source files.

![TTX highlighting preview](media/ttx-preview.png)

## Features

- `.ttx` file association and a Tetrodotoxin file icon
- TextMate syntax highlighting for comments, modifiers, attributes, Types,
  Addressables, Callables, literals, operators, and punctuation
- optional semantic highlighting from the bundled language server
- full-document synchronization for open files
- format requests that preserve the authored source

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

## Language server

The Linux extension package includes the `puffer` language-server binary and
starts it for Tetrodotoxin documents. Puffer tracks open document text and
provides full semantic-token responses over the Language Server Protocol.

See [Puffer](../puffer/README.md) for its command-line interface,
[TTX](../ttx/README.md) for the shared language model, and
[Tetrodotoxin](../tetrodotoxin/README.md) for the concrete Dialects.
