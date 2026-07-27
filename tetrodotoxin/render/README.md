# Render Dialect

The future `Tetrodotoxin::Render::Dialect` owns authoring for render value state,
constants, push values, resources, and the Stage Callable contracts that a
Shader must implement.

Environment will install the concrete Dialect under its selected exact name.
After Environment parses the universal source envelope, Render interpretation
will consume the remaining Cursor and construct one concrete Render Monograph in
the Environment Arena.

The Render Monograph owns its definitions, resolution rules, and semantic
identities. Shader owns SPIR V representation and assembly. Runtime submission
and Graphics transactions remain separate owners.

## Body contract

Render grammar must make every render facing role explicit. It defines ordinary
value Addressables, distinguishes constant data and push values, and retains
resources with their binding facts. It identifies every required Stage,
defines each Stage input and result Layout, retains builtin and location
Attributes, and declares read sets.

Attributes are distinct scalar facts. A binding that needs a set and slot uses
separate Attributes. Structural Layout coincidence does not establish a Shader
representation or Stage interface.

## Name conflict

The current design calls this top level Dialect `Render`, while the preserved
structural fixture
[`../../validation/data/ttx/shader_artifact/render.ttx`](../../validation/data/ttx/shader_artifact/render.ttx)
uses `dialect : Gpu;` and constructs a `ShaderFormat`.

No Render parser is implemented, so this conflict is deliberately unresolved.
Implementation must select one canonical name and source contract rather than
accepting both as compatibility aliases.

## Semantic handoff

A successful Render Dialect will construct complete declarations directly in
its Monograph. It will not retain Stage token ranges, copy Addressable records,
or construct target specific field descriptions.

Shader validation consumes the real Render identities after their semantic
owners are complete. Environment retains the Monograph and its host Dialect but
does not interpret or mirror Render declarations.

## Status

There is no active Render Dialect, Monograph, or parser. The existing fixture
proves tokenizable historical source shape only.
