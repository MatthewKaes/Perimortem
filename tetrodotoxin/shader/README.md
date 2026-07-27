# Shader Parser

`Tetrodotoxin::Shader::Parser` owns the body grammar that
implements one exact Render contract with Stage Callables and Shader owned
Stage Bodies.

It constructs one concrete Shader Abstract root in the Arena owned by
`Tetrodotoxin::Language::Source`. The Shader root owns its definitions,
resolution rules, Stage identities, and completed semantic facts.

The parser does not own SPIR V module emission, target validation, package
terminal paths, or archive storage. `Tetrodotoxin::Shader::Assembler::SpirV`
owns source independent SPIR V word emission within the Shader subsystem.

## Source contract

`Language::Source::parse` consumes the leading Documentation and
`dialect : Shader;` envelope once. A future toolchain map may bind the exact
name to static `Tetrodotoxin::Shader::Parser::parse`, then pass it the same
Cursor. The Shader parser consumes only the remaining body. A Shader definition
selects one exact Render identity and implements its required Stages:

```ttx
shader TestShader : Formats::Simple {
  func Fragment[.color : Vec4D] -> [.color : Vec4D] {
    state copied : Vec4D = color;
    return (.color = copied);
  }
}
```

The concrete Shader grammar owns:

1. Shader name and exact Render query;
2. one implementation for each required Stage;
3. Stage parameter and result Layouts;
4. Shader Stage Body construction;
5. declared constant, push, and resource access;
6. locations, builtins, sets, slots, and address space facts; and
7. explicit representation edges where semantic and target Types differ.

Managed Types are not admitted into GPU values merely because they are
available through a binding context. A matching Structured Layout does not
create vector or ABI identity.

## Single consumption

Every Stage body is consumed once into its semantic Stage owner. Shader does
not assume the eventual CPU executable representation accepted by Library
compilation. A smaller shared value or control component may be extracted only
after both real implementations prove the same requirements. The parser does
not retain source ranges for Shader lowering, and Shader lowering does not
reopen the Tokenizer. Library's CPU compiler does not participate in this
path.

The outer `Language::Source` owns source bytes, Tokens, Arena, and the Shader
root lifetime. The future Environment Graph may retain completed Shader edges,
but it does not own or mirror Shader declarations.

## Status

The current structural fixture is
[`../../../validation/data/ttx/shader_artifact/shader.ttx`](../../../validation/data/ttx/shader_artifact/shader.ttx).
There is no active Shader parser or semantic evaluator in the current
`//tetrodotoxin:shader` target.
