# TTX validation acceptance inputs

This tree owns parser and package host acceptance data. It is not an
application tree and does not establish semantic behavior merely by
tokenizing. `apps/ttx/echo` is the intended first Terminal application target.
`apps/ttx/scene_lifetime` remains the broader App and Scene pressure target.

The ownership matrix and oracles below record intended acceptance. They do not
claim that the corresponding parsers, package loader, compiler, or runtime
already exists.

## Dialect ownership matrix

| Dialect | Accepted owner boundary |
| --- | --- |
| Package | Package owns ordered exact `resolve` declarations, explicit semantic name to confined path Source bindings, an optional Package body, and selection of the sole completed App root. |
| Library | Library owns ordinary definitions, functions, initializers, executable Bodies, and explicit admission of embedded CPU Foreign. |
| Scene | Scene owns one implicit Scene object imported under its Package authored Source name, signals, members, lifecycle role declarations, executable Bodies, and explicit admission of embedded CPU Foreign. |
| App | App owns one implicit App root, its startup profile, direct Static or Scene lifecycle policy, generated platform entry semantics, and explicit admission of embedded CPU Foreign. |
| Foreign | Foreign is an embedded FFI block rather than a Source envelope or named Definition. |
| Render | Render owns separate render package authoring and host contracts outside Library implementation. |
| Shader | Shader owns separate GPU legality and lowering outside Library implementation and never acts as an implicit CPU Foreign host. |

The App Source body is implicit after `dialect : App;`. Future Package
construction selects exactly one completed App root. Neither `main.ttx` nor
the member's authored Source name selects the App.

## Canonical Source bindings

Package source membership is an explicit binding:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
source Main from "main.ttx";
```

The left side is the exact semantic name used for cross Source resolution. The
right side is the package root relative path opened by Package input. File and
directory names never derive, transform, or alias the semantic name.

Paths use `/` and remain subject to package confinement. Lexical `.` segments
may be normalized for opening, while absolute paths, `..` escapes, empty
segments, and opened objects outside the root are rejected before publication.
Path normalization has no effect on the authored semantic name.

`apps/ttx/scene_lifetime/main.ttx` is the implicit App body. It directly names
the two Scene identities above and owns the Splash to Title to Splash
transition cycle. It also selects the Windowed startup profile and embeds the
package root `$[resources/icon.png]`. The Scene Sources do not name one
another. They use typed `= new`, accepted deferred `:=` inference, explicit
`return Void;`, and the package root `$[resources/logo.png]`. App and Scene
parsing, startup generation, transitions, graphics, windowing, and execution
remain unimplemented and are not claimed by this fixture.

## Package Source confinement

`Environment::Workspace::import_source` is intended to consume the universal
envelope, select its installed Package Dialect, and retain the resulting
`Package::Language::Monograph`. The current interpreter does not yet complete
that transaction. A future confined package loader opens each Source path
relative to the package root, then imports its bytes under the Source local
name. It never resolves paths from the descriptor directory, current working
directory, or a source local directory. Future confined loader tests must
construct these independent failures:

| Case | Frozen result |
| --- | --- |
| `/absolute.ttx` | An absolute route is rejected. |
| `../outside.ttx` | Lexical escape is rejected before opening. |
| A package local symlink whose opened target is outside the root | The resolved outside root object is rejected without a check then read race. |
| `missing.ttx` | A missing member is rejected. |
| A route naming a directory or another non file | A non file member is rejected. |
| Two declarations of the same semantic source name | The second declaration is rejected without loading or publishing it. |

Content outside the package root is reachable only through an exact resolved
Package. Symlink cases are created inside a temporary test root so this
repository does not contain an intentionally escaping path.
`package/duplicate_source.ttx` still uses the retired anonymous Source spelling
and must be migrated before it can freeze duplicate name or path behavior.
`package/float_version.ttx` and `package/noncanonical_version.ttx` remain
independent invalid version inputs. Whether two different names may bind the
same normalized path remains an open Package policy decision rather than a
frozen oracle.

## Package resource oracle

`package_resources/package.ttx` still uses the retired anonymous Source
spelling and must be migrated before it becomes a current Package acceptance
input. Its two intended Library Sources request the same normalized resource
path `resources/table.bin`, so one future graph construction transaction must
read one stable backing snapshot while retaining two distinct Constant
bindings. `shared_a.ttx` also requests `resources/empty.bin`. Its zero byte
value is successful and distinct from read failure.

The first 64 bytes of `table.bin` are exactly:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/
```

Future evaluation of both Sources must fold only that `[0, 64]` slice. A
finalized Package and format `1` archive must retain the reachable 64 byte
values, not the complete input, package root, route, read cache, or unused
bytes. Missing, unreadable, non file, absolute, lexical escape, and opened
outside root resources must be source errors. Resource symlink failures must
also be created by future tests rather than checked in as escaping filesystem
state.

The unit test runfiles include `table.bin`, the genuinely empty `empty.bin`,
and the complete `scene_lifetime` source and resource target explicitly. Echo
does not yet have a Bazel source target and is not a unit test runfile. File
presence and the exact table prefix are structural fixture evidence only.

## Active and legacy data

`library/` owns the frozen Library and native oracle corpus. `package/` owns
descriptor failure inputs, and `package_resources/` owns resource success and
sharing inputs. Their anonymous Source membership fixtures require migration
to the explicit name to path grammar. Every TTX Source in those directories is
an explicit unit test runfile, but runfile inclusion alone does not make a
retired spelling current authority.

`shader_artifact/` is preserved as legacy Shader migration evidence. Its
type led `source Formats = ...`, `dialect : Gpu`, and `ShaderFormat`
spellings are not current authority, and that directory is deliberately
excluded from active unit test runfiles. Replacing it requires a separately
authorized Shader change.

## Closed contract blockers

Canonical signatures and empty returns still need one real Void Type owner.
Executable source still needs one Library owned executable representation, and
`object` still needs one Library owned managed object contract. Do not reopen
TTX or create shadow contracts to bypass those decisions.

TTX v1 is closed around its neutral Type, Layout, Addressable, and Callable
vocabulary. Library owns Generic materialization, concrete scalar identities,
Constants, evaluation semantics, and invocation refinements. Environment owns
the Workspace lifetime and retained Dialect Monographs, not another semantic
model.
