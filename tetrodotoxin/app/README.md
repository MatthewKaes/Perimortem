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
// Application policy.
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

`Windowed` requests a native window and graphics presentation. The authored
named Pack supplies `title`, `icon`, `width`, `height`, and `resizable` and fits
the Windowed profile Layout. The leading dots name produced Pack slots rather
than postfix Address access.

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

The selected Callable has an empty parameter Layout and an empty result Layout.
Static means the call has no implicit Self value. App still retains the exact
source and Callable selected by the declaration. It does not require the
Callable's language to publish a concrete `Void` Type.

Generated platform entry code invokes it once. The Function may have any
authored name, and the source file may have any Package member name. App does
not search for a conventional `main` Function.

Command line arguments are queried through the linked System package rather
than injected into the entry Signature.

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
submission facts for that frame are stable. Scene owns state, signals, hosted
graphics relationships, and lifecycle roles. App owns movement between Scene
identities.

## Windowed execution

The Windowed Scene driver realizes App policy without becoming another semantic
owner. It opens the declared System window, creates the initial Scene instance,
and calls `prepare` before the first frame.

Each frame follows one observable order:

1. System completes one immutable input snapshot and a monotonic delta time.
2. App calls `update` on the active Scene exactly once.
3. Scene and Graphics make that frame's submission stable.
4. The selected backend presents the stable submission.
5. App applies the returned Scene transition.

A resize changes the System surface and the target presentation extent. It does
not replace the active Scene or rewrite its semantic identities. Shutdown
releases the Scene stack according to App policy before destroying the backend
and window resources that realized it.

## Package selection

Package assembly selects the App Monograph that provides the application policy.
Its Source route is the semantic name used to select that Monograph. `main.ttx`
is only a filename convention.

Embedded startup resources resolve beneath the App source's Package root. The
Package retains their bytes and App interprets their role in the startup
profile.

## Persistence

App is a persistent Dialect. Its payload records the startup profile, exact
resource relationships, selected Static entry Callable, initial Scene, and
signal transition edges needed to reconstruct the policy in a fresh Workspace.
It does not record a live Scene stack, process state, window, or generated entry
code.

See [Scene](../scene/README.md) for Scene roles,
[Library](../library/README.md) for Callable and named Layout semantics, and the
[standard packages](../../packages/ttx/README.md) for the source visible System
terminal, argument, and input surfaces.
