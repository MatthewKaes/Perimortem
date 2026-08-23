# Tetrodotoxin for Visual Studio Code

<p align="center">
  <img src="https://raw.githubusercontent.com/tetrodotoxin-dev/Tetrodotoxin/main/extension/media/logo.png" alt="Tetrodotoxin Toolchain" width="100%">
</p>

Open a Tetrodotoxin project and move from its Package manifest to CPU code,
application policy, Scene state, render contract, and Shader without changing
mental models or editor tools. This extension gives every installed language a
shared editing experience while preserving the meaning that makes each one
useful.

Navigation follows real program identities across files and languages.
Diagnostics come from the same Workspace used by the build. Formatting and
source color make dense TTX code easier to read before the program is complete.

![Tetrodotoxin editor preview](https://raw.githubusercontent.com/tetrodotoxin-dev/Tetrodotoxin/main/extension/media/ttx-preview.png)

## Understand the program as you write it

The editor reads the same source facts that Tetrodotoxin uses to build a
Workspace. That connection makes the help specific to the program rather than
an approximation based only on spelling.

* Hover shows complete Callable signatures, declaration Types, documentation,
  and folded constants, while unresolved declarations show the strongest known
  shape with `<unknown>` where an edge is still settling
* Access completion follows `.`, `::`, and `->` through the receiver's real
  Context or Type, visibility, and Static or Self role. Call suggestions remain
  available after the trailing space in the canonical ` -> ` spelling
* Parameter hints name fitted positional arguments at their call sites
* Go to definition follows authored identities across Package sources and
  dependencies
* Diagnostics point back to exact authored Tokens and ranges
* Source colors distinguish Types, Addressables, Callables, values, control
  flow, modifiers, and punctuation using Tetrodotoxin's own vocabulary

The result is especially helpful in dense TTX source, where punctuation carries
meaning and several languages can appear in one Package without sharing the
same grammar.

## Keep source consistent

The bundled formatter gives equivalent TTX source one canonical shape. It
understands declarations, Packs, Layouts, access operators, Blocks,
Documentation, Attributes, and alignment islands. Incomplete source remains
editable, so formatting can help recover a file without discarding the text the
author is still repairing.

Open documents are interpreted as editor overlays. A change to one Package
member rebuilds the shared Package view. Complete islands remain eligible for
build products, while incomplete edits retain their Tokens, diagnostics, and
strongest semantic identities for hover, navigation, and completion.

## Debug native Tetrodotoxin programs

Tetrodotoxin can emit DWARF 5 source correlation for native Library code. The
extension enables TTX breakpoints through CodeLLDB and includes Python summaries
for the runtime carriers used by generated values.

A CodeLLDB launch opts in by naming Tetrodotoxin in `sourceLanguages`:

```json
{
  "type": "lldb",
  "request": "launch",
  "sourceLanguages": ["tetrodotoxin"]
}
```

The physical debug information uses C11 carrier descriptions so LLDB can
materialize values with its existing native support. Those descriptions are a
debugger bridge. The authored source and language model remain TTX.

## A toolchain in the extension

The Linux VSIX includes Puffer, the Tetrodotoxin language server and command
host, together with the standard Tetrodotoxin Packages needed by editor
sessions. Puffer owns the editor session while Environment, Package, and each
Dialect continue to own the semantic work they contribute.

Packaging Puffer with the standard Packages makes the extension a natural
distribution boundary for the Tetrodotoxin SDK. Editor services and command
builds can share the same Toolchain composition instead of maintaining separate
language models.

## Color and editor preferences

The default palette is designed around TTX's warm semantic groups. Keywords,
Types, constants, data flow, control flow, and muted punctuation each have a
related place in that palette. Bracket matching and automatic closing remain
available without replacing the punctuation colors with a separate rainbow.

TextMate highlighting is the default because it gives incomplete source a
stable presentation. Semantic highlighting is available for readers who prefer
the completed Workspace to refine those categories. You can enable it in VS
Code settings:

```json
{
  "tetrodotoxin.semanticHighlighting.enabled": true
}
```

## Explore the platform

* [Tetrodotoxin](https://github.com/tetrodotoxin-dev/Tetrodotoxin/blob/main/tetrodotoxin/README.md)
  introduces the language and toolchain platform
* [TTX](https://github.com/tetrodotoxin-dev/Tetrodotoxin/blob/main/ttx/README.md)
  explains the shared semantic vocabulary
* [Puffer](https://github.com/tetrodotoxin-dev/Tetrodotoxin/blob/main/puffer/README.md)
  documents the command and editor host
* [Standard Packages](https://github.com/tetrodotoxin-dev/Tetrodotoxin/blob/main/packages/ttx/README.md)
  describe the included Memory, Math, System, and Graphics APIs
