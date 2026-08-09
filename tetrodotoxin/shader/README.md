# Shader

The Shader Dialect implements one exact Render contract. It owns Shader Types,
Stage bodies, resource access, and the semantic facts required for GPU lowering.

Grammar prototype: [Shader.g4](grammar/Shader.g4).

```ttx
dialect : Shader;

shader TestShader : Formats::Simple {
  func Fragment[.color : Vec4D] -> [.color : Vec4D] {
    state copied : Vec4D = color;
    return (.color = copied);
  }
}
```

`Formats::Simple` is contextual Type access. `.color` names entries in the Stage
parameter and result Layouts; it is not postfix Address access.

## Render implementation

A Shader definition selects one Render identity and supplies every Stage that
contract requires. Each Stage must fit the declared parameter and result
Layouts and satisfy the required resource, built-in, location, and address-space
facts.

Structural Layout coincidence does not create Shader Type or ABI identity. A
managed CPU Type is not admitted into a GPU value merely because both expose a
similar Layout.

## Stage bodies

Shader owns the grammar and semantics of Stage bodies. Constants, push values,
resources, and local state become Shader facts directly. They do not pass
through Library's CPU expression or executable-body model.

When a Type has distinct semantic and target representations, Shader retains an
explicit edge between those identities. Lowering consumes that completed edge
rather than reconstructing meaning from source Tokens.

## Access

Shader follows the shared TTX access domains:

- `value.name` selects an Addressable from a named Layout;
- `context::Type` resolves a Type through contextual access;
- `receiver -> callable(arguments...)` selects and invokes a Callable admitted by
  Shader grammar;
- `.[...]` selects named Layout flow;
- `[...]` requests indexed reference access from writable `Access[T]`, while
  `:[...]` selects a safe indexed value.

## Lowering boundary

Shader lowering chooses GPU representation, storage classes, bindings, and
instructions after semantic validation. The Shader assembler emits
source-independent SPIR-V words from those completed decisions.

Package owns durable payload framing and artifact identity. Runtime graphics
owns submission. Neither one reinterprets Shader source grammar.

See [Render](../render/README.md) for the interface being implemented and
[TTX semantics](../../ttx/ttx_semantics.md) for the shared Type and Layout
contracts.
