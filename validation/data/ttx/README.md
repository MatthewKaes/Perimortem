# TTX validation acceptance inputs

This tree owns parser and package host acceptance data. It is not an
application tree and does not establish semantic behavior merely by
tokenizing. `apps/ttx/scene_lifetime` remains the real App and Scene pressure
fixture.

## Dialect ownership matrix

| Dialect | Accepted owner boundary |
| --- | --- |
| Package | Ordered exact `resolve` declarations, confined `source` declarations, optional Package body, and selection of the sole completed App root |
| Library | Ordinary definitions, functions, initializers, executable Bodies, and explicit opt-in to embedded CPU Foreign |
| Scene | One path-derived implicit Scene object, signals, members, lifecycle-role declarations, executable Bodies, and explicit opt-in to embedded CPU Foreign |
| App | One implicit App root, startup profile, direct Static or Scene lifecycle policy, generated platform entry semantics, and explicit opt in to embedded CPU Foreign |
| Foreign | An embedded FFI block only; never a Source envelope or named Definition |
| Render | Separate render-package authoring and host contracts; out of scope for the Library plan |
| Shader | Separate GPU legality and lowering; out of scope for the Library plan and never an implicit CPU Foreign host |

The App Source body is implicit after `dialect : App;`. Package construction
selects exactly one completed App root, and `main.ttx` is only a filename
convention.

## Canonical Scene identities

The confined normalized member route owns a Scene's identity. The accepted
fixture freezes these exact mappings:

```text
scenes/splash.ttx -> Scenes::Splash
scenes/title.ttx  -> Scenes::Title
```

Routes are package-relative, use `/`, have an exact lowercase `.ttx`
extension, and use lowercase snake-case path segments. Lexical `.` segments
are removed before identity derivation. Absolute routes, `..` escapes, empty
segments, noncanonical case, and a segment that is not lowercase snake case
fail before Scene publication. Each route segment becomes one PascalCase
identity segment. Repeating a normalized route or producing an identity
already owned by another route is a package error; no first-wins behavior,
case folding, or filename-derived alias is accepted.

`apps/ttx/scene_lifetime/main.ttx` is the implicit App body. It directly names
the two Scene identities above and owns the Splash to Title to Splash
transition cycle. It also selects the Windowed startup profile and embeds the
package root `$[resources/icon.png]`. The Scene Sources do not name one
another. They use typed `= new`, accepted deferred `:=` inference, explicit
`return Void;`, and the package root `$[resources/logo.png]`. App and Scene
parsing, startup generation, transitions, graphics, windowing, and execution
remain unimplemented and are not claimed by this fixture.

## Package Source confinement

`source "route.ttx";` is consumed by the owning
`Language::Source::parse` transaction and static
`Package::Language::Parser::parse` function into
`Package::Language::Source`.
`Package::Workspace` resolves the route relative to the package root, not the
descriptor directory, current working directory, or a Source local directory.
Package tests construct these
independent failures:

| Case | Frozen result |
| --- | --- |
| `/absolute.ttx` | Reject an absolute route. |
| `../outside.ttx` | Reject lexical escape before opening. |
| A package-local symlink whose opened target is outside the root | Reject the resolved-outside-root object without a check-then-read race. |
| `missing.ttx` | Reject a missing member. |
| A route naming a directory or another non-file | Reject a non-file member. |
| Two declarations of the same normalized route | Reject the second declaration without loading or publishing it. |

Content outside the package root is reachable only through an exact resolved
Package. Symlink cases are created inside a temporary test root so this
repository does not contain an intentionally escaping path.
`package/duplicate_source.ttx` freezes the duplicate-route spelling.
`package/float_version.ttx` and `package/noncanonical_version.ttx` remain
independent invalid version inputs. The obsolete
`source Types : Library = "types.ttx";` fixture was removed because named or
type-led Source membership is not current Package grammar.

## Package resource oracle

`package_resources/package.ttx` loads two Library Sources in authored order.
Both request the normalized route `resources/table.bin`, so one future graph
construction transaction must read one stable backing snapshot while retaining
two distinct Constant bindings. `shared_a.ttx` also requests
`resources/empty.bin`; its zero byte value is successful and distinct from read
failure.

The first 64 bytes of `table.bin` are exactly:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/
```

Both Sources fold only that `[0, 64]` slice. A finalized Package and format-1
archive retain the reachable 64-byte values, not the complete input, package
root, route, read cache, or unused bytes. Missing, unreadable, non-file,
absolute, lexical-escape, and opened-outside-root resources are source errors.
Resource symlink failures are also created by future tests rather than checked
in as escaping filesystem state.

The unit-test runfiles include `table.bin`, the genuinely empty `empty.bin`,
and the complete `scene_lifetime` source/resource target explicitly. Their
presence and exact table prefix are structural fixture evidence only.

## Active and legacy data

`library/` owns the frozen Library and native-oracle corpus. `package/` owns
descriptor failures that are current Package grammar. `package_resources/`
owns resource success and sharing inputs. Every active TTX Source in those
directories is an explicit unit-test runfile.

`shader_artifact/` is preserved as legacy Shader migration evidence. Its
type-led `source Formats = ...`, `dialect : Gpu`, and `ShaderFormat`
spellings are not current authority, and that directory is deliberately
excluded from active unit-test runfiles. Replacing it belongs to a separately
authorized Shader slice.

## Closed contract blockers

Canonical signatures and empty returns still need one real Void Type owner.
Executable source still needs one Body owner, and `object` still needs one
Managed Type owner. None has an approved live owner at this checkpoint. Do not
reopen TTX or create Tetrodotoxin shadow contracts to bypass those decisions.
Parser, archive, and Library compiler work that requires them remains blocked
until their concrete Dialect ownership is approved. Type, Layout, Generic,
Constant, Addressable, Callable, Namespace, and Workspace now belong to
`Tetrodotoxin::Library::Language`; they are not candidates for reopening TTX.
