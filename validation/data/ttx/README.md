# TTX fixture data

This directory contains authored TTX inputs, binary resources, and exact process
observations used by validation tests. Each test chooses the files relevant to
its own contract. The directory as a whole is a source corpus rather than one
program.

## Directory map

| Path | Contents |
| --- | --- |
| [`library/`](library/) | Library source examples, focused rejection inputs, Foreign declarations, and a native C harness |
| [`package/`](package/) | Focused invalid Package manifests |
| [`package_resources/`](package_resources/) | A Package with two Library members that share retained resource input |
| [`products/`](products/) | Complete Package fixtures for native Library and Foreign integration |
| [`oracles/`](oracles/) | Exact Scene lifecycle observations |
| [`shader_artifact/`](shader_artifact/) | One Render contract and Shader Program compiled into an embedded GPU module |

## Access syntax in fixtures

The source files use three independent access domains:

| Syntax | Meaning |
| --- | --- |
| `value.name` | select one Addressable from an applicable named Layout |
| `context::Type` | select a Type through contextual resolution |
| `receiver -> callable(arguments)` | select and invoke one Callable |

For example, `self.elapsed` selects state, `Scene::Flow` selects a Type, and
`Scene::Flow -> stay()` invokes a Callable. A Callable is never selected with
`.`. Named entries such as `.color` inside a parameter or result Layout have no
receiver and are not postfix Address access.

## Package fixtures

The focused manifests in [`package/`](package/) each preserve one invalid input:

| File | Source condition |
| --- | --- |
| [`duplicate_semantic_name.ttx`](package/duplicate_semantic_name.ttx) | two Sources use the same semantic name |
| [`duplicate_normalized_path.ttx`](package/duplicate_normalized_path.ttx) | `./member.ttx` and `member.ttx` normalize to one logical path |
| [`float_version.ttx`](package/float_version.ttx) | a Package version is written as an unquoted real literal |
| [`noncanonical_version.ttx`](package/noncanonical_version.ttx) | a quoted Package version uses a leading zero |

Package Source declarations keep semantic identity separate from path:

```ttx
source Scenes::Splash from "scenes/splash.ttx";
```

`Scenes::Splash` is queried through contextual `::` access. The quoted path is a
confined Package root location and does not derive semantic identity.

## Resource fixtures

[`package_resources/package.ttx`](package_resources/package.ttx) binds
`SharedA` and `SharedB`. Both Library sources read
`resources/table.bin` and select its first 64 bytes. `SharedA` also reads the
zero byte `resources/empty.bin`.

The first 64 bytes of `table.bin` are:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/
```

These files distinguish several resource facts:

* equivalent normalized routes can share one retained input snapshot
* separate Library declarations retain separate Constant identities
* empty content is a successful Resource
* indexed value access selects reachable bytes without changing the Package
  path.

## Process observations

Unit validation runs a reserved self process fixture to prove exact standard
input, standard output, standard error, exit status, and timeout observation.
That fixture validates the process observer itself. It is not evidence for a
generated TTX Terminal.

[`scene_lifetime.golden`](oracles/scene_lifetime.golden) records deterministic
Scene clock steps, hosted state and submission order, signals, releases,
transition construction, and final process exit. Stable instance ordinals
distinguish fresh Scene construction without using process addresses. It does
not make managed Object reclamation observable.

## Shader source sample

[`shader_artifact/shader.ttx`](shader_artifact/shader.ttx) demonstrates named
Stage Layouts and `Formats::Simple` contextual Type access. Its companion
[`render.ttx`](shader_artifact/render.ttx) owns the matching target neutral
contract. Package validation compiles that Program through the SPIR V Terminal,
links the resulting words as read only native data, and validates the linked
bytes independently. The canonical Render model is documented in
[Tetrodotoxin Render](../../../tetrodotoxin/render/README.md).

See the [Library fixture reference](library/README.md),
[Package language](../../../tetrodotoxin/package/README.md), and
[TTX semantics](../../../ttx/ttx_semantics.md) for the corresponding contracts.
