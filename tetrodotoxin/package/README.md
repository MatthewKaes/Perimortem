# Package

`tetrodotoxin/package` owns the authored Package Dialect, confined package
inputs, and durable Distribution representation.

Package depends only on Language, TTX, and Perimortem and provides a simple
manifest dialect for bootstrapping larger Tetrodotoxin workspaces.

## Dialect

Packages provide two lists:
* A collection of versioned package dependencies.
* A collection of sources to include.

Both dependencies and sources preserve source authored order, but dependencies
can be loaded in any given order.

Authored sources are rooted to the package directory. `..` is not supported so
package sources should be treated as using the `package.ttx` (or relevant source)
as the root folder for look ups.

### Language

The language is extremely straight forward and just contains a set of `resolve`
commands followed by a set of `source` commands:

```ttx
resolve Math : Perimortem.Math = "1.0";
resolve Graphics : Perimortem.Graphics = "1.0";
resolve Runtime : Perimortem.Runtime = "1.0";

source "scenes/splash.ttx";
source "scenes/title.ttx";
source "main.ttx";
```

Currently the language does not support documentation.

## Durable distribution

Currently Package only supports managing sources through a direct dialect. In
the future the goal is to have Package also support creating and consuming the
distribution format "precompiled" terminal artifacts.
