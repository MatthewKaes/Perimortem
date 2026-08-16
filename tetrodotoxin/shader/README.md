# Shader

Shader is Tetrodotoxin's language for writing GPU programs that work with both
[Render](../render/README.md) interfaces and CPU-visible
[Library](../library/README.md) Types. It checks each stage against its Render
contract, describes how data moves between CPU and GPU forms, and compiles the
finished GPU program to SPIR-V.

Shader is designed for Tetrodotoxin programs whose CPU and GPU sides share an
authored contract. It does not try to replace every standalone GLSL, HLSL, or
platform-specific shader workflow.

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

`Formats::Simple` is contextual Type access. `.color` names entries in Stage
parameter and result Layouts rather than postfix Address access.

## CPU and GPU layers

Shader uses the Library and Render languages installed in its Workspace. If
either language is missing, the source cannot be completed. Reusing those
installed languages keeps shared Types consistent across the whole Workspace.
Shader does not create private copies of them, and it does not depend on Vulkan.

Each completed Shader has this shape:

```text
Shader Monograph
├── selected Render contracts and CPU↔GPU bridge facts
├── Library CPU child
│   └── CPU helpers, host representations, and marshaling facts
└── Render GPU child
    └── concrete GPU Types, resources, values, and Stage body facts
```

Only the outer Shader Monograph appears as a Package member. A tool interested
in Library can ask the Shader for its CPU layer, while a Render tool can ask for
its GPU layer. A Shader-aware tool can inspect both layers and the bridge between
them. These are the real child layers, not copies in a second Shader-only model.

Shader completes both children as one operation. Errors from the outer Shader
and either child appear together in source order. When a Shader is restored from
an Archive, each child reads its own stored section and uses the same Package
context as the outer Shader.

## Render contracts and Stage bodies

A Shader definition selects one Render interface and supplies every Stage that
contract requires. Each Stage must fit the declared parameter and result
Layouts and satisfy required resources, builtins, locations, address spaces,
and capabilities.

Shader decides how stages are written and whether their implementation is
legal. The resulting GPU values, expressions, resources, and bodies live in the
Render child. They do not pass through Library's CPU expression model.
Constants, push values, resources, and local state remain GPU values throughout
the toolchain.

Shader can construct a GPU value explicitly with `Type(arguments...)`. Named
swizzles select and reorder entries from a GPU Layout. Shader checks whether
these operations are legal, while the Render child keeps the resulting values.

A similar Layout does not make two CPU, GPU, or ABI Types interchangeable. A
managed Library Type cannot become a GPU value merely because their fields look
alike.

## CPU-to-GPU bridges

Every bridge names one Library Type and one Render Type, the direction data
moves, how it is converted or marshaled, and when it must be synchronized.
Shader owns this relationship because it is the layer that understands both
sides. Matching Layouts can help prove that a bridge is valid, but they never
turn the two Types into the same Type.

Shader describes uploads, downloads, and marshaling without choosing a graphics
API. Once the Shader and its Render contract are complete, the SPIR-V compiler
chooses a GPU representation. The CPU compiler uses the Library child. Neither
side tries to reconstruct the other from offsets or reflection data.

## Access

Shader follows the shared TTX access domains:

- `value.name` selects an Addressable from a named Layout.
- `context::Type` resolves one Type.
- `receiver -> callable(arguments...)` invokes a Callable admitted by Shader.
- `.[...]` selects named Layout flow.

The keyword forms `and` and `or` own short-circuit logic. Reserved `&` and `|`
Tokens are not alternate spellings and remain available for separately defined
bitwise GPU operations.

Render Attributes on Shader definitions, resources, and Stage entries describe
the interface facts they implement. Shader rejects an Attribute that is not
allowed by the selected Render contract.

## SPIR-V and Vulkan

The SPIR-V backend chooses the GPU representation, storage classes, bindings,
and instructions. It can emit validated SPIR-V from a completed Shader without
reading the source again. The generated SPIR-V is an output of compilation, not
an input to the language model.

Shader keeps graphics-API-independent marshaling and synchronization
requirements. Vulkan later consumes the generated SPIR-V, Graphics batches,
and a selected host surface. It chooses concrete offsets and descriptor
bindings, creates handles and command buffers, synchronizes the device, and
presents the result. Vulkan-specific rules stay in that backend instead of
leaking into Library, Render, or Shader's shared bridge.

## Persistence

Shader can be stored in a Package Archive and rebuilt without its source file.
A Complete payload keeps the stage organization, contracts, bridge details, and
Complete Library and Render children needed to compile it again. An Interface
payload keeps the public CPU and GPU contracts, bridge details, and compiled
artifact locations, but leaves out executable bodies.

Neither profile stores live backend handles, commands, device resources,
generated SPIR-V, or source-level debugging data.

See [Render](../render/README.md) for the interface and reusable GPU layer,
[Library](../library/README.md) for CPU semantics, and
[TTX semantics](../../ttx/ttx_semantics.md) for shared Type and Layout
contracts.
