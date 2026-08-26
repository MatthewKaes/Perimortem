# Scene

Interactive state is easier to understand when its data, lifecycle, signals,
and graphics relationships live together. Scene gives that retained piece of
an application a language shaped around how it actually behaves.

Each Scene explains how one instance prepares, updates, pauses, resumes, and
releases its state. [App](../app/README.md) owns the live Scene stack and chooses
when the application moves between them. A Scene can focus on its own world
without learning which Scene came before it or which one comes next.

Canonical grammar reference: [Scene.g4](grammar/Scene.g4).

```ttx
// Retained scene state.
dialect : Scene;
```

## Library layer

Scene uses the Library language installed in its Workspace. Library provides
the Object, Field, Function, and expression rules used by Scene state. If
Library is not installed, the Scene source cannot be completed. Scene never
creates a private Library copy, so shared and Generic Types keep the same
identity throughout the Workspace.

Each completed Scene contains one Library child:

```text
Scene Monograph
├── Signals, lifecycle roles, frame events, and render relationships
└── Library child
    ├── Source context for imports and Foreign declarations
    └── Object representing one Scene instance
        ├── state Fields
        ├── helper Functions
        └── lifecycle Self Callables
```

Only the outer Scene appears as a Package member. It points directly to the real
Library Object, Fields, and Callables rather than copying them into a separate
Scene Type. Library tools can inspect the child directly. Scene aware tools use
the outer layer to see Signals and render relationships as well.

Scene and its Library child complete as one operation. Their errors appear
together in source order. When a Scene is restored from an Archive, the child
reads its own stored section and uses the same Package context as the outer
Scene.

## Lifecycle roles

A Scene provides `prepare`, `update`, and `release`. It may also provide `pause`
and `resume`:

```ttx
Scene prepare[self] -> []
Scene pause[self] -> []
Scene resume[self] -> []
Scene update[self, .delta_time : R64] -> []
Scene release[self] -> []
```

Each role names a Self Callable hosted by the Scene Object. Its parameters,
expressions, and ordinary statements follow Library rules. The `emit` statement
belongs to Scene because Library does not need to know about signals.

`prepare` and `release` bracket one live instance. App `push` pauses the active
instance before retaining it. App `pop` releases the active instance and resumes
the one below it. A paused Scene receives no update calls.

## Signals and emission

A Signal is a named event owned by a Scene. It can carry no value or one value
of a declared Type. A Signal is not stored state, a Function, or a Boolean flag:

```ttx
signal finished;
signal loaded : Result;

emit finished;
emit loaded(result);
```

`emit` selects one of the current Scene's Signals and checks its optional value.
It is a Scene statement rather than Field access such as `self.finished`. The
Scene reports what happened, while App decides whether that Signal means
`push`, `pop`, `replace`, or `exit`.

Signals emitted during `update` enter a frame event queue in order. Once the
Scene and Graphics have finished the frame and the backend has presented it,
the queue becomes visible to subscribers. Delivery keeps the same order. A
Signal emitted while the queue is being delivered waits for the next frame,
which prevents recursive delivery.

App is the first subscriber. It applies at most one transition per frame. The
first event with a matching App rule wins, and no matching event leaves the
current Scene in place. More general connections can be added without changing
what a Signal means.

Events delivered to another worker use a read only View whose borrow lasts for
the completed handoff call. The receiving worker copies any event data it keeps
after that call. Scene Objects and writable Access values never cross workers.
Persistent receiving state uses a newly constructed Object identity.

## Package identity

A Scene's Package Source route gives it a stable identity:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
```

The paths only locate the source inside Package storage. A directory or filename
does not become the Scene's identity.

## Hosted graphics state

A Scene instance owns its hosted graphics state. A private `state` Field is
hosted when its Object Type supports the Graphics hosting contract. The
ordinary Object default creates a fresh identity, while an explicit `new` can
make that construction visible when the source benefits from it:

```ttx
private state icon_top : Graphics::Sprite;
private state icon_bottom : Graphics::Sprite = new[Graphics::Sprite];
private state icon_shader : Graphics::Shader::DefaultTexturedQuad2D::Instance;
```

These Fields are the real hosted objects. Scene does not build a second node
tree or ordering table beside them. It finds hosted state from each Field's Type
and keeps the order written in the source. The Objects exist before `prepare`,
which configures them through ordinary Library access:

```ttx
self.icon_top.texture = Graphics::Texture2D -> from_image(.image = image);
self.icon_top.shader = self.icon_shader;
self.icon_top.transform = transform;
self.icon_shader.parameters.tone = tone;
```

The Sprite retains the concrete Shader Instance through
`Implementation[Render::TexturedQuad2D]`, so the Scene can keep editing
`icon_shader.parameters` directly while the next frame sees the same identity.

Graphics follows the current Object values through these Fields. Visibility and
transforms compose through that tree. `z_index` sets the main draw order, and a
later Field appears in front when two values share the same index. Assigning a
new Object to a hosted Field changes what the next frame submits.

`release` runs before the Scene gives up its hosted graphics values. Source code
cannot observe when the final reference releases one of those Objects. An Object
Field whose Type does not support Graphics hosting remains ordinary Scene state.

## Time and input

Delta time is scheduler input after `self`:

```ttx
Scene update[self, .delta_time : R64] -> [] {
  self.elapsed = self.elapsed + delta_time;
  if (self.elapsed > 1.0) {
    emit finished;
  }
}
```

Elapsed time and process input are ordinary Scene state. The runtime refreshes
one read only `System::Input` snapshot at each frame boundary. Terminal and
Headless applications use the same lifecycle because `update` has no
graphics specific parameter.

## Resources and persistence

Embedded assets resolve beneath the source Package root. Package keeps those
resources confined and alive while Scene or its Library child interprets their
bytes.

Scene can be stored in a Package Archive and reconstructed without its source
file. A Complete payload keeps its public and private Signal, lifecycle,
and Library query contracts. A Contract payload keeps their public closure and
compiled artifact locations. Graphics hosting is derived again from the
restored real Fields, so the Archive needs no parallel render inventory.
Executable Scene behavior remains in the compiled artifacts.

Neither profile stores a live Scene instance, current Object values, queued
frame events, elapsed time, input state, backend resources, or source level
debugging data.

See [App](../app/README.md) for transition policy,
[Library](../library/README.md) for Object, Field, Option, and Callable
semantics, and the [standard packages](../../packages/ttx/README.md) for the
Graphics and System Types used by Scene sources.
