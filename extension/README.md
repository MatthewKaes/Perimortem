# Tetrodotoxin TTX

Language support for TTX, Perimortem's Tetrodotoxin source IR.

![Synthetic TTX highlighting preview](media/ttx-preview.png)

This preview is a static package asset built from TTX source text and the
extension color rules.

## Current feature set

- `.ttx` file association with the Tetrodotoxin language id.
- Bundled TTX TextMate grammar for syntax highlighting.
- Bundled red TTX color defaults for comments, modifiers, attributes, types,
  members, functions, constants, strings, numbers, operators, and punctuation.
- A format request that currently preserves the authored source unchanged.
- Full-document synchronization with the language server for open `.ttx` files.
- Optional LSP semantic tokens for users who want editor semantic highlighting.
- Tetrodotoxin file icon for `.ttx` documents.

## Semantic highlighting

Semantic tokens are disabled by default and can be turned on to override the
bundled TTX TextMate color scheme.

```json
{
  "tetrodotoxin.semanticHighlighting.enabled": true
}
```

## Bundled TTX language server

The extension packages the current Linux `puffer` binary and starts it in LSP
mode. The server currently supplies transport, document synchronization, and
lexical semantic tokens. Dialect-aware formatting and diagnostics will use the
same resolver and evaluator path as package compilation when that integration
lands.

## Not yet included

The current extension does not advertise completions, hover, go-to-definition,
references, or published diagnostics.
