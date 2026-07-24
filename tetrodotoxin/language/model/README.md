# Tetrodotoxin Model

`tetrodotoxin/model` owns the concrete semantic facts produced by
Tetrodotoxin's Parsers and consumed by its compiler, linker, archiver, and
future runtime.

The Model builds on TTX Abstract, Type, Layout, Expression, Addressable,
Callable, Documentation, and Body contracts. It does not copy them into a
second hierarchy.

## Source and Environment

Source owns one source stream, Tokenizer, Arena, diagnostic path, and every
semantic root derived from those bytes. It is anonymous semantic identity;
resolver, editor, and cache names remain on those external owners.

Environment owns the shared binding transaction. It retains exact package
dependencies, direct bindings, immutable Generic formulas, and one append-only
materialization writer shared by every member Source.

Source owns no package membership or dependency vector. Environment owns no
filesystem handle, process path, Source directory, repository search object, or
parser diagnostic.

Those physical inputs belong to `Parser::Package::Workspace`. Model never
borrows a Parser or Puffer type; Puffer is an application client of the
completed library boundary.

Cursor and token indexes are transient. The removed body-token replay overload
was migration residue, not Model state. Parser and evaluator work must produce
a semantic owner while the Cursor is active or reject the form.

## Dialect

`Model::Dialect` is the real named identity selected by a document envelope. A
Dialect controls presentation, accepted builtins, parsing/evaluation legality,
and additional semantic facts. Binding an identity makes it available without
importing its producing Dialect's rules.

The concrete source grammar for each Dialect lives under
[`../parser`](../parser/). Model retains only the semantic result.

## Namespace and Package

Namespace is one concrete Exports owner. It retains real roots, public edges,
and lookup indexes. It does not fabricate a Type or Layout.

`Package::Source` is the model produced from the root `package.ttx`. It owns
only the opening Documentation, exact Resolution requests, and normalized
member routes. `Parser::Package::Source` consumes tokens but retains none of
these facts.

`Package::Resolved` is anonymous semantic composition. External name and exact
Version belong to dependency and Manifest owners. Source-backed and restored
packages implement this same surface while retaining different lifetime
owners.

`Package::Sources` retains explicit member Sources, distinct direct
dependencies, terminal products, and canonical local definition coordinates
after the graph is complete. `Package::Precompiled` owns equivalent restored
facts without claiming interpreted Source.

## Type members

`Types::Structure` and `Types::Members` retain the real Addressable, Callable,
Alias, and other Abstract edges owned by a Type. Public, private, static, self,
and exposed lookup surfaces are owner indexes over those edges.

There is no generic Member model whose only purpose is to add another accessor
name. Layout continues to expose real Addressables. Documentation, attributes,
defaults, representation facts, and completion remain on their actual owners.

Concrete mutable lookup owners seal themselves before publication. Sealing is
not a universal Abstract state.

## Render and Shader

Render is a real semantic contract over:

- ordinary render values;
- constants;
- push values;
- resources; and
- required Stage Callables.

Shader retains one exact Render identity and its implemented Stages. Required
Stages own complete Callable Layouts and read sets. Shader products connect
validated Stage facts to terminal coordinates.

Parser grammar lives in [`../parser/render`](../parser/render/) and
[`../parser/shader`](../parser/shader/). Target representation and SPIR-V
records belong to Compiler. Archive encoding belongs to Archiver.

The live Render fixture currently conflicts with the documented `Render`
envelope by using `Gpu` and `ShaderFormat`. Model must not add aliases or a
second identity to hide that unresolved source-owner decision.

## App and Scene

The active Model contains an App Render-to-Shader binding value but not a
complete App or Scene owner. Their source pressure is documented in
[`../parser/app`](../parser/app/) and
[`../parser/scene`](../parser/scene/).

When implemented, App owns process composition and Scene transitions. Scene
owns reusable state, lifecycle roles, render roots, and typed signals. Runtime
storage and transition execution remain outside Model.

## Foreign

Foreign imports require real Addressable, Writable, and Callable owners plus a
private source-local surface. Those facts are not implemented in the current
Model.

The source contract lives in
[`../parser/foreign`](../parser/foreign/). Provider choice, process addresses,
target relocations, and linker matches never become Model facts.

## Implementation guardrails

- Do not add `final` to a Model class by default. A closed inheritance boundary
  requires an observable invariant documented beside the declaration.
- Do not persist Cursor positions, token ranges, parser state, target records,
  runtime cells, source paths, or archive coordinates as semantic substitutes.
- Do not introduce pointer-valued absence or failure. Semantic failure is
  Invalid; closed non-semantic alternatives use the existing Union/Option
  abstractions.
- Do not add private helper members merely to carry transaction state. Keep
  implementation-only helpers local to their `.cpp` owner.
- Do not create a Member wrapper, Route object, resolver layer, registry,
  shadow Type graph, or compatibility alias to reconcile mismatched owners.

Existing declarations that violate these rules are migration work, not
precedent.
