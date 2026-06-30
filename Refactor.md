# Tetrodotoxin Architecture Refactor Backlog
This is a "full repo audit" refactor. “Full repo audit” means all non-vendored source in perimortem, tetrodotoxin, apps, and validation; exclude .git, .bin, Bazel output, node_modules, generated VSIX contents, and generated protocol files unless their checked-in wrapper namespace is wrong.

Tetrodotoxin should continue to read as one source-shaped pipeline:

```text
authoring surface -> lexical -> syntax/scope -> resolution/import graph -> validation context -> compilation -> generation
```

Each stage may add context, cache derived answers, or validate invariants. A
stage should not duplicate source-owned, resolution-owned,
validation-owned, dialect-owned, ABI-owned, or standard-package-owned facts into
a new primary structure unless it is producing a terminal artifact.

Validation context is an enrichment surface, not a lowering pass. It includes
type/layout queries plus dialect, ABI, and provider facts such as expression
type, pack fit, selected callable, assignment and return compatibility,
access-chain result, shader stage, descriptor binding, push constants, and
foreign boundary metadata. Dialect is not a separate replacement IR stage:
package kind is syntax-visible, import dialect compatibility is resolved with
the package graph, and dialect-specific legality and metadata are queried by
validation and compilation.

The old semantic Binding summary layer is gone. Do not recreate it under a new
name; remaining type, pack, layout, call, and conversion checks should return
through the owner that actually knows the fact.

Don't be lazy. Keep an eye on future plans and code reuse. Do not bolt on code unnecessarily.
Always refer back and keep this markdown up to date as the source of truth.

Make sure you are always familiar with the TTX philosophy and reference the README files,
ttx_design.md, and ttx_semantics.md. After compactions make sure they are always in your
memory or context.

## Fresh Session Execution Notes

- Start by rereading this file, `tetrodotoxin/README.md`,
  `tetrodotoxin/ttx_design.md`, and `tetrodotoxin/ttx_semantics.md`.
- Do an initial ownership scan before editing: look for duplicated concepts,
  fake adapter layers, namespace/path mismatches, and code that rebuilds facts
  already owned by Syntax, Resolution, validation, dialects, ABI providers, or
  standard packages.
- Execute in dependency order: Syntax/formatting ownership first, then
  validation/provider surfaces, then dialect/ABI boundaries, then compiler
  intermediate cleanup, then Library compilation/linker/application work.
- Keep each slice buildable. Run targeted tests as each layer is moved, and run
  the full validation/build pass before calling the refactor complete.
- Do a final ownership scan before stopping. Any new helper or abstraction must
  name the concept it owns and delete or replace the duplicate API it made
  obsolete.

## Proper Tetrodotoxin hierarchy

- Tetrodotoxin is a toolchain, not just the TTX spec. Evaluate whether the
  various compiler layers should move under a clear toolchain namespace or
  whether the current top-level folders already express the intended ownership.
  The decision should make LSP, formatting, syntax, resolution, validation,
  dialect, compiler, and generation boundaries easier to read from paths alone.


## Syntax And Formatting Ownership

- Add a canonical declaration/definition kind in Syntax so formatter and parser
  code do not invent `TypeGroup`, predicate wrappers such as `is_enum` or
  `is_shader`, or duplicate `get_decl_type()` helpers.
- Clean `tetrodotoxin/format` headers so formatter declarations live beside the
  concept implementations they describe. Remove `format/syntax.hpp`, or shrink
  it to a deliberate include facade rather than a declaration dump.
- Keep package formatting limited to package documentation, header, and imports;
  package bodies should use the same type-body formatter as ordinary type bodies
  with only the ordering policy supplied by the caller.
- Keep file paths, namespaces, and concept ownership aligned. A file should not
  publish APIs for a different concept or namespace just because a nearby caller
  needs a declaration.

## Type And Layout Validation Surface

- Reintroduce pack rules, literal conversion rules, assignment and return
  acceptance, and standard package fit helpers only under the owning type/layout
  validation surface.
- Keep `Layout` as the central TTX shape fact. Avoid rebuilding copied semantic
  IR or Binding-style summaries over Syntax declarations.
- Keep `Ttx::Type` responsible for naming, classification, and dispatch facts,
  with `Ttx::Layout` responsible for concrete/fluid shape, storage, and field
  facts.

## Standard Packages And Provider Facts

- Remove direct syntax dependencies from TTX provider facts where Core and
  Graphics still answer layout/type questions from `Syntax::Type::Ref`.
- Add provider-owned generic layout caching for generated facts such as
  `Vec[T, N]`, keyed by canonical resolved type identity rather than raw syntax.
- Model compiler-provided packages as layouts so `TTX.Core` and `TTX.Graphics`
  expose package globals, types, and functions through the same query surface as
  ordinary concrete types.
- Replace remaining scalar terminology in public TTX provider APIs with direct
  `Type` and `Layout` queries.
- Rename remaining semantic `builtin` terminology to `standard`,
  `compiler_provided`, or another precise term. Keep lexical and documentation
  uses only where they mean source-language builtin tokens.

## Dialect And ABI Boundaries

- Keep `tetrodotoxin/syntax/dialect` focused on syntax-time legality. Tighten
  naming so those checks do not read like semantic shader facts.
- Add an explicit `Foreign`/ABI provider layer for externally owned
  declarations, symbols, storage layout, and host/device boundary facts.
- Ensure terminal backends consume resolution, type/layout validation, dialect
  queries, and ABI/provider facts instead of walking raw syntax to rediscover
  facts.

## Compiler Intermediate Boundary

- Keep `compiler/intermediate` quarantined from the shader path until it is
  either a real terminal-independent ABI layer or removed.


## Compiler for Library objects

To avoid over fitting the current code we need to flesh out the `Library` class and generators as well so
we have a better view for spotting code reuse and the overall layout of the layers and code. Eventually we will need a memory management system and modern GC for dynamically created TTX objects but that can possibly be delayed for now but the design should consider it and not close any doors on `Objects` for the `Library` dialect.

- Update the code to also work for Library objects rather than just emitting default output.
- The unit test TTX should emit a proper library that is linked into the test.
- The x86_64 code generator should be extended to emit the required code.
- The compiler for x86_64 needs to consider register usage and should have a layer for basic mem_to_reg
  optimization built in. It should use a modern register coloring algorithm for spill over but and should follow correct System V calling conventions.
- Code generated should be C++ and TTX compatible. A "Foreign" API header for C++ will need to be generated but to avoid name mangling in C++ it may need to have an `extern "C"` wrapper inside of a C++ namespace.
- The x86_64 library will need to at least have basic "lowering" support for `Byte::Views`. We can avoid passing dynamically created objects over any TTX / Foreign boundaries for now.
- A few interop tests should be created that have our validation binary round trip from TTX, right now there is one existing use case but it doesn't include actual types. Layouts can be used for marshaling the wire format. We can assume that host endianess is the same for both TTX and Foreign code.
- Library code should be able to `import Shader` code through it's dialect. This mostly just exposes the push constants and will give us a better idea of how to set shader on 2D objects.
- Replace the SplashScreen.cpp app code with TTX. This will be the ultimate stress test of our architecture and will most likely spawn a number of follow ups.

## Linker should be able to emit full executables from TTX for the Demo App
- Add a Bazel rule for invoking the TTX compiler as part of the tetrodotoxin toolchain to emit an executable.
- The Bazel rule should be able to link and consume other TTX libraries as well as C++ generated libraries.
- Above just linking, the rule should be able to support "Foreign" imports that expose APIs via a "C" ABI.
- Vulkan and Engine code should mostly exist in the "Perimortem" layer of the engine which is default included for any TTX binaries.
- The Bazel rule should name which scene is the "Entry Point" / "Start" scene. We should not default assume that it is main. During testing we might select different scenes to start from during rebuilds.
- At this stage you will most likely need to get into the weeds of how resources are packaged (the icon should most likely be embedded in the binary using the `$[]` embedded path syntax)
