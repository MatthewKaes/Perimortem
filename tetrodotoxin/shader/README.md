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
  public fragment : func = [.color : Math::Vec4D] -> [.color : Math::Vec4D] {
    state copied : Math::Vec4D = color;
    return (.color = copied);
  }
}
```

`Formats::Simple` is contextual Type access. `.color` names entries in Stage
parameter and result Layouts rather than postfix Address access.

Every Program and member begins with the same Definition envelope used by
Library: Documentation, Attributes, visibility, modifiers, name, and `:`. The
qualifier that follows selects Shader, Library, or relationship meaning. Shader
therefore interprets only the contract and body after `shader`, while `func`
delegates its complete Signature and Block to Library.

## One meaning, three responsibilities

Shader follows the same division as the other Tetrodotoxin languages. Its
Interpreter understands Shader syntax and directs each declaration to its real
owner. A real Library child keeps the Types, Functions, expressions, Blocks, and
Flow authored by each Stage. The outer Shader graph keeps Programs, Render
contract selection, storage roles, and CPU to GPU Bridges. Archive support later
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

The Library child is genuine because the Shader source directly authors its
Functions and execution graph. Neighboring Library and Render sources remain
ordinary Workspace members owned by their own transactions. A tool follows each
exact graph edge to its real owner, and Workspace completion keeps the whole
semantic island consistent.

## Render contracts and Stage bodies

A Shader definition selects one Render interface and supplies every Stage that
contract requires. Each Stage must fit the declared parameter and result
Layouts and satisfy required resources, builtins, locations, address spaces,
and capabilities.

Stage Functions and their bodies use the canonical Library parser and semantic
owners. Shader decides which Library operations and Types are legal for GPU
execution, while the SPIR V Terminal chooses their representation. Constants,
push values, resources, and local state therefore keep one executable graph
instead of acquiring a Render shaped or Shader shaped copy.

Stage code can use Library construction, access, Packs, operations, control Flow,
and named swizzles. Shader validation limits that complete language where a GPU
target cannot preserve its meaning.

TTX Interface negotiation connects the real Library Function to the Render Stage
requirement. Layout fitting proves the data flow shape. Render Attributes add
the resource, location, builtin, and capability meaning that a Layout
deliberately leaves out.

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
and instructions. It can emit validated SPIR-V from a completed Shader without
reading the source again. The generated SPIR-V is an output of compilation, not
an input to the language model.

Shader keeps graphics API independent marshaling and synchronization
requirements. Vulkan later consumes the generated SPIR-V, Graphics batches,
and a selected host surface. It chooses concrete offsets and descriptor
bindings, creates handles and command buffers, synchronizes the device, and
presents the result. Vulkan specific rules stay in that Terminal instead of
leaking into Library, Render, or Shader's shared bridge.

## Persistence

Shader can be stored in a Package Archive and reconstructed without its source
file. A Complete payload keeps its public and private Shader relationships and
the complete contract of its Library child. A Contract payload keeps the public
Library and GPU contracts, neighboring relationship routes, and compiled
artifact locations. The neighboring Render payload remains with its real
Package member. Executable bodies remain source or live Workspace facts.

Neither profile stores live backend handles, commands, device resources,
generated SPIR-V, or source level debugging data.

See [Render](../render/README.md) for authored GPU interfaces,
[Library](../library/README.md) for reusable execution semantics, and
[TTX semantics](../../ttx/ttx_semantics.md) for shared Type, Layout, and
Interface contracts.
