# Render Parser

`Tetrodotoxin::Render::Parser` owns the body grammar that
declares render value state, constants, push values, resources, and the Stage
Callable contracts a Shader must implement.

The parser constructs one concrete Render Abstract root in the Arena owned by
`Tetrodotoxin::Language::Source`. The Render root owns its definitions,
resolution rules, and semantic owners. Shader owns SPIR V representation and
assembly. Runtime submission and Graphics transactions belong to their
separate Tetrodotoxin owners.

## Source boundary

`Language::Source::parse` consumes Documentation and the selected top level
dialect envelope once. A future toolchain map may bind the resolved exact name
to static `Tetrodotoxin::Render::Parser::parse`, then pass it the same Cursor.
That binding remains unavailable until the name conflict below is resolved. The
Render parser consumes only the remaining body. That body must make every
render facing role explicit:

1. ordinary value Addressables;
2. constant data;
3. push values;
4. resources and their binding facts;
5. required Stages;
6. Stage input and result Layouts;
7. builtin and location attributes; and
8. declared read sets.

Attributes are distinct scalar facts. A binding that needs a set and slot uses
separate attributes. Structural Layout coincidence does not establish a Shader
representation or Stage interface.

## Name conflict to resolve

Tetrodotoxin's design currently calls this top level Dialect `Render`, while the
live structural fixture
[`../../../validation/data/ttx/shader_artifact/render.ttx`](../../../validation/data/ttx/shader_artifact/render.ttx)
uses `dialect : Gpu;` and constructs a `ShaderFormat`.

No Render parser is implemented, so this spelling and owner conflict is not
silently resolved here. The parser must not accept both names as compatibility
aliases. The concrete owner and canonical source must be selected before
implementation.

## Semantic handoff

A successful parser will pass complete declarations directly to the Render
model. It will not retain Stage token ranges, copy Addressable records, or
construct target specific field descriptions. Shader validation consumes the
real Render identities after their semantic owners are complete.

The outer `Language::Source` owns source bytes, Tokens, Arena, and the Render
root lifetime. The future Environment Graph may retain completed Render edges,
but it does not own or mirror Render declarations.

## Status

There is no active Render parser. The existing fixture proves tokenizable source
shape only.
