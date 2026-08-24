# Graphics

A Scene should be able to describe what exists without learning how Vulkan
records a command buffer. Graphics makes that separation practical. It reads
real hosted Objects after an update and gathers one stable frame submission,
leaving Scene focused on interactive meaning and the backend focused on
presentation.

Graphics is a runtime composition contract rather than another Dialect. It
does not create a generic node Type, copy Scene Fields, or retain a second
Shader model. The same Objects authored through Library remain the source of
the frame.

## A semantic hosting requirement

The ordinary `Perimortem.Graphics` Package publishes `Host` as a Library
Structure. Its public state describes the transform, visibility, and ordering
facts a hosted Object promises. A concrete Object can carry additional state
and behavior while satisfying that requirement.

Graphics uses a higher order TTX Interface to negotiate the real Host Type
against the real candidate Object Type. Every required public state Field must
remain visible, mutable instance state with the exact promised Type. Matching
Layout alone is insufficient because two values with the same storage shape
need not share graphics meaning.

This relation is directional. A Sprite may satisfy Host without Host replacing
Sprite or erasing the Sprite identity. Scene therefore keeps its exact Field
and Object relationships, while a Terminal can derive the runtime traversal
behavior only after the semantic proof succeeds.

## Runtime traversal

At runtime, a compact descriptor reads one completed Object payload. It exposes
that Object's local placement, its hosted children in authored order, and any
draws it contributes. The descriptor contains behavior rather than a copied
Field table, so replacing an Object in a real Field changes the value inspected
for the next frame.

Visibility and transforms compose while Graphics walks the hosted tree. An
invisible value removes its complete subtree. Cycles are rejected because a
hosted edge describes containment for one frame even when ordinary Object
references elsewhere may form richer relationships.

## Stable frame submissions

Each accepted draw becomes one immutable Batch. Graphics copies its input bytes
and transform, retains its worker local resource Objects, and keeps the authored
traversal order used to break equal draw indices. Batches are then ordered from
back to front by `z_index`, with a later authored value remaining in front when
indices match.

The completed Submission no longer borrows mutable Scene state. Scene can
change or release its hosted Objects after collection without changing the
frame being presented. Resource reservations remain alive through that frame
and release naturally when the Submission leaves scope.

A Batch carries only an opaque process lifetime locator for its selected
compiled Program. SPIR-V words, descriptor layouts, and Vulkan pipelines remain
sibling target products. The backend receives those products beside the
Submission and never asks Graphics to recompile or reinterpret Shader meaning.

## Rendering backends

Perimortem Graphics owns reusable decoded image and draw data. Vulkan owns its
pipeline descriptions, device resources, command recording, synchronization,
and presentation. Its descriptions are derived from completed Render, Shader,
and SPIR-V products and live beneath the Vulkan subsystem rather than becoming
Graphics semantics.

Graphics has no Archive payload of its own. Library and Scene Archives preserve
the Types, Fields, resources, and hosting relationships needed to rebuild the
semantic program. Submissions, resource reservations, and backend objects are
live runtime values.

See [Scene](../scene/README.md) for hosted application state,
[Render](../render/README.md) for rendering requirements,
[Shader](../shader/README.md) for GPU programs, and the
[standard packages](../../packages/ttx/README.md) for the authored Graphics
Types.
