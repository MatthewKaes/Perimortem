# App Parser

`Tetrodotoxin::App::Parser` consumes the App body and constructs one concrete
App Abstract root containing exactly one startup profile and one lifecycle
policy. `Tetrodotoxin::Language::Source` owns its text, Tokenizer, Arena, and
the App root lifetime. App owns its authored grammar, configuration legality,
direct semantic edges, and generated entry flow semantics.

App does not assemble instructions, encode object files, perform relocation,
or link a final binary. App owned executable facts are lowered by Library, and
Linker owns the final platform artifact.

## Startup profile

`Language::Source::parse` consumes the leading Documentation and
`dialect : App;` envelope once, finds the future static `App::Parser::parse`
function in the borrowed toolchain map, and passes it the same Cursor. The App
parser consumes only the remaining body and returns the concrete App root. It
does not tokenize the document again or retain an envelope bookmark or parser
map.

The required `runtime` field selects an App owned startup profile rather than a
resolved Runtime Package:

```ttx
runtime = Windowed {
  .title = "Scene Lifetime",
  .icon = $[resources/icon.png],
  .width = 800,
  .height = 600,
  .resizable = true,
}
```

The supported profile vocabulary is:

1. `Windowed` selects native window support, Vulkan support, generated platform
  entry code, and window package configuration.
2. `Terminal` selects native terminal startup and standard input and output
  support.
3. `Headless` selects background startup without a presentation surface.

The profile contributes App owned startup facts for `_start` or the
platform equivalent entry. Library lowers its completed CPU executable facts,
and Linker binds the selected support libraries and emits the final binary.
Neither operation converts the App into a Library Source or shadow graph.

An embedded icon uses the ordinary package root resource contract. The package
construction owner supplies bytes read through the confined
`Package::Workspace` capability. No App parser opens a filesystem path.

## Callable lifecycle

`Managed` and `Unmanaged` each retain a direct typed edge to a
`Tetrodotoxin::Library::Language::Callables::Static`. App construction resolves
the authored route after definitions are available, proves the Static contract
and required signature, and retains the real Callable in the finalized graph.
The finalized Graph exposes the Callable through its owning Library Language
Namespace and the direct App lifecycle edge. Execution never searches for a
declaration named `main`.

```ttx
lifecycle = Managed {
  run Some::File -> launch,
}
```

The declaration name has no lifecycle meaning. Managed and Unmanaged differ in
how the generated platform entry retains control around the call, not in the
kind of semantic edge they store. Their exact completion and teardown
distinction must be specified before either parser continuation is
implemented.

## Scene lifecycle

The Scene lifecycle retains one initial Scene and transition edges keyed by the
real producing Scene and signal identities:

```ttx
lifecycle = Scene {
  initial Scenes::Splash;
  on Scenes::Splash.finished replace Scenes::Title;
  on Scenes::Title.shift_pressed replace Scenes::Splash;
  on Scenes::Title.space_pressed exit;
}
```

`push` names a destination Scene, while `pop` has no destination:

```ttx
on Scenes::Game.pause_requested push Scenes::Pause;
on Scenes::Pause.closed pop;
```

Every transition is applied at the synchronization boundary after the active
Scene's `update` returns:

1. `replace` releases the active instance and prepares a fresh replacement.
2. `push` calls optional `pause`, retains the complete active instance, and
  prepares a fresh Scene above it.
3. `pop` releases the active instance and calls optional `resume` on the
  retained instance below it. An empty stack terminates the process.
4. `exit` releases every instance from top to bottom without resuming retained
  Scenes, then terminates the process.

App owns those transition choices. Scene owns state, signals, and lifecycle
Callables. The generated App lifecycle code owns the live Scene stack,
scheduling boundaries, and execution of the retained policy. Linked support
libraries provide the selected System and graphics capabilities.

The cyclic Splash to Title to Splash transition is a runtime state machine
cycle, not a source dependency cycle. All referenced Scene and signal owners
must exist before the App root can seal.

## Source and Graph handoff

The outer `Language::Source` owns source bytes, Tokens, Arena, and the App root
lifetime. App owns every App specific definition and resolution table required
by its policies. The future Environment Graph may retain completed App edges,
but it does not interpret `Windowed`, `Managed`, `replace`, or another App
policy.

App finalization requires:

1. exactly one startup profile;
2. exactly one lifecycle policy;
3. a Static target with the required signature for Managed or Unmanaged;
4. exactly one initial Scene for Scene lifecycle;
5. real producing Scene and signal identities for every transition;
6. a real destination Scene for every `replace` or `push`;
7. one edge for each producing Scene and signal pair; and
8. complete referenced owners before the App root seals.

The App root retains no runtime strings, source paths, token ranges, callable
names used as conventions, or parser transaction state.

## Status

No App parser, semantic App owner, generated entry planner, or lifecycle
executor is active. The canonical source is a human review pressure fixture.
Package membership and tokenization do not establish App semantics, native
startup, transitions, or execution.
