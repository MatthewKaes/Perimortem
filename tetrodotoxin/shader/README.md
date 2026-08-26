# Shader

Shader lets GPU code live inside the same semantic project as the CPU code and
rendering contract it serves. It implements authored
[Render](../render/README.md) interfaces, reuses CPU visible
[Library](../library/README.md) Types where the two sides meet, and carries the
finished GPU program toward SPIR-V.

That shared contract is the reason Shader exists. A Stage can be checked against
the program that will call it before a graphics backend chooses bindings or
machine representation. Standalone GLSL and HLSL workflows remain useful when
that cross language relationship is not needed.

Canonical grammar reference: [Shader.g4](grammar/Shader.g4).

```ttx
// GPU implementation.
dialect : Shader;

public TestShader : shader Formats::Simple {
  public gain : uniform R32 = new[R32](1.0);

  public fragment : func {
    state copied : Math::Vec4D = (
      .x = color.x * parameters.gain,
      .y = color.y,
      .z = color.z,
      .w = color.w,
    );
    return (.color = copied);
  }
}
```

`Formats::Simple` is contextual Type access. `.color` names entries in Stage
parameter and result Layouts rather than postfix Address access.

Every Program and member begins with the same Definition envelope used by
Library: Documentation, Attributes, visibility, modifiers, name, and `:`. The
qualifier that follows selects Shader, Library, or relationship meaning. Shader
therefore interprets only the contract after `shader`. A `uniform` creates one
authored Field in the Program's generated Parameters Type, while `func` supplies
only a Stage body because the complete Signature is inherited from Render.

## One meaning, three responsibilities

Shader follows the same division as the other Tetrodotoxin languages. Its
Interpreter understands Shader syntax and directs each declaration to its real
owner. A real Library child keeps the Types, Functions, expressions, Blocks, and
Flow authored by each Stage. The outer Shader graph keeps Programs, Render
contract selection, storage roles, and CPU to GPU Bridges. Archive support
reconstructs that observable graph without becoming another Shader model.

This is what lets GPU lowering begin from completed meaning. The SPIR-V
Terminal walks the Shader and Render graph after interpretation, linking, and
finalization have finished. It does not need to recover a language from parser
nodes or from backend reflection.

## Composition through real language owners

Shader uses the Library and Render languages installed in its Workspace. If
either language is missing, the source cannot be completed. Reusing those
installed languages keeps shared Types consistent across the whole Workspace.
Shader does not create a second executable model, and it does not depend on
Vulkan.

Each completed Shader has this shape:

```text
Shader Monograph
├── one Library child with Program Types, Stage Functions, and Flow
├── exact references to selected Render contracts
└── exact Bridges between Library Types used across the graphics boundary
```

Each Program owns a generated `Parameters` Structure containing its authored
uniform Fields and a generated `Instance` Object containing one public mutable
`parameters` Field. The concrete Instance is the CPU visible owner of runtime
Shader state.

Inherited Fields and Stage slots retain the exact Types already resolved by the
Render owner. Shader does not replay Render's authored route spellings inside
the application Package. Archive restoration rebuilds those generated edges
from the restored Render contract, which lets an application implement a
dependency owned Render interface without reexporting its internal members.

The Library child is genuine because the Shader source directly authors its
Functions and execution graph. Neighboring Library and Render sources remain
ordinary Workspace members owned by their own transactions. A tool follows each
exact graph edge to its real owner, and Workspace completion keeps the whole
semantic island consistent.

## Render contracts and Stage bodies

A Shader definition selects one Render interface and supplies one body for every
Stage that contract requires. The generated Function inherits its complete
parameter and result Layouts. Resources, push inputs, builtins, locations,
address spaces, and capabilities are projected from the real Render declarations
before body linking.

Stage Functions and their bodies use the canonical Library parser and semantic
owners. Shader decides which Library operations and Types are legal for GPU
execution, while the SPIR V Terminal chooses their representation. Constants,
inherited push values, resources, uniforms, and local state therefore keep one
executable graph. Render declarations retain their real identities, while
Shader creates only the deterministic Library projections required for
execution.

Stage code can use Library construction, access, Packs, operations, control Flow,
and named swizzles. Shader validation limits that complete language where a GPU
target cannot preserve its meaning.

TTX Interface negotiation connects the real Library Function to the Render Stage
requirement. Layout fitting proves the data flow shape. Render Attributes add
the resource, location, builtin, and capability meaning that a Layout
deliberately leaves out.

The same relation applies to nested contract Types. For example,
`Render::TexturedQuad2D::Inputs` remains a Render Structure while
the Program generates one related Library projection used by Stage bodies. The
projection is not authored, is not a second contract owner, and is restored with
the same query surface from an Archive.

A similar Layout does not make two CPU, GPU, or ABI Types interchangeable. A
managed Library Type cannot become a GPU value merely because their fields look
alike.

## CPU to GPU bridges

Every bridge names two distinct Library Type identities used on opposite sides
of the graphics boundary, the direction data moves, how it is converted or
marshaled, and when it must be synchronized. Shader owns this relationship
because it understands both uses. Matching Layouts can help prove that a bridge
is valid, but they never turn the two Types into the same Type.

Each bridge spells out the policy another tool would otherwise have to guess:

```ttx
@direction("upload")
@marshal("copy")
@sync("submission")
public vertices : bridge Cpu::Vertex -> Gpu::Vertex;
```

Direction may be `upload`, `download`, or `bidirectional`. Marshaling may keep
an identical semantic shape, copy values, or pack a different shape.
Synchronization may be `none`, `submission`, or `frame`. These remain portable
Shader facts. Vulkan, another graphics API, or an offline compiler decides how
to realize them.

Shader describes uploads, downloads, and marshaling without choosing a graphics
API. Once the Shader and its Render contract are complete, the SPIR-V compiler
chooses a GPU representation. The CPU compiler follows the selected Library
meaning independently. Neither
side tries to reconstruct the other from offsets or reflection data.

## Access

Stage bodies follow Library access and execution semantics:

* `value.name` selects an Addressable from a named Layout.
* `context::Type` resolves one Type.
* `receiver -> callable(arguments...)` invokes a Callable admitted by Shader.
* `.[...]` selects named Layout flow.

The keyword forms `and` and `or` keep Library short circuit semantics. Reserved
`&` and `|` Tokens remain available for separately defined bitwise operations.

Render Attributes on Shader definitions, resources, and Stage entries describe
the interface facts they implement. Shader rejects an Attribute that is not
allowed by the selected Render contract.

## SPIR-V and Vulkan

The SPIR-V Terminal chooses the GPU representation, storage classes, bindings,
and instructions while the completed live Shader still retains executable
bodies. The generated SPIR-V is an output of compilation, not an input to the
language model. During Package production Linker embeds each completed module
as named read only native data, so source free application composition can use
that product without a loose shader file beside its executable.

Exact R64 flow remains R64 in SPIR V. A module that needs it declares Float64,
64 bit constants retain both literal words, real remainder uses `OpFRem`, and
an authored `new[R32](value)` emits `OpFConvert`. Vulkan enables Float64 only
when the selected physical device reports support.

Portable application parameters can use bounded U32 ticks instead. Their push
bytes remain exact, and `new[R32](ticks)` emits `OpConvertUToF`. Keeping the
bounded tick below the exact integer range of R32 avoids Float64 while leaving
cycle interpretation with the Shader that owns the effect.

Shader keeps graphics API independent marshaling and synchronization
requirements. Vulkan later consumes the generated SPIR-V, Graphics batches,
and a selected host surface. It chooses concrete offsets and descriptor
bindings, creates handles and command buffers, synchronizes the device, and
presents the result. Vulkan specific rules stay in that Terminal instead of
leaking into Library, Render, or Shader's shared bridge.

## Persistence

Shader can be stored in a Package Archive and reconstructed without its source
file. A Complete payload keeps its public and private Shader relationships and
the complete query contract of its Library child. A Contract payload keeps the
public Library and GPU contracts plus neighboring relationship routes. The
neighboring Render payload remains with its real Package member, and the
Package product connects these reconstructed facts with compiled artifact
locations. Executable bodies remain source or live Workspace facts.

Neither profile stores live backend handles, commands, device resources,
generated SPIR-V, or source level debugging data.

See [Render](../render/README.md) for authored GPU interfaces,
[Library](../library/README.md) for reusable execution semantics, and
[TTX semantics](../../ttx/ttx_semantics.md) for shared Type, Layout, and
Interface contracts.
