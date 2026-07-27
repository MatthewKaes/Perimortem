# Scene

Scene grammar describes one retained Scene object. Environment Workspace owns
the graph lifetime, parses the universal source envelope, and routes the
remaining cursor to the installed Scene Dialect. The Dialect creates one Scene
Monograph in the Workspace arena.

The Monograph owns Scene state, signals, render facts, completed Library
Callables, and direct edges assigning those Callables to lifecycle roles. App
owns transitions between Scene identities and the live Scene stack.

## Lifecycle

A Scene provides `prepare`, `update`, and `release`. It may also provide
`pause` and `resume`:

```ttx
Scene prepare[self] -> Void
Scene pause[self] -> Void
Scene resume[self] -> Void
Scene update[self, .delta_time : Real_64] -> Scene::Flow
Scene release[self] -> Void
```

`prepare` and `release` bracket one live instance. App `push` calls `pause`
before retaining an instance. App `pop` releases the active instance and calls
`resume` on the instance below it. A paused Scene receives no `update` calls.

The role prefix creates a direct edge to an ordinary Library Self Callable.
Generated lifecycle code follows that edge rather than searching for a
conventional function name.

An update returns `Scene::Flow -> stay()` or emits one of its own signals. A
Scene never selects another Scene or terminates the process directly. App maps
the emitted identity to transition policy.

Scene identity comes from the exact local name in its Package Source binding:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
```

The paths select confined inputs only. Package never derives a Scene name from
a directory or filename.

## Time and input

Delta time is the only explicit argument after `self`. It is scheduler input,
so Terminal and Headless Apps need no Graphics dependency:

```ttx
Scene update[self, .delta_time : Real_64] -> Scene::Flow
```

Elapsed Scene time is retained state and accumulated from that argument:

```ttx
state elapsed : Real_64 = 0.0;

self.elapsed = self.elapsed + delta_time;
```

Input remains global retained state. A Package requests
`Perimortem.System` under an authored alias, and Scene code queries that alias:

```ttx
resolve System : Perimortem.System = "1.0";

const input := System -> get_input();
```

The exact System input API is a dependency contract rather than part of the
Scene update ABI.

## Resources

Embedded paths resolve from the Package root. Future Package construction must
supply the confined bytes to source interpretation and may share retained bytes
for repeated reads before constant folding.

The canonical fixtures are
[`../../apps/ttx/scene_lifetime/scenes/splash.ttx`](../../apps/ttx/scene_lifetime/scenes/splash.ttx)
and
[`../../apps/ttx/scene_lifetime/scenes/title.ttx`](../../apps/ttx/scene_lifetime/scenes/title.ttx).
They record state, embedded resources, required lifecycle roles, the intended
System input query, signals, and an App owned transition cycle. They do not yet
record `pause` or `resume`.

## Status

The Scene Dialect, Scene Monograph, confined resource input, lifecycle executor,
System input API, and durable Scene schema are not implemented. The canonical
sources record the accepted grammar and provide future acceptance fixtures.
Parsing their universal envelopes alone does not establish Scene behavior.
