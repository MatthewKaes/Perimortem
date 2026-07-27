# Tetrodotoxin Language

`tetrodotoxin/language` defines the common extension boundary for top level
Tetrodotoxin source Dialects. The complete folder builds as
`//tetrodotoxin:language`.

Language depends on TTX and Perimortem. It owns no concrete Package, Library,
App, Scene, Render, or Shader semantics and no filesystem, compiler, runtime, or
durable product policy.

## Dialect

`Language::Dialect` is a stateful interpreter installed by
`Environment::Workspace` under an exact authored name. Environment supplies the
shared TTX registry at construction and keeps the Dialect alive for every
Monograph it creates.

The interpretation boundary is:

```text
interpret(
  Environment owned Arena,
  forward TTX Cursor,
  opening Documentation,
  shared Abstract registry)
-> Option<Dialect::Monograph&>
```

A concrete Dialect consumes only its body because Environment has already
parsed the opening comment and `dialect : Type;` instruction. It constructs its
concrete Monograph directly in the supplied Arena and returns no partially
owned parser object.

Dialect state may cache or retain facts required across its Monographs. It does
not own the Environment Arena or the authored source provider.

## Monograph

`Language::Dialect::Monograph` is the shared Abstract root for one interpreted
source island. Its concrete derived class owns the source Dialect semantics and
resolution rules.

The common base retains the Arena domain supplied by Environment, the opening
Documentation, and the host Dialect that interpreted it.

The host Dialect and Arena outlive every retained Monograph. This makes the
Monograph the stable semantic root without introducing a second Source wrapper
or copying its graph.

## Shared parser fragments

Shared parsers consume the same forward `Ttx::Lexical::Cursor` used by the
selected Dialect. They retain no declaration inventory, token bookmark, or
transaction state.

`Parser::Comment` greedily consumes consecutive comment Tokens. It strips the
comment marker and one canonical separating space, preserves all remaining text
and empty authored lines, and constructs one compact Documentation Block in the
Cursor Arena. When no comment begins at the Cursor it returns the shared empty
Documentation without advancing.

`Parser::Dialect` consumes:

```ttx
dialect : Package;
```

It returns the exact authored Type shaped name. Environment owns lookup,
unknown Dialect diagnostics, and dispatch to the installed instance.

## Ownership boundary

Language contains no static parser map, parser inheritance hierarchy, Frontend,
Source lifetime object, Container, semantic Namespace, filesystem capability,
or concrete definition model.

A parser fragment belongs here only when several real Dialects share both its
grammar and its returned contract. Type, Generic, Constant, expression, body,
and publication rules remain with the concrete language that defines them.

Package demonstrates the intended extension:

```text
Environment installs Package::Dialect as "Package"
-> Environment parses the universal envelope
-> Package::Dialect::interpret consumes the Package body
-> Package::Language::Monograph becomes the imported root
```

The current Package implementation does not yet complete this flow. The example
defines the Language boundary rather than claiming a working import.
