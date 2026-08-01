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
Workspace wide TTX registry at construction and keeps the Dialect alive for
every Monograph it creates. Each interpretation separately receives the exact
source local Abstract context used while constructing that Monograph.

The interpretation boundary is:

```text
interpret(
  Environment owned Arena,
  forward TTX Cursor,
  opening Documentation,
  source local Abstract interpretation context)
-> Option<Dialect::Monograph&>
```

A concrete Dialect consumes only its body because Environment has already
parsed the opening comment and `dialect : Type;` instruction. It constructs its
concrete Monograph directly in the supplied Arena and returns no partially
owned parser object. The fourth argument does not replace the Workspace wide
registry retained by the installed Dialect.

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

## Contextual resolution values

Language owns two cross Dialect Abstract contracts for values returned by a
concrete semantic context. `Language::Resource` carries only Arena stable bytes
acquired by that context's real owner. It has no filesystem handle, route
grammar, diagnostic path, Type, Constant policy, or consumer semantics. Empty
bytes remain a successful Resource.

`Language::Error` proves that the context recognized an instruction and
resolved it to a stable owner-specific failure identity. The concrete owner
retains the cause while the consuming parser retains the authored Span. Error
is not one shared enum, message record, provenance model, or textual Report.

Both contracts extend TTX Abstract without adding a TTX v1 category. An
ordinary missing semantic lookup still returns shared TTX Invalid. Resource and
Error exist so Package, Library, Shader, and later concrete Dialects can share
one contextual resolution boundary without sharing filesystem or value-domain
policy.

## Completion and persistence dispatch

The accepted shared lifecycle adds two owner neutral operations.

1. Workspace invokes one ordered post pass on each retained Monograph after
   local staging and dependency restoration drain.
2. Each concrete Dialect encodes and restores the opaque precompiled payload
   for its own Monograph kind.

The post pass may complete the same semantic objects reserved during
interpretation and may query the retained Workspace host. It never turns a
Cursor, Token index, source route, or declaration mirror into unfinished
semantic state. It returns failure instead of receiving a textual error sink.
The concrete Dialect logs details known only while completing its graph, while
Workspace retains the authored input identity needed to publish a user facing
diagnostic.

The persistence hooks name no Package envelope, concrete Dialect value, native
object, or universal terminal. Package owns Archive framing. The concrete
Dialect owns payload schema and restoration into the importing Workspace Arena.
Language owns only the common dispatch. Workspace uses those operations during
source-free dependency restoration and invokes post pass after its complete
staging queue drains.

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
concrete definition model, terminal registry, or cross owner product variant.

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

The production Workspace completes this flow for authored and source-free
Package roots. Package local members remain within that exact root context, and
installed concrete Dialects restore their own member payloads before the
ordered post pass.
