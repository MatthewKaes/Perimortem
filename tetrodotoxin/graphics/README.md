# Graphics

A Scene should be able to describe what exists without learning how Vulkan
records a command buffer. Graphics makes that separation practical. It reads
completed Scene objects and gathers one stable frame submission for the
renderer, while Scene remains focused on interactive meaning.

Graphics is a runtime contract shared by Scene, App, Render, Shader, and the
rendering backends rather than another Dialect. The same authored objects flow
through it directly, so there is no generic scene Type or copied graphics graph
between the application and renderer.

## Hosted graphics state

A Library Object can be hosted when its Type supports the Graphics contract.
The standard `Perimortem.Graphics` Package provides Types such as `Image` and
`Sprite` that support it.

Scene hosts one of these objects through a private `state` Field initialized
with `new[ObjectType]`:

```ttx
private state icon : Graphics::Sprite = new[Graphics::Sprite];
```

The Field remains an ordinary Library Field. Scene changes the Sprite through
normal field access, and Graphics reads the same Object when it prepares a
frame. There is no separate node declaration or copied field table.

Hosted Objects may contain more private state Fields that follow the same rule.
Graphics walks those real Fields in authored order. If Scene assigns a new
Object to a hosted Field, the next frame sees the replacement through the same
Field identity.

## Frame submission

After `update` finishes, Graphics walks the hosted tree and records the data a
renderer needs. This can include images, transforms, sizes, tones, visibility,
draw order, and the selected Render and Shader contracts.

Visibility and transforms flow from a host to its children. An invisible host
removes its whole subtree from the frame. Higher `z_index` values are drawn in
front of lower values. When two values have the same index, the Field authored
later is drawn in front.

The completed submission is a runtime value, not a collection of Scene objects.
The backend presents it before Scene publishes the frame's signals and before
App changes the active Scene. Scene replacement therefore cannot change a frame
while the renderer is reading it.

## Rendering backends

A backend turns the submission into images, buffers, bindings, draw batches,
and commands. Perimortem Graphics owns reusable image decoding and
backend independent draw data. Vulkan owns device resources, command recording,
synchronization, and presentation.

Render and Shader describe what the backend must produce. Vulkan handles and
target offsets remain backend details. They never enter Scene state, Package
Archives, or the live TTX graph.

## Archives

Graphics has no source language and therefore no Archive data of its own.
Library and Scene Archives store the Types, Fields, resources, and relationships
needed to rebuild hosted state. Frame submissions and backend objects are live
runtime data and are never restored from an Archive.

See [Scene](../scene/README.md) for hosted state and lifecycle,
[Render](../render/README.md) for rendering interfaces, and
[Shader](../shader/README.md) for GPU programs. The
[standard packages](../../packages/ttx/README.md) define the Graphics Types used
by the repository examples.
