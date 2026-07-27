# Package

`tetrodotoxin/package` owns the authored Package Dialect. The current target
contains the concrete Dialect scaffold, exact Dependency request value, exact
Source binding value, and Package Monograph model.

Package depends on Language, TTX, and Perimortem. Environment is intended to
install `Package::Dialect` and retain each interpreted Package Monograph.

## Authored grammar

A Package body contains ordered dependency requests followed by ordered Source
bindings:

```ttx
resolve Math : Perimortem.Math = "1.0";
resolve Graphics : Perimortem.Graphics = "1.0";
resolve System : Perimortem.System = "1.0";

source Scenes::Splash from "scenes/splash.ttx";
source Scenes::Title from "scenes/title.ttx";
source Main from "main.ttx";
```

Each `resolve` declaration records the local Type shaped name used by this
package, the exact external package name, and the pinned version.

`Package::Language::Dependency` is that request, not the fetched package or a
semantic resolution.

After the dependency region, each `source` declaration binds an exact authored
Type shaped semantic name to one package path. The left side is the name used
for cross Source resolution. The right side is only the location opened beneath
the package root. A filename never creates a semantic name implicitly.

`Package::Language::Source` retains that exact pair, and the Package Monograph
retains the Source values in authored order. Package interpretation does not
open their paths.

## Dialect and Monograph

`Package::Dialect` derives from `Language::Dialect`. Environment selects its
stateful instance after parsing `dialect : Package;` and passes the same forward
Cursor, opening Documentation, graph Arena, and shared registry to
`interpret`.

The interpretation contract constructs one
`Package::Language::Monograph` in the Environment Arena after a successful
transaction. The Monograph retains its opening Documentation, its host Package
Dialect, ordered exact Dependency requests, and ordered Source bindings.

It retains no filesystem handle, downloaded dependency, opened member
Monograph, compiler product, or archive entry.

The current parser path is incomplete and does not yet produce a successful
Package Monograph. These are the result invariants it must establish.

## Package root policy

The intended package contract confines every source and embedded resource to
one opened package root. Future Package input opens a Source path beneath that
root, then imports its bytes into Environment under the Source local name.
Absolute paths and `..` escapes are invalid. Content outside that root is
available only through an exact resolved Dependency.

That filesystem capability is not implemented in the current Package target.
The current Monograph model retains authored Source bindings only. No current
Package path proves confinement, loads member contents, imports them under
their local names, deduplicates resources, or selects an App.

`main.ttx` is only a filename convention. Future package assembly selects the
sole completed App Monograph regardless of its local Source name or member
filename.

## Future durable products

Package will eventually own generated Distribution values and their Reader and
Writer. The prototype archive format remains `1` and has no backwards
compatibility requirement before release.

No Distribution, Manifest, Entry, Reader, Writer, archive codec, repository
client, or source free restoration owner exists in the current Package target.
A Package Monograph proves only that the authored manifest was interpreted.
