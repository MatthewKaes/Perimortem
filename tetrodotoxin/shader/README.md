# Shader Dialect

The future `Tetrodotoxin::Shader::Dialect` owns the grammar that implements one
exact Render contract using Stage Callables and Shader owned Stage bodies.

Environment will install the concrete Shader Dialect and retain each resulting
Shader Monograph. After the universal envelope is parsed, Shader interpretation
consumes the remaining forward Cursor and constructs its semantic identities in
the Environment Arena.

The Shader Monograph owns its definitions, resolution rules, Stage identities,
and completed semantic facts.

## Source contract

A Shader definition selects one exact Render identity and implements its
required Stages:

```ttx
shader TestShader : Formats::Simple {
  func Fragment[.color : Vec4D] -> [.color : Vec4D] {
    state copied : Vec4D = color;
    return (.color = copied);
  }
}
```

Shader grammar owns the Shader name and exact Render query, one implementation
for each required Stage, the Stage parameter and result Layouts, and Shader
Stage body construction. It also defines declared constant, push, and resource
access. It retains locations, builtins, sets, slots, address space facts, and
explicit representation edges where semantic and target Types differ.

Managed CPU Types are not admitted into GPU values merely because they are
available through a binding context. A matching Structured Layout does not
create vector or ABI identity.

## Single consumption

Every Stage body is consumed once into its Shader semantic owner. Shader does
not assume the future CPU executable representation accepted by Library.

A shared value or control component may be extracted only after real Library
and Shader implementations prove identical requirements. Shader does not retain
source ranges for lowering, and lowering does not recreate a Cursor over
authored source.

Library's CPU compiler does not participate in Shader lowering.

## Assembler boundary

`Tetrodotoxin::Shader::Assembler::SpirV` owns source independent SPIR V word
emission within the Shader subsystem. It does not own Shader grammar, semantic
validation, package terminal paths, or archive storage.

The future Shader lowering path supplies decisions already proven against the
selected Render contract. Environment retains the source Monograph but does not
mirror Shader declarations.

## Status

The current structural fixture is
[`../../validation/data/ttx/shader_artifact/shader.ttx`](../../validation/data/ttx/shader_artifact/shader.ttx).
There is no active Shader Dialect, Monograph, parser, semantic validator, or
lowering transaction. The current target contains only the SPIR V assembler.
