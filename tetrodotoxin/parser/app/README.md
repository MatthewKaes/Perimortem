# App Parser

The App parser owns process-level source composition: application options,
initial Scene selection, Scene transition policy, render roots, and explicit
Render-to-Shader selection.

It constructs App semantic facts. Worker-local execution, Realm storage,
Graphics submission, and cleanup belong to Tetrodotoxin Runtime and Graphics.

## Source contract

A complete App document starts with Documentation and `dialect : App;`.

The current canonical design-pressure source is
[`../../../apps/ttx/scene_lifetime/main.ttx`](../../../apps/ttx/scene_lifetime/main.ttx).
It separates window policy from Scene composition:

```ttx
scene {
  initial Scenes::Splash;
  on Scenes::Splash.finished replace Scenes::Title;
  on Scenes::Title.shift_pressed replace Scenes::Splash;
  on Scenes::Title.space_pressed exit;
}
```

App owns the initial Scene and every transition. A transition key resolves to
the real Scene and signal identities. Replace, push, pop, and exit are App
policy; a Scene never names its successor.

The cyclic Splash-to-Title-to-Splash transition is a runtime state-machine
cycle, not a source dependency cycle. Both Scene owners must exist before App
connects their edges.

## Semantic handoff

The parser consumes composition directly into the App owner. It does not store
runtime strings, source paths, token ranges, or magic lifecycle names.
Runtime later follows direct semantic role and transition edges.

## Status

No App parser or evaluator is active. The scene-lifetime source is a
human-review fixture. Package parsing alone is not evidence that App semantics,
runtime transitions, or archive records work.
