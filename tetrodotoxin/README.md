# Tetrodotoxin

Tetrodotoxin is a multi-phase IR -> IR toolchain used by Perimortem for development.

## About

Tetrodotoxin aims to solve two problems with toolchains: Interoperability and performance.

As projects scale most toolchain complexity can grow with, slowly calcifying and creating execution risk.
On top of that builds and other common operations like type completion suffer as well.

Inspired by LLVM, Tetrodotoxin aims to generalize the LLVM IR model by developing a toolchain level IR.
This includes targeting LLVM IR as its primary end target for compliation to integrate with the existing ecosystem.

The toolchain spec is fundementally composed of three concepts:

* Context
* Terminals
* Layers

## Context

Context is the meta Intermediate Representation (IR) output that maps to of any single layer in Tetrodotoxin.

In the abstract goal of any toolchain is to convert information through several operations into something useful using some IR.
Given the number of formats is unbounded the only way to keep things fast and managable is to keep each surface area as small as possible.
A useful observation is that there is a strong corrolation between frequency of an operation and how much data it actually needs.

To support this Context uses a meta IR that focuses on minimizing the amount of useful IR required for translation.
Each layer executed can add to the context in a purely additive fashion allowing for granular control and conversion of IR components.

## Terminals

Terminals include any data which enters or leaves the toolchain that aren't expressive enough that the context must collapse,
or require so much data that the meta IR ceases to be a useful intermediary.

Terminals serve an additional purpose as the sole means of getting mutable state into a layer.
If a terminal is consumed at multiple parts of a layer stack each instance is considered a unique "context", even if the underlying data is the same.

Since terminals directly expose a layer to additional data they should be used sparingly when possible.

## Layers

In practice a layer in Tetrodotoxin can be viewed as a "Middle-end" that satisfies three strict invariants:

1. A layer must be a pure function of exactly a single layer's _context_ + any number of additional _terminal_ inputs.
2. A layer must create _at least_ one ouput which may contain _at most_ one _context_.
3. Equivalents to _all of a_ layer's inputs must be derivable soley from its outputs.

While these invariants leave a lot of wiggle room they are enough to cover a number of properties the toolchain relies on.
In particular data may only be losslessly transformed or compressed.

* Two or more layers can be composed into a single optimized layer when intermediary stages aren't required to be observed.
* A layer can be decomposed into multiple layers to provide finer grain details.
* Since any two layers that produce the same output are factually the same "layer" back propagation can be optimized separately from its forward variant.

Since a layer can only consume a _single context_ there is no way to "merge" context from two invocations.
Instead concepts such as "Import" are just layers that add context from imported files as a shell around the core context.

### Layer 0: TTX

TTX is a special "Layer 0" and is the only canonical defined layer for Tetrodotoxin.
It has semetric inputs and outputs; producing and consuming itself, serving as a _sort_ of identity function.

TTX define a mapping of exactly **64** values to **64** values.
The values themselves are _arbitrary_ but any following layer must agree implicitly on those base 64 values since it must eventually be able to rehydrate them by virtue of *layer invariant #3*.

TTX chains with itself so any number of TTX implementations can be chained together (A->B->C->D).
This is mostly useful for toolchain conversions: Layer_3_TTX_B that uses Layer_2_TTX_B can be retrofitted to use Layer_2_TTX_A by composing (TTX_A-B * Layer_2_TTX_B).

Currently there are currently a few TTX implementations used for toolchain interop:

* TTX Classifier: Maps arbitrary human readable names to a 64 bit field.
  * Compressed TTX used for in process debugging.
* TTX Network: Maps values to a set of serializable names.
  * Used when transmitting TTX over pipes.
* TTX Colorful: Maps values to colors.
  * Useful for corrdinating syntax highlighting between the LSP and any editors for theming.
* TTX as CPP: Maps JSON values to non-overlapping VSCode CPP syntax types.
  * Maps TTX to a set of C++ Abstract Machine types

NOTE: Once TTX leaves the Tetrodotoxin toolchain for another toolchain (VSCode, LLVM, ect.) it loses all gurentees of being reversible.
As an example the TTX theme included in the VSCode extension compresses several TTX values into the same color.

## Toolchain Examples

This implementation of Tetrodotoxin comes with a number of layers used by Perimortem.

Here are a few examples of how those layers are used for various operations.

### LSP Syntax Highlighting

Syntax highlighting uses terminals a call response interop between toolchains.
No values are cached since the context required to perform syntax highlighting is minimal.

* Terminal In: String
* Layer 0: Client RPC
* Layer 1: JSON RPC
* Layer 2: TTX Classifier
* Layer 3: Tokenizer
* Layer 4: Range
* Layer 5: JSON RPC
* Layer 6: Client RPC
* Layer 7: TTX Colorful
  * Terminal Out: VSCode JSON

### LSP Formatting

Formatting only requires a few additional layers to gain context about structure.
To speed things up it's faster to compose the layers into two meta layers.

* Terminal In: String
* Layer 0: Client RPC
* Layer 1: JSON RPC
* Layer 2: Lexical (composed)
  * Layer 0: TTX Classifier
  * Layer 1: Tokenizer
* Layer 3: Parser (composed)
  * Layer 0: Statements
  * Layer 1: Scopes
* Layer 4: Shifter
* Layer 5: JSON RPC
* Layer 6: Client RPC
  * Terminal Out: String

### Compiler

Converting a single file to LLVM.
Terminals can be handed off to any LLVM tool chain to produce binary outputs.
This mostly used the same compositions seen in formating, however parser has a few additional steps.
In addition to imports and looping the parser layer we add context to disabled scopes so LLVM know they are comments.

* Terminal In: String
* Layer 0: Lexical (composed)
  * Layer 0: TTX Classifier
  * Layer 1: Tokenizer
* Layer 1: Parser (composed)
  * Layer 0: Statements
  * Layer 1: Scopes
  * Layer 2: Disable
  * Layer 3: Import
* Layer 1_x: Parser (composed)
  * ... Repeated for each unresolve import context ...
* Layer 2: Virtual Machine
  * Layer 0: Types
  * Layer 1: Storage
  * Layer 2: Basic Blocks
* Layer 3: Compile
  * Terminal Out: LLVM IR

# Tetrodotoxin (The Language)

Of the Perimortem project an "idealized" programing language is being for Tetrodotoxin.
While the hope is to use it in the future as a "human readable" IR interface it currently serves as a simple integration test.