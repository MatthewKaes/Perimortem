# Scene Parser

The Scene parser owns reusable managed-state source: Scene state, typed signals,
lifecycle roles, and render roots.

It constructs Scene semantic facts. App owns transitions, Runtime owns the
transition transaction, and Graphics remains unaware of Scene.

## Source contract

A complete Scene document starts with Documentation and `dialect : Scene;`.
The authored shape includes:

- Scene-owned state Addressables;
- signal declarations with complete payload Layouts;
- `prepare`, `update`, and `release` lifecycle roles;
- a real Self Callable and Body for each role; and
- explicit render roots.

The lifecycle prefix records a direct role edge to an ordinary Self Callable.
Runtime does not discover roles by searching conventional names.

A Scene update returns `Scene::Flow::stay` or emits one of that Scene's signals.
It never names another Scene and never decides process exit. App connects the
emitting Scene and signal to transition policy after all Scene owners exist.

Embedded resources use the shared embedded-resource literal contract and the
Source's Environment. The Scene parser does not open files.

## Canonical pressure fixture

[`../../../apps/ttx/scene_lifetime/scenes/splash.ttx`](../../../apps/ttx/scene_lifetime/scenes/splash.ttx)
and
[`../../../apps/ttx/scene_lifetime/scenes/title.ttx`](../../../apps/ttx/scene_lifetime/scenes/title.ttx)
exercise resource loading, state, lifecycle roles, typed input, signals, and a
cycle connected by the App owner.

## Status

No Scene parser, evaluator, transition runtime, or durable Scene schema is
implemented. Tokenization and Package membership do not establish those
behaviors.
