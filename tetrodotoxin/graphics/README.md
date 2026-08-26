# Graphics

A Scene describes what exists without learning how Vulkan records a command
buffer. Graphics is the handoff between those worlds. It reads the real Objects
owned by a completed Scene and freezes the values needed to present one frame,
while Scene keeps interactive meaning and the backend keeps device state.

Graphics is not a Dialect and it does not build another scene graph. Library's
structural Interface proves placement relationships over existing Types. Shader
Programs separately prove that their concrete Instance Objects implement one
exact Render contract.

## Independent runtime capabilities

The ordinary `Perimortem.Graphics` Package publishes `Placement2D` as a Library
Structure requirement. Its public state names transform, visibility, and draw
order. A Sprite can satisfy that requirement while retaining its own Type,
Fields, behavior, and lifetime.

Runtime behavior stays divided by purpose:

* `Placement2D` reads transform, visibility, and z index.
* `Children2D` reads ordered child Objects and their configured Type indices.
* `Drawable2D` extracts draws, exact Program locators, retained resources, and
  frozen Shader parameter bytes.

Sprite has Placement2D and Drawable2D. A compiled Scene root has Children2D. A
future nonvisual container can have only Children2D, while a group can combine
Placement2D and Children2D. Generated application tables keep the three
provider arrays separate, so no common descriptor or runtime class registry is
needed.

## Textures, Shaders, and Sprites

`Image` owns decoded CPU pixels. `Texture2D` gives one Image stable rendering
identity and sampling meaning. A backend can cache a device image by that real
Object while keeping target images, views, samplers, descriptors, and upload
state inside Vulkan.

Sprite stores one `Implementation[Render::TexturedQuad2D]`. The value retains a
real Shader `Instance` Object and the immutable ABI Projection derived for that
exact implementation. The Projection selects the generated Program and the
byte range of the Instance's `Parameters` subobject. It contains no semantic
graph pointer, route string, or target handle.

```ttx
private state shader : Graphics::Shader::DefaultTexturedQuad2D::Instance;

self.sprite.texture = texture;
self.sprite.shader = self.shader;
self.shader.parameters.tone = (
  .x = new[R32](1.0),
  .y = new[R32](1.0),
  .z = new[R32](1.0),
  .w = new[R32](alpha),
);
```

The concrete Shader Instance remains the parameter owner. Frame collection
copies its current parameter bytes, so later Scene mutations appear in the next
stable frame without copying those Fields into Sprite.

## Render and Shader agreement

`Render::TexturedQuad2D` owns the fixed image resource, base push inputs, Stage
signatures, vertex layout, topology, blending policy, geometry, and vertex
count. A Shader author writes only the Stage bodies and custom uniforms.

Each Shader Program generates one Library `Parameters` Structure containing
the authored uniform Fields and one `Instance` Object containing `parameters`.
The Program inherits executable projections of the Render declarations during
the cross Dialect composition barrier. Render remains the contract owner and
Shader remains the executable owner.

```text
Render::TexturedQuad2D
  fixed resources, inputs, Stages, and pipeline agreement

Shader::DefaultTexturedQuad2D
  Parameters { tone }
  Instance { parameters }
  executable Stage bodies

Application::GlitchTexturedQuad2D
  Parameters { phase_milliseconds }
  Instance { parameters }
  executable Stage bodies
```

Application composition discovers every Program reached by the Scene's concrete
Shader Instance Types. Each generated Vulkan description carries its exact
module locator, host layout, descriptor bindings, vertex inputs, pipeline facts,
and device capabilities.

## Stable frame submissions

Submission walks Children2D after the Scene update. Placement visibility removes
a complete subtree, transforms compose through the path, and cycles reject the
frame. Higher z indices appear in front, with authored traversal order retained
for equal indices.

Each accepted draw becomes one immutable Batch. Graphics copies the current
Shader parameter bytes and composed transform, retains the Texture2D resources
used by that frame, and carries the exact process lifetime Program locator.

Vulkan consumes ordered Batches together with the generated Program table. It
builds a distinct pipeline for each locator, fills target host roles, caches
device textures by Texture2D identity, and owns command recording,
synchronization, and presentation. None of those target facts flow back into
Graphics, Render, Shader, or Scene.

See [Scene](../scene/README.md) for application state,
[Render](../render/README.md) for rendering contracts,
[Shader](../shader/README.md) for GPU implementations, and the
[standard packages](../../packages/ttx/README.md) for the authored Graphics
surface.
