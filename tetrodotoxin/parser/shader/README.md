# Shader Parser

The Shader parser owns the source grammar that implements one exact Render
contract with Stage Callables and common executable Bodies.

It does not own SPIR-V records, module emission, target validation, package
terminal paths, or archive storage.

## Source contract

A complete Shader document starts with Documentation and
`dialect : Shader;`. A Shader definition selects one exact Render identity and
implements its required Stages:

```ttx
shader TestShader : Formats::Simple {
  func Fragment[.color : Vec4D] -> [.color : Vec4D] {
    state copied : Vec4D = color;
    return (.color = copied);
  }
}
```

The concrete Shader grammar owns:

- Shader name and exact Render query;
- one implementation for each required Stage;
- Stage parameter and result Layouts;
- common Body construction;
- declared constant, push, and resource access;
- locations, builtins, sets, slots, and address-space facts; and
- explicit representation edges where semantic and target Types differ.

Managed Types are not admitted into GPU values merely because they are
available through a binding context. A matching Structured Layout does not
create vector or ABI identity.

## Single consumption

Every Stage body is consumed once into a semantic Stage owner and common Body.
The parser does not retain source ranges for the target compiler, and the
target compiler does not reopen the Tokenizer.

## Status

The current structural fixture is
[`../../../validation/data/ttx/shader_artifact/shader.ttx`](../../../validation/data/ttx/shader_artifact/shader.ttx).
There is no active Shader parser or semantic evaluator in the current
`//tetrodotoxin:parser` target.
