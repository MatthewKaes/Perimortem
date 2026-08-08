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
owner supplied Abstract context used while constructing that Monograph. The
concrete Dialect decides what contextual instructions that object supports.

The interpretation boundary is:

```text
interpret(
  Environment owned Arena,
  forward TTX Cursor,
  opening Documentation,
  owner supplied Abstract interpretation context)
-> Option<Monograph&>
```

A concrete Dialect consumes only its body because Environment has already
parsed the opening comment and `dialect : Type;` instruction. It constructs its
concrete Monograph directly in the supplied Arena and returns no partially
owned parser object. The fourth argument does not replace the Workspace wide
registry retained by the installed Dialect. It is not a universal source scope,
Type interface, or promise that every Monograph exports the same context.

Dialect state may cache or retain facts required across its Monographs. It does
not own the Environment Arena or the authored source provider.

## Monograph

`Language::Monograph` is the shared Abstract root for one interpreted or
restored source island. Its concrete derived class owns the source Dialect
semantics and resolution rules. The common root is not a Type or scope base. A
concrete Monograph may expose one Type, several Types, or another arbitrary
contextual shape without changing this interface.

The common base retains the Arena domain supplied by Environment, opening
Documentation, and ordered `Language::Diagnostic` facts. A Diagnostic carries
an optional exact Anchor plus owner produced message and hint. Authored
failures supply that Anchor while synthetic and restored failures leave it
absent. It owns no path or source bytes. Environment combines it with the
separately retained source Origin and uses the Origin boundary when no authored
Anchor exists.

The Arena outlives every retained Monograph. The common root does not retain
the installed Dialect, parser, Cursor, or a second Source wrapper.

## Contextual resolution values

Language owns two cross Dialect Abstract contracts for values returned by a
concrete semantic context. `Language::Resource` carries only Arena stable bytes
acquired by that context's real owner. It has no filesystem handle, route
grammar, diagnostic path, Type, Constant policy, or consumer semantics. Empty
bytes remain a successful Resource.

`Language::Error` proves that the context recognized an instruction and
resolved it to a stable owner-specific failure identity. The concrete owner
retains the cause while the consuming parser constructs the authored Anchor
and Report. Error
is not one shared enum, message record, provenance model, or textual Report.

Both contracts extend TTX Abstract without adding a TTX v1 category. An
ordinary missing semantic lookup still returns shared TTX Invalid. Resource and
Error exist so Package, Library, Shader, and later concrete Dialects can share
one contextual resolution boundary without sharing filesystem or value-domain
policy.

## Linking, finalization, and persistence dispatch

The shared lifecycle separates three graph transactions.

1. A concrete Dialect interprets grammar into stable source shaped identities.
   Unknown Type and Addressable routes remain authored facts rather than parse
   failures.
2. Workspace invokes `link()` on every Monograph in one frozen discovery range
   before any finalizer runs. Linking connects exact semantic edges after the
   complete batch has published its declaration identities.
3. Only a fully linked range invokes `finalize()` on every Monograph. A failed
   finalizer does not skip later owners and prevents terminal publication of
   the complete range.

Both graph hooks return failure without receiving a textual error sink. A
concrete Monograph publishes every durable Diagnostic it can establish, and
Retention combines those facts with its separately retained authored Origin.
No Cursor, Token index, declaration mirror, or parser transaction substitutes
for the retained semantic graph.

The persistence hooks name no Package envelope, concrete Dialect value, native
object, or universal terminal. Package owns Archive framing. The concrete
Dialect owns payload schema and restoration into the importing Workspace Arena.
Language owns only the common dispatch. Workspace uses restoration while
staging source free dependencies, then applies the same link and finalize
barriers to the complete range.

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

The production Workspace completes this flow for authored and source free
Package roots. Package local members remain within that exact root context, and
installed concrete Dialects restore their own member payloads before the
ordered link and finalize barriers.
