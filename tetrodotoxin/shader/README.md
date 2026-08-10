# Shader

Shader is Tetrodotoxin's GPU source language. Like a conventional shader
language, it defines typed Stage bodies, resources, and values that lower to
SPIR-V. Its distinguishing feature is that each Shader implements a semantic
Render contract shared with the rest of the program.

That relationship lets the host check a Stage interface before either side has
been reduced to a GPU ABI. Shader retains its own Types, resource rules, and
body semantics while exact edges connect them to the Render identities they
implement.

Use Shader when a Tetrodotoxin program needs GPU stages that compose with an
authored Render interface. The model requires that interface and is not intended
as a standalone replacement for every GLSL, HLSL, or platform shader workflow.

Canonical grammar reference: [Shader.g4](grammar/Shader.g4).

```ttx
// GPU implementation.
dialect : Shader;

shader TestShader : Formats::Simple {
  func Fragment[.color : Math::Vec4D] -> [.color : Math::Vec4D] {
    state copied : Math::Vec4D = color;
    return (.color = copied);
  }
}
```

`Formats::Simple` is contextual Type access. `.color` names entries in the Stage
parameter and result Layouts. It is not postfix Address access.

## Render contracts

A Shader definition selects one Render identity and supplies every Stage that
contract requires. Each Stage must fit the declared parameter and result
Layouts and satisfy the required facts for resources, builtins, locations, and
address spaces.

Structural Layout coincidence does not create Shader Type or ABI identity. A
managed CPU Type is not admitted into a GPU value merely because both expose a
similar Layout.

## Stage bodies

Shader owns the grammar and semantics of Stage bodies. Constants, push values,
resources, and local state become Shader facts directly. They do not pass
through Library's CPU expression or executable body model.

Shader may construct one of its concrete value Types explicitly with
`Type(arguments...)`. This is a Shader construction operation, not a Type value
flowing through the expression graph. Named swizzles select and reorder entries
from a Shader Layout. Shader defines the legality of those operations for its
own Types.

When a Shader Type implements a distinct Render Type, Shader retains an exact
edge between those semantic identities. Lowering consumes that completed edge
and derives its GPU representation without placing the backend Type in the
semantic graph.

## Access

Shader follows the shared TTX access domains:

* `value.name` selects an Addressable from a named Layout.
* `context::Type` resolves a Type through contextual access.
* `receiver -> callable(arguments...)` selects and invokes a Callable admitted
  by Shader grammar.
* `.[...]` selects named Layout flow.

Render Attributes on Shader definitions, resources, and Stage entries retain
the exact interface facts they implement. Shader rejects an Attribute that is
not admitted by the selected Render contract.

## Lowering boundary

Shader lowering chooses GPU representation, storage classes, bindings, and
instructions after semantic validation. The Shader assembler emits SPIR-V words
from those completed decisions without consulting source.

Package owns durable payload framing and artifact identity. Runtime submission
lies outside Shader semantics. Neither one reinterprets Shader source grammar.

Shader is a persistent Dialect. Its payload records the concrete Types, Stage
bodies, resource facts, Render identity edges, and Layout relationships needed
to construct a fresh semantic graph. SPIR-V is a separate Terminal product and
cannot substitute for that payload.

See [Render](../render/README.md) for the shared rendering interface and
[TTX semantics](../../ttx/ttx_semantics.md) for the shared Type and Layout
contracts.
