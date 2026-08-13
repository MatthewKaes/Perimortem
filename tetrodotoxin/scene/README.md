# Scene

Scene is Tetrodotoxin's language for one retained unit of interactive state. It
combines the ideas of a scene definition and a lifecycle contract. A Scene owns
its state, signals, hosted graphics relationships, render submission facts, and
lifecycle Callables.

Use Scene when an application needs authored state with explicit prepare,
update, pause, resume, and release behavior. App owns the live stack and the
policy that moves between Scene identities, so one Scene describes itself
without choosing its successor or global application flow.

Canonical grammar reference: [Scene.g4](grammar/Scene.g4).

```ttx
// Retained scene state.
dialect : Scene;
```

Each Scene Monograph owns one exact Scene Type. That Type hosts its state,
signals, helper Functions, and lifecycle Self Callables. The Monograph remains
the retained source root while the Scene Type is the receiver identity used by
a live instance. Neither role is replaced with a Library Monograph or a generic
scene node.

## Lifecycle roles

A Scene provides `prepare`, `update`, and `release`. It may also provide `pause`
and `resume`:

```ttx
Scene prepare[self] -> Void
Scene pause[self] -> Void
Scene resume[self] -> Void
Scene update[self, .delta_time : Real_64] -> Scene::Flow
Scene release[self] -> Void
```

The role prefix retains a direct edge to an ordinary Library Self Callable.
Generated lifecycle code follows that edge rather than searching for a
conventional Function name.

`prepare` and `release` bracket one live instance. App `push` pauses the active
instance before retaining it. App `pop` releases the active instance and resumes
the one below it. A paused Scene receives no update calls.

An update returns a `Scene::Flow` value or emits one of the Scene's signals:

```ttx
return Scene::Flow -> stay();
return Scene::Flow -> emit(self.finished);
```

The Scene does not select its successor directly. App maps the emitted identity
to `push`, `pop`, `replace`, or `exit` policy.

## Package identity

A Scene's semantic identity comes from its Package Source route:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
```

The paths locate confined inputs. Package never derives Scene identity from a
directory or filename.

## Hosted graphics state

A Scene instance is the root of its hosted graphics state. A private `state`
Field initialized with `new` is hosted when its exact Object Type proves the
Graphics hosting contract:

```ttx
private state top_icon : Graphics::Sprite = new;
private state bottom_icon : Graphics::Sprite = new;
```

The Fields are the real hosted identities. Scene does not create a second
hosting declaration, node, or ordering table. It discovers hosted state from
the exact Field Type and uses the Fields' authored order. Their nonnull Objects
construct before `prepare`, which configures those runtime values through
ordinary Address access:

```ttx
self.top_icon.image = image;
self.top_icon.position = (.x = 200, .y = 100);
```

An ordinary Scene helper may perform that configuration. Moving the writes
into a Function does not move construction or hosting away from the Field.

Hosted values follow the current Object values through the real Field
relationships. Visibility and transform compose through those relationships.
Authored Field order and `z_index` determine stable draw order. Scene publishes
render submission facts after update. Runtime submission lies outside Scene
semantics and does not change the semantic identity of a hosted Field.

A hosted Field remains ordinary Scene state. Assignment may replace the Object
reached through that Field. The next complete submission observes the new
value through the same Field identity. Scene does not infer single assignment
or ownership from the hosting relationship.

`release` runs before the Scene instance relinquishes its hosted graphics
roots. Library still makes Object reclamation timing and order unobservable. An
Object Field whose Type does not prove the Graphics contract remains ordinary
Scene state. Graphics submission selects only Fields whose Type proves that
contract.

## Time and input

Delta time is the scheduler input passed after `self`:

```ttx
Scene update[self, .delta_time : Real_64] -> Scene::Flow {
  self.elapsed = self.elapsed + delta_time;
  return Scene::Flow -> stay();
}
```

Elapsed time is ordinary Scene state. Terminal and Headless applications can use
the same lifecycle because the update Signature has no graphics specific
argument.

Process input is runtime Scene state. A Scene retains the current snapshot in a
`state` Field:

```ttx
private state input : System::Input = System -> get_input();
```

The update Callable refreshes that snapshot at the frame boundary:

```ttx
self.input = System -> get_input();
```

## Resources and submission

Embedded assets resolve beneath the source Package root. Scene or its Library
Types interpret the retained Resource bytes while Package preserves confinement
and lifetime.

App applies a transition only after update and the retained submission facts for
that frame are stable.

## Persistence

Scene is a persistent Dialect. Its payload records the Scene Type, state and
signal declarations, helper Functions, lifecycle role edges, and render
relationships needed to construct a fresh graph. Hosted order is recovered
from the reconstructed Fields and exact Graphics Type proofs. The payload does
not record a live Scene instance, Object runtime values, elapsed time, input
state, or backend resources.

See [App](../app/README.md) for transition policy,
[Library](../library/README.md) for Object, Field, and Callable semantics, and
the [standard packages](../../packages/ttx/README.md) for the Graphics and
System Types used by Scene sources.
