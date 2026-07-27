# Scene Parser

`Tetrodotoxin::Scene::Parser` owns reusable Scene body grammar: managed state,
typed signals, lifecycle roles, and render roots.

It constructs one concrete Scene Abstract root in the Arena owned by
`Tetrodotoxin::Language::Source`. The Scene root owns its definitions,
resolution rules, and semantic facts. App owns transitions and the generated
lifecycle code that applies them. Graphics remains unaware of Scene.

## Source contract

`Language::Source::parse` consumes the leading Documentation and
`dialect : Scene;` envelope once, finds the future static
`Scene::Parser::parse` function in the borrowed toolchain map, and passes it the
same Cursor. The Scene parser consumes only the remaining body. The authored
shape includes:

1. Scene owned state Addressables;
2. signal declarations with complete payload Layouts;
3. required `prepare`, `update`, and `release` lifecycle roles;
4. optional `pause` and `resume` lifecycle roles;
5. a real Callable and CPU Body for each role; and
6. explicit render roots.

The lifecycle prefix records a direct role edge to an ordinary
`Library::Language::Callable` whose receiver is parameter zero. Generated
lifecycle code follows those edges and does not discover roles by searching
conventional names.

`prepare` and `release` bracket one live Scene instance. They are analogous to
entering and leaving a retained scene tree. `pause` and `resume` preserve that
same instance while controlling whether it is active:

```ttx
Scene pause[self] -> Void {
  return Void;
}

Scene resume[self] -> Void {
  return Void;
}
```

App `push` calls optional `pause` before retaining the current instance. App
`pop` releases the active instance and calls optional `resume` on the retained
instance below it. `replace` releases without pausing, and `exit` releases the
entire stack without resuming it. A missing optional role performs no
operation. A paused Scene receives no `update` calls.

Scene retains those Library Language Callables, their completed body facts, and
the lifecycle role edges. After Graph finalization, the Library compiler lowers the
selected CPU executable facts without converting the Scene into a Library
Source or moving its state, signals, roles, or render roots into Library.

A Scene update returns `Scene::Flow::stay` or emits one of that Scene's signals.
It never names another Scene and never decides process exit. App connects the
emitting Scene and signal to transition policy after all Scene owners exist.

Input is live retained state exposed by linked `Perimortem.System` support
rather than a Scene parameter aggregate. The future Environment Graph retains
the selected System query identity, never the mutable singleton state. Update
receives one explicit nonreceiver parameter:

```ttx
Scene update[self, .delta_time : Real_64] -> Scene::Flow
```

Delta time is invocation specific scheduler input. Keeping it explicit is
deterministic and allows Headless execution without a Graphics dependency.
Putting scheduling time on Graphics would couple lifecycle execution to a
presentation subsystem.

Embedded resources use the package resource contract. The outer
`Language::Source` owns source bytes, Tokens, Arena, and the Scene root
lifetime. The package construction owner supplies a confined resource snapshot
through `Package::Workspace`. The Scene parser does not open files.

## Canonical pressure fixture

[`../../../apps/ttx/scene_lifetime/scenes/splash.ttx`](../../../apps/ttx/scene_lifetime/scenes/splash.ttx)
and
[`../../../apps/ttx/scene_lifetime/scenes/title.ttx`](../../../apps/ttx/scene_lifetime/scenes/title.ttx)
exercise resource loading, state, required lifecycle roles, typed input,
signals, and a cycle connected by the App owner. They do not yet exercise the
optional pause and resume roles.

## Status

No Scene parser, evaluator, lifecycle executor, System input owner, or durable
Scene schema is implemented. Tokenization and Package membership do not
establish those behaviors. The canonical Scene bodies still use the rejected
`Runtime::Frame`; they must move to scalar delta time and a real System input
query before semantic acceptance.
