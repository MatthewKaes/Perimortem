# Scene

The Scene Dialect describes one retained Scene identity: its state, signals,
declared children, render submission facts, and lifecycle Callables. App owns
the live Scene stack and transitions between Scene identities.

Grammar prototype: [Scene.g4](grammar/Scene.g4).

```ttx
dialect : Scene;
```

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

## Declared children

A Scene instance is the root of one owned child tree. `child` declarations
create nonnull values with stable identity for that Scene lifetime:

```ttx
child top_icon : Graphics::Sprite;
child bottom_icon : Graphics::Sprite;
```

Children construct and attach in authored order before `prepare`. Prepare
configures those existing values through ordinary Address access:

```ttx
self.top_icon.image = image;
self.top_icon.position = (.x = 200, .y = 100);
```

After update, visible attached graphics children are collected in retained tree
order. Scene code mutates child state; it does not issue an imperative draw call.
Equal depth follows sibling order, and visibility and transforms propagate
through graphics parents.

`release` runs before reverse-order destruction of the declared child subtree.
Dynamic attachment, detachment, and reparenting require explicit runtime
operations and do not change declared child identity.

## Time and input

Delta time is the scheduler input passed after `self`:

```ttx
Scene update[self, .delta_time : Real_64] -> Scene::Flow {
  self.elapsed = self.elapsed + delta_time;
  return Scene::Flow -> stay();
}
```

Elapsed time is ordinary Scene state. Terminal and Headless applications can use
the same lifecycle because the update Signature has no graphics-specific
argument.

Process input is queried through a linked dependency rather than added to the
Scene ABI:

```ttx
const input := System -> get_input();
```

## Resources and submission

Embedded assets resolve beneath the source Package root. Scene or its Library
Types interpret the retained Resource bytes while Package preserves confinement
and lifetime.

App applies a transition only after update and the retained submission facts for
that frame are stable.

See [App](../app/README.md) for transition policy and
[Library](../library/README.md) for Object, Field, and Callable semantics.
