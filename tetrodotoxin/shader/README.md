# Shader

Shader lets GPU code share the same semantic project as the CPU-visible
material and the [Pipeline](../render/README.md) it implements. One Shader
source owns one implementation. It does not manufacture a named Shader Type
inside a second source wrapper.

```ttx
// One textured material implementation.
dialect : Shader;

public Math : alias = package(.name = "Perimortem.Math", .version = "1.0");
implements source("../pipelines/textured_quad_2d.ttx");

public tone : uniform Math::Vec4D = (
  .x = new[R32](1.0),
  .y = new[R32](1.0),
  .z = new[R32](1.0),
  .w = new[R32](1.0),
);

Shader vertex[
  .position : Math::Vec2D,
  .texture_uv : Math::Vec2D,
] -> [
  .position : Math::Vec4D,
  .texture_uv : Math::Vec2D,
] {
  // Library expressions produce the vertex result.
}

Shader fragment[.texture_uv : Math::Vec2D] -> [.color : Math::Vec4D] {
  state sampled := image -> sample(.uv = texture_uv);
  state color : Math::Vec4D = (
    .x = sampled.x * parameters.tone.x,
    .y = sampled.y * parameters.tone.y,
    .z = sampled.z * parameters.tone.z,
    .w = sampled.w * parameters.tone.w,
  );
  return (.color = color);
}
```

Canonical grammar reference: [Shader.g4](grammar/Shader.g4).

## One owner and one requirement

`implements` accepts a common source or Package import expression. Because `::`
is ordinary Type access, the route can continue through any number of exported
names:

```ttx
implements package(
  .name = "Perimortem.Graphics",
  .version = "1.0",
)::Pipeline::TexturedQuad2D;
```

That expression selects the real Pipeline Monograph. It does not copy the
Pipeline, flatten its names, or create a private contract alias.

Each `Shader stage[...] -> [...]` declaration names the Stage explicitly and
authors the complete expected Layout in context. Shader compares that
signature with the selected Pipeline Stage before accepting its Library body.
The canonical declaration order is vertex followed by fragment; the formatter
preserves that semantic order instead of alphabetizing the two names.

## Executable and material meaning

Shader owns one real Library child because uniforms, Functions, expressions,
Packs, operations, control flow, and Constants use Library semantics. The outer
Shader Monograph owns the selected Pipeline edge, storage roles, and executable
entry relationship. Tools can follow those exact owners without a copied
interface graph.

Every completed Shader publishes one generated Library `Material` Object. Its
`parameters` Field selects a generated `Parameters` Structure containing the
authored uniforms. A resource with a CPU carrier also becomes a public Material
Field:

```ttx
public noise : resource read Graphics::Texture2D -> Graphics::Image;
public time_milliseconds : uniform U32 = new[U32];
```

Here Stage code samples `noise` as an Image, while CPU code configures the same
Material with a retained Texture2D. The immutable ABI projection records the
parameter byte range and ordered material resources. Sprite and Vulkan do not
need shader-specific Field names.

`Program` remains a private semantic and compiler owner for executable Stages.
It is not part of the authored or Package-facing API; consumers configure the
generated `Material`.

## Generated GPU interface

Pipeline Layout order determines ordinary Stage locations. Standard builtin
names determine builtin decorations. Resource order determines descriptor
order, and push Structures determine their generated storage layout. These are
backend products derived from the completed source graph rather than attributes
that authors must keep synchronized.

The Vulkan backend produces both sides of the contract:

* SPIR-V modules and entry metadata for the GPU;
* host ranges, resource projections, and material offsets for CPU submission.

The Vulkan runtime consumes those generated descriptions together with a
backend-neutral frame draw. It does not rediscover Shader Types or assume that
all Programs use one topology, blend mode, geometry, or vertex count.

Exact R64 flow remains R64 in SPIR-V and requires device Float64 support. A
portable animation can instead transport bounded U32 milliseconds losslessly
and convert with `new[R32](parameters.time_milliseconds)` inside its Shader.

## Persistence

A Shader Package member can be reconstructed from a source-free Archive. The
payload retains its private executable owner because the public Material is
generated from it, but Package lookup publishes only the Material surface. A
Contract profile retains the public semantic agreement needed by dependents;
generated SPIR-V and native ABI products remain Terminal artifacts.

See [Pipeline](../render/README.md) for the declarative requirement,
[Library](../library/README.md) for executable semantics, and
[Graphics](../graphics/README.md) for runtime draw ownership.
