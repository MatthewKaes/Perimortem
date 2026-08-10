# App

App is Tetrodotoxin's program level policy language. It plays the role normally
split between an entry declaration, application manifest, and lifecycle
configuration. An App selects how a completed program starts, which
presentation surface it requires, and how its long lived state is controlled.

Use App when those decisions should remain semantic and refer directly to
Library Callables or Scene identities. App retains the selected relationships
while Library owns CPU lowering and Linker owns the final platform artifact.
This keeps startup policy inspectable without turning App into an instruction or
linking language.

Canonical grammar reference: [App.g4](grammar/App.g4).

```ttx
dialect : App;
```

## Startup profiles

Every App selects one startup profile.

### Windowed

```ttx
runtime = Windowed {
  .title = "Scene Lifetime",
  .icon = $[resources/icon.png],
  .width = 800,
  .height = 600,
  .resizable = true,
}
```

`Windowed` requests a native window and graphics presentation. Its named Layout
contains `title`, `icon`, `width`, `height`, and `resizable`. The leading dots
name Layout entries rather than postfix Address access.

### Terminal startup profile

The `Terminal` startup profile requests standard terminal input and output
without a window or Scene stack.
This source name describes App policy, not the Terminal product boundary.

### Headless

`Headless` provides no presentation surface. It is suitable for services,
workers, and batch programs whose dependencies provide their external I/O.

## Program lifecycle

Program lifecycle retains one Static Callable as the application entry:

```ttx
lifecycle = Program {
  start Main -> run,
}
```

The selected Callable takes no parameters and returns `Void`. Static means the
call has no implicit Self value. App still retains the exact source and Callable
selected by the declaration.

Generated platform entry code invokes it once. The Function may have any
authored name, and the source file may have any Package member name. App does
not search for a conventional `main` Function.

Command line arguments and process state are queried through linked system
interfaces rather than injected into the entry Signature.

## Scene lifecycle

Scene lifecycle retains one initial Scene and maps Scene signals to transitions:

```ttx
lifecycle = Scene {
  initial Scenes::Splash;
  on Scenes::Splash.finished replace Scenes::Title;
  on Scenes::Title.shift_pressed replace Scenes::Splash;
  on Scenes::Title.space_pressed exit;
}
```

App owns the live Scene stack and four transition operations:

* `replace` releases the active Scene and prepares a new destination.
* `push` pauses and retains the active Scene before preparing a new destination.
* `pop` releases the active Scene and resumes the retained Scene below it.
* `exit` releases the complete stack from top to bottom without resuming it.

A transition is applied after the active Scene has finished its update and its
submission facts for that frame are stable. Scene owns state, signals, children,
and lifecycle roles. App owns movement between Scene identities.

## Package selection

Package assembly selects the App Monograph that provides the application policy.
Its Source route is the semantic name used to select that Monograph. `main.ttx`
is only a filename convention.

Embedded startup resources resolve beneath the App source's Package root. The
Package retains their bytes and App interprets their role in the startup
profile.

See [Scene](../scene/README.md) for Scene roles and
[Library](../library/README.md) for Callable and named Layout semantics.
