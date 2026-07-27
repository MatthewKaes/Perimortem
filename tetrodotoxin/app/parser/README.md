# App Parser

App grammar describes how a completed program starts and how it retains
control. Environment Workspace owns the graph lifetime, parses the universal
source envelope, and routes the remaining cursor to the installed App Dialect.
The Dialect creates one App Monograph in the Workspace arena. That Monograph
owns one startup profile and one lifecycle policy.

App owns startup configuration, lifecycle policy, direct semantic edges, and
the platform entry flow derived from them. Library lowers completed CPU
executable facts. Linker binds support libraries and emits the platform
artifact.

## Startup profile

Every App selects one profile:

```ttx
runtime = Windowed {
  .title = "Scene Lifetime",
  .icon = $[resources/icon.png],
  .width = 800,
  .height = 600,
  .resizable = true,
}
```

`Windowed` selects native window and graphics support. `Terminal` selects
terminal input and output. `Headless` provides no presentation surface. These
are App profiles, not Runtime Package exports.

Package assembly selects the sole completed App Monograph regardless of its
authored Source name or member filename. `main.ttx` is only a convention.

Embedded paths resolve from the Package root. Future Package construction must
supply the confined bytes to source interpretation.

## Callable lifecycle

`Managed` and `Unmanaged` retain a direct typed edge to a completed Library
Static Callable:

```ttx
lifecycle = Managed {
  run Some::File -> launch,
}
```

The declaration can have any authored name. App validates its required Layout.
Execution never searches for `main`.

Echo introduces a one shot `Program` lifecycle:

```ttx
lifecycle = Program {
  start Main -> run,
}
```

`Program` retains one Static Callable that takes no parameters and returns
Void. Generated platform entry code invokes it once. Command line arguments are
not injected into that Layout, so authored code queries process state through
the linked System surface when needed. The intended relationship between
`Program`, `Managed`, and `Unmanaged` still needs one explicit inventory
decision before all three can become accepted grammar.

## Scene lifecycle

The Scene policy retains one initial Scene and transition edges keyed by
producing Scene and signal identities:

```ttx
lifecycle = Scene {
  initial Scenes::Splash;
  on Scenes::Splash.finished replace Scenes::Title;
  on Scenes::Title.shift_pressed replace Scenes::Splash;
  on Scenes::Title.space_pressed exit;
}
```

`replace` releases the active instance and prepares a fresh destination.
`push` pauses and retains the active instance before preparing a fresh
destination. `pop` releases the active instance and resumes the retained
instance below it. `exit` releases the complete stack from top to bottom
without resuming it.

Transitions occur after the active Scene update returns. App owns the live
Scene stack and transition policy. Scene owns its state, signals, and lifecycle
roles.

The intended acceptance order uses two application fixtures:

1. [`../../../apps/ttx/echo`](../../../apps/ttx/echo/) is the first Terminal
   target. It exercises Package membership, an App entry policy, one Library
   Callable, CPU compilation, linking, and terminal input and output without
   graphics or Scene management.
2. [`../../../apps/ttx/scene_lifetime`](../../../apps/ttx/scene_lifetime/) is
   the broader Windowed target. It adds resources, graphics startup, Scene
   lifecycle roles, signals, and transitions.

## Status

The App Dialect, App Monograph, confined resource input, platform entry planner,
and lifecycle executor are not implemented. Echo is not yet a frozen acceptance
oracle because its Source bindings, visibility, and Terminal profile are
internally unresolved. The callable contract for `Program` is recorded above,
while its relationship to the older callable policy names remains open. The
Scene Lifetime sources record the broader accepted grammar, but parsing their
universal envelopes alone does not establish App behavior.
