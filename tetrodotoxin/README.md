# Tetrodotoxin

Tetrodotoxin is the core source-IR toolchain used by Perimortem for
development. TTX is the default frontend IR language for that toolchain, tuned
for Perimortem's authoring, editor, shader, ABI, and build needs.

For the TTX source language, start with [ttx_design.md](ttx_design.md). The
compiler architecture, parser contract, type/layout validation rules, and
type-system notes
live in [ttx_semantics.md](ttx_semantics.md).

## About

Tetrodotoxin is inspired by LLVM, but it operates at a different layer. LLVM IR
is a lowered compiler representation. TTX is Tetrodotoxin's Perimortem-focused
frontend IR: it keeps package boundaries, layouts, packs, dialect metadata,
comments, and addressability visible long enough for both tools and compilers
to use them. The toolchain is not inherently limited to TTX. Another project
could provide a different frontend language and still use the later
Tetrodotoxin layers, but TTX is the surface Perimortem optimizes around.

The central architecture is built around **monotonic context layering**, where
each phase enriches the same program representation with more queryable context.
The core pipeline used by Perimortem's Bazel extension has an authoring surface
followed by compiler stages:

```text
authoring surface -> lexical -> syntax/scope -> resolution/import graph -> validation context -> compilation -> generation
```

Each layer adds data, validation, and queryable facts while preserving the
source-shaped program for the next layer.

The validation context is an enrichment surface, not a separate lowered IR. It
combines type/layout queries with dialect, ABI, and compiler-provider facts so
later stages can ask for proven answers instead of rebuilding source meaning.
Dialect participates early as dialect-header syntax and dialect-body parsing,
during resolution as import compatibility, and later as legality and metadata
queries.

### The long road of compilation has many side streets

Tetrodotoxin is not an **anti-lowering** architecture. Final outputs such as
`SPIR-V`, LLVM IR, `x86_64` code, generated headers, and object archives are
still lowered artifacts. In Tetrodotoxin, however, these are treated as
**terminal** outputs.

By delaying final lowering, the IR can be progressively enriched and produce
multiple terminal outputs from the same pipeline. A full pipeline execution is
not always necessary:

- Syntax highlighting can stop after lexical classification.
- Formatted source can stop after syntax.
- Project navigation can stop after resolution.
- Compilation can prepare backend-specific context.
- Generation can emit backend blobs, headers, archives, editor data, and other
  terminal artifacts.

Any step of a Tetrodotoxin pipeline can be cached, allowing for speedy
continuations as long as no prior layer is invalidated. This gives the toolchain
different output depths for things like tooltips, diagnostics, formatting, or
generated binary data. The only destructive operations happen at terminal
boundaries.

The cost of this approach is that earlier phases are limited to enriching the
same source-shaped IR with queryable context instead of rewriting it into a new
primary representation at each step. Every layer has to keep its added context
cheap, coherent, and reusable. If a layer adds facts that later stages cannot
query cleanly, the context becomes weight instead of leverage.

Perimortem takes that tradeoff because it does not need only a batch compiler.
It needs a language server, formatter, import graph, shader pipeline, host ABI
surface, diagnostics, generated data, and eventually multiple backend targets.
Those systems need to support real-time type and layout queries without the compromises that
come from typical scripting-language approaches. By sharing one source-shaped
representation, the whole toolchain can stay smaller than a pile of one-off
translators while providing faster cached results across layers.

The bet is not that TTX can beat a tiny C compiler at compiling one simple file
once and exiting. A tiny compiler can scan, parse, emit, and forget. TTX carries
more structure because it is solving a broader problem: many jobs, one stable
source IR, and tooling interop guarantees.

TTX also diverges from MLIR in an important way. MLIR makes compiler IR
extensible through dialect operations and rewrite passes. TTX makes authored
source semantically dense enough that many tools can share it before lowering.
There is no reason Tetrodotoxin could not target MLIR, but moving in that
direction now would make much of the toolchain clone MLIR infrastructure for
little benefit to either project at this stage.

### Tetrodotoxin is highly opinionated

Delayed lowering is useful only if the context layers stay cheap. The practical
memory model is stable source buffers, views instead of copied strings,
arena-local syntax nodes, and compact or lazy derived facts keyed by resolved
type identity or syntax identity. If each stage builds large duplicate vectors of "bound types",
"shader types", and "compiler types", then the context grows iteratively rather
than being enriched and reused.

The correct shape trends toward query caches and dialect views over the same
program, not parallel cloned IRs. The core Perimortem pipeline uses the
following breakdown:

- Source and package records own source text and arena lifetime.
- Lexical owns token classification.
- Syntax nodes own source shape.
- Resolution owns package identity, imports, cross-file queries, and canonical
  type identity.
- Validation context exposes type/layout facts such as expression type,
  pack-fit, selected calls, returns, and assignment compatibility using
  resolution, syntax, dialect, ABI, and compiler-provided TTX package facts.
- Dialects own package-specific body parsing, legality, and metadata, such as
  Render stage declarations or descriptor bindings, but they are still
  provider/query surfaces over shared source shape rather than cloned program
  representations.
- Compilation performs terminal lowering when an output artifact is requested.

If a layer starts building a private clone of the program, it is probably
fighting the architecture.

Dialect is also not a synonym for backend target. `Library`, `Render`,
`Shader`, and `Entity` are authoring spaces. `SPIR-V`, `x86_64`, generated
headers, archives, and engine bridge code are terminal outputs or backend
targets selected later by compilation.

A result of this is that Tetrodotoxin optimizes for highly opinionated and
localized stages that do their work without considering every possible thing to
the right of them. The toolchain pushes stable local semantics "to the left"
when possible, and that pressure directly shapes the TTX IR. The authoring
surface already encodes a large amount of semantics through a constrained
grammar. Later phases should not have to ask whether the author meant a token to
be a type or a variable, which lets even the lexer ask richer questions and
layer its own intent.

### Optimizing for shifting left: the TTX IR format

Any tool or system that can consume an enriched IR output from a given stage can
also consume enriched IR output from stages to its right. In practice, each
additional layer has diminishing returns for tools that do not need its extra
context. Terminal outputs generated from the leftmost layers are therefore the
most generally useful, though they have the least data to pull from.

For Perimortem, Tetrodotoxin provides an optimized frontend IR in the form of
TTX as its authoring surface. TTX uses syntax designed to pull as much stable
semantic information left as possible while staying quick to process. The TTX
tokenizer carries semantic classes such as type names, addressable names,
sigils, attributes, builtin type forms, and operators before what would be a
typical parser pass.

A reference [ttx grammar](syntax/ttx.g4) is provided for custom parsers or
parser generators.

TTX does not aim to be the ideal authoring surface for anything outside of
Perimortem. For other use cases, create a DSL with custom layers that plug into
the toolchain at the appropriate level, whether lexical, syntax, or later.

## Terminal output generation

Tetrodotoxin currently provides a TTX LSP that produces several terminal outputs
mostly for VSCode integration.

For more general outputs the Tetrodotoxin toolchain provides a few simple
standalone codegen targets. Currently this includes `SPIR-V` and `x86_64`,
which cover the major backend families planned for Perimortem. These codegen
layers produce intermediaries using a C ABI that can integrate with other
toolchains such as LLVM via `clang`.

A linker layer can create additional binary outputs. The supported outputs are
currently:

- PNGs
- ELF archives and executables
