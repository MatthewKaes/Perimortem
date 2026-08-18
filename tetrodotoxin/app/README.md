# App

App is Tetrodotoxin's program-level policy language. It plays the role normally
split between an entry declaration, application manifest, and lifecycle
configuration. An App selects how a completed program starts, which
presentation surface it requires, and how its long-lived state is controlled.

App points directly to the Library Functions or Scenes that take part in that
policy. Library compiles the CPU code and Linker builds the platform program.
App keeps the startup and lifecycle choices visible without becoming a machine
instruction or linking language.

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

`Windowed` requests a native window and graphics presentation. Its settings are
`title`, `icon`, `width`, `height`, and `resizable`. The leading dots name those
settings rather than selecting Fields from another value.

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
Callable's language to publish a zero-value Type.

Generated platform entry code invokes it once. The Function may have any
authored name, and the source file may have any Package member name. App does
not search for a conventional `main` Function.

Command line arguments are queried through the linked System package rather
than injected into the entry Signature.

## Scene lifecycle

Scene lifecycle names one initial Scene and maps its Signals to transitions:

```ttx
lifecycle = Scene {
  initial Scenes::Splash;
  on Scenes::Splash.finished replace Scenes::Title;
  on Scenes::Title.shift_pressed replace Scenes::Splash;
  on Scenes::Title.space_pressed exit;
}
```

App owns the live Scene stack and four transition operations:

- `replace` releases the active Scene and prepares a new destination.
- `push` pauses and retains the active Scene before preparing a new destination.
- `pop` releases the active Scene and resumes the retained Scene below it.
- `exit` releases the complete stack from top to bottom without resuming it.

At the end of each frame, App checks the events published by the active Scene in
the order they were emitted. The first event with a matching `on` rule wins.
Other subscribers may still observe later events, but App performs no second
transition during that frame. If no event matches, the Scene stack stays as it
is.

Scene owns the signals and decides when to emit them. App owns the transition
rules and the live Scene stack.

## Windowed execution

The Windowed Scene driver applies App policy at runtime. It opens the declared
System window, creates the initial Scene instance, and calls `prepare` before
the first frame.

Each frame follows one observable order:

1. System completes one read-only input snapshot and a monotonic delta time.
2. App calls `update` on the active Scene exactly once.
3. Scene and Graphics make that frame's submission stable.
4. The selected backend presents the stable submission.
5. Scene publishes the frame's events in emission order.
6. App applies the first matching transition, if there is one.

An event emitted while the current events are being delivered waits for the
next frame. Scene never returns a transition from `update`. The `on` rules in
App are the only source of transition policy.

A resize changes the System surface and the presentation size. It does not
replace the active Scene or rewrite its language objects. Shutdown
releases the Scene stack according to App policy before destroying the backend
and window resources that realized it.

## Package selection

Package assembly selects the App result that provides the application policy.
Its Source route is the stable Package name used to select it. `main.ttx` is
only a filename convention.

Embedded startup resources resolve beneath the App source's Package root. The
Package retains their bytes and App interprets their role in the startup
profile.

## Persistence

App supports both Package Archive profiles. Complete stores the full startup and
lifecycle policy. Interface stores the public policy and the location of the
compiled program, without executable bodies.

Referenced Scenes remain separate Package members. App stores the relationships
to those members instead of copying their data. Neither profile stores a live
Scene stack, queued events, process state, an open window, generated code, or
debug information.

## Target and host selection

Building an App selects both a CPU target and a platform host. The CPU target
defines how functions and values are represented. The Linux or Windows host
provides process startup, loading, terminals, windows, and events.

Library compiles CPU code with LLVM. Linker produces the final ELF or PE
program without making LLVM part of App behavior.

See [Scene](../scene/README.md) for Scene roles,
[Library](../library/README.md) for Callable and named Layout semantics, and the
[standard packages](../../packages/ttx/README.md) for the source-visible System
terminal, argument, and input surfaces.
