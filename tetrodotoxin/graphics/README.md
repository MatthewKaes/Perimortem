# Graphics

Graphics is Tetrodotoxin's language neutral boundary between completed Scene
state and a rendering backend. It is comparable to a retained presentation tree
followed by an immutable frame submission. Scene owns its exact state Fields and
lifecycle. Graphics owns the common contract needed to collect hosted values
without teaching Scene about Vulkan, a window system, or GPU commands.

Use Graphics when several Tetrodotoxin languages and runtime components need to
agree on what is presented while keeping target representation outside the
semantic graph. A direct renderer is simpler when one application already owns
its scene model and no cross language semantic identity needs to survive.

Graphics is not a Dialect. It defines concrete runtime and submission contracts
used by Scene, App, Render, Shader, and backend consumers. It adds no TTX
category, generic scene node, global registry, or second semantic graph.

## Hosted graphics state

A hostable graphics value has a concrete Type that proves the Graphics hosting
contract. The proof is owner specific and does not make every value share one
Field inventory. The standard `Perimortem.Graphics` Package supplies Library
Types such as `Image`, `Sprite`, point, size, and tone values that satisfy this
contract.

A private `state` Field initialized with `new` is hosted when its exact Object
Type proves this contract. The Field and Object remain their real Library
identities. Scene uses the Field's authored order and mutates the Object through
its ordinary Library Layout. Graphics does not copy those Fields into a
universal node record or ask Scene to author a second declaration form.

The Field remains ordinary runtime state. Assignment changes the Object that a
later submission observes through that Field. An alias may observe the same
Object. Only a retained hosted Field creates a hosting edge.

Every recursive hosting edge follows the same private `state` and `new` rule.
Field identity and authored order define the traversal. The current Object
value supplies the runtime graphics state. Library rejects a mandatory
construction cycle before any Scene instance is created.

## Frame submission

After one Scene update completes, Graphics collects the hosted subtree in stable
Field order. Hosted Fields inside the current Object values follow the same
rule. The resulting submission tree comes from the real Field relationships. A
second semantic hierarchy would lose the identity Scene and Library already
preserve.

Visibility and transform compose from host to hosted value. An invisible host
removes its complete subtree from the submission. Relative transforms are
applied in tree order before the renderer sees the resulting presentation
facts.

Draw order is stable and observable. Lower `z_index` values are behind higher
values. When two hosted values have the same index, the later Field in authored
tree order is in front. A valid unconfigured value contributes no draw rather
than becoming an invalid semantic object.

A frame submission contains only the presentation facts needed by a renderer,
including exact retained resources, transform, size, tone, visibility, order,
and the selected Render and Shader contracts.

The submission is an identity free runtime value. It does not own Scene
transitions or semantic objects. App waits for the submission to become stable
before applying a transition, which prevents release or replacement from
changing the frame while a consumer reads it.

## Backend boundary

A backend derives target images, buffers, descriptor bindings, draw batches,
and commands from one complete submission. Perimortem Graphics owns reusable
image decoding and backend independent draw data. A target backend such as
Vulkan owns device resources, command recording, synchronization, and
presentation.

Render and Shader facts constrain that derivation but do not become backend
reflection records. Vulkan handles and target offsets never enter Scene,
hosted graphics state, a Package Archive, or the live TTX graph.

## Durability

Graphics has no Dialect payload because it owns no authored Monograph. Library
and Scene payloads record the Types, hosted Fields, resources, and semantic
edges needed to reconstruct their graph. A frame submission and every backend
object are runtime state and are never restored from Package Archive.

See [Scene](../scene/README.md) for hosted state and lifecycle,
[Render](../render/README.md) for rendering interfaces, and
[Shader](../shader/README.md) for GPU implementations. The
[standard packages](../../packages/ttx/README.md) define the concrete Graphics
Types used by the repository sources.
