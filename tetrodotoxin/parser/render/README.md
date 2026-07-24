# Render Parser

The Render parser owns the source grammar that declares render value state,
constants, push values, resources, and the Stage Callable contracts a Shader
must implement.

The parser constructs Render semantic owners. Target representation, SPIR-V
planning, runtime submission, and Graphics transactions belong to their
separate Tetrodotoxin owners.

## Source boundary

A complete Render document starts with Documentation and its selected top-level
Dialect envelope. The body must make every render-facing role explicit:

- ordinary value Addressables;
- constant data;
- push values;
- resources and their binding facts;
- required Stages;
- Stage input and result Layouts;
- builtin and location attributes; and
- declared read sets.

Attributes are distinct scalar facts. A binding that needs a set and slot uses
separate attributes. Structural Layout coincidence does not establish a Shader
representation or Stage interface.

## Name conflict to resolve

Tetrodotoxin's design currently calls this top-level Dialect `Render`, while the
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
construct target-specific field descriptions. Shader validation consumes the
real Render identities after their semantic owners are complete.

## Status

There is no active Render parser. The existing fixture proves tokenizable source
shape only.
