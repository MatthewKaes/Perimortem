# Standard Tetrodotoxin Packages

The standard Tetrodotoxin packages connect the language model to reusable math,
system, and graphics behavior. They are ordinary Package products, not compiler
builtins. A source receives one only by declaring a Package dependency, and a
source free consumer receives the same semantics by restoring its Archive.

Use these packages when an application needs the repository's runtime services
and wants those relationships to remain explicit in the semantic graph. A host
embedding Tetrodotoxin can publish different packages for the same roles. The
compiler does not acquire an implicit `System`, `Math`, or `Graphics` namespace.

The three packages deliberately keep different meanings in different Types.
A four component math vector, a color tone, and a Render Stage value may expose
similar Layouts without becoming the same Type. Each consuming language retains
the exact identity it selected.

## Perimortem.Math

`Perimortem.Math` supplies the concrete Library numeric and vector Types used by
the provided Render, Shader, and Graphics contracts. `Math::Vec4D`, for example,
is an inline Struct with named `x`, `y`, `z`, and `w` entries of exact
`Real_32` Type.

These Types are ordinary Library Structs. Their operations obey Library's exact
Type rules, so structural coincidence does not convert a Graphics point or tone
into a math vector. Render and Shader sources name the same Math identity when a
Stage is meant to exchange that value.

## Perimortem.System

`Perimortem.System` publishes CPU facing Library Types and Callables backed by
explicit Foreign declarations. Package native locators connect those declared
symbols to the Perimortem System runtime. Neither Library nor Workspace knows a
special System namespace.

### Terminal lines

`System::Terminal -> read_line()` returns one nonnull `System::Line` Object. It
owns the returned bytes and keeps their read only View stable for the Line
identity's lifetime. Its public observations are:

* `available` is true for a completed line, including an empty line
* `failed` is true for an input failure
* `value` is the exact `View[Unsigned_8]` without its line terminator

End of input has both flags false. A failure has `failed` true. These states
belong to the Terminal operation and do not introduce a universal Library
Option or Result Type.

Terminal publishes two `write_line` Callables. One writes a single byte View.
The other writes a prefix View followed by a value View. Both append exactly one
line terminator and report success as `Bool`. Keeping the Views separate allows
scatter and gather output without adding byte concatenation to Library's
numeric `+` operation.

The canonical Echo loop therefore remains ordinary Library control flow:

```ttx
const prefix : View[Unsigned_8] = "Echo: ":[0, 6];
const quit : View[Unsigned_8] = "quit":[0, 4];
const exit : View[Unsigned_8] = "exit":[0, 4];

while (true) {
  const line := System::Terminal -> read_line();
  if (line.failed or !line.available) {
    return;
  }
  if (line.value == quit or line.value == exit) {
    return;
  }
  const written := System::Terminal -> write_line(prefix, line.value);
  if (!written) {
    return;
  }
}
```

The native boundary uses an explicit carrier for line status, retained storage,
and byte Views. A C linkage symbol never exposes a C++ `Option`, allocator
object, or process address as its semantic contract.

### Process arguments

`System -> get_arguments()` returns one immutable `System::Arguments` Object
established before the Program entry Callable runs. Its `count` excludes the
platform executable name. `arguments -> at(.index = n)` returns the exact byte
View for an index below that count and the empty View at or above it.

The byte View excludes any platform terminator and remains stable for the
Arguments Object lifetime. An empty authored argument remains distinct from a
missing index because callers compare the index with `count` first. App does
not add these values to the Program entry Signature.

The native boundary carries the platform arguments through an explicit target
lifetime. The standard Package constructs the language Object and does not
turn an `argv` address into semantic identity.

### Input snapshots

`System -> get_input()` returns one immutable `System::Input` snapshot. The
production window loop and deterministic application driver both supply the
same value shape. The snapshot exposes exact `current`, `pressed`, and
`released` queries over stable `System::Key` identities.

`System::Key -> space()` and `System::Key -> shift()` return the identities used
by the Scene Lifetime sources. Focus loss, key repeat, and frame boundaries are
System policy. Scene observes the completed snapshot and does not receive a
platform event stream in its update Signature.

The native boundary carries one completed key snapshot in a target value shape.
Platform event objects and window system addresses never become `System::Input`
identity or Package reconstruction facts.

## Perimortem.Graphics

`Perimortem.Graphics` supplies the concrete Library Types used by the provided
Scene sources. They include `Image`, `Point2D`, `Size2D`, `Tone`, and `Sprite`.
These Types reuse `Perimortem.Math` where the semantic identity is genuinely a
math value and keep a distinct Graphics Type where point, size, color, or image
meaning matters.

`Point2D`, `Size2D`, and `Tone` are inline Struct values. Point coordinates and
Tone channels are `Real_64`. Pixel width and height are `Unsigned_32`. `Image`
and `Sprite` are nonnull Objects. An Image owns a stable decoded pixel result or
represents the authored empty image state. Backend textures and upload resources
are not part of that Object's semantic identity.

`Sprite` is a nonnull Object that proves the Tetrodotoxin Graphics hosting
contract. Its public mutable Fields are `image`, `size_pixels`, `position`,
`tone`, `visible`, and `z_index`. Their exact Types are `Image`, `Size2D`,
`Point2D`, `Tone`, `Bool`, and `Signed_64` in that order. Construction creates a
valid unconfigured Sprite with an empty Image, zero size and position, opaque
white tone, visible state, and zero draw index. It produces no draw until it has
drawable content. Those initial values are authored Sprite construction facts,
not implicit defaults of Object, Struct, or Image Types.

A Scene hosts a Sprite through the real private const Field that retains it.
The Scene mutates the Sprite's public Fields through ordinary Address access.
Graphics reads the same Object when it builds a frame submission. No standard
package creates a second node, Field table, or global Sprite registry.

Hosted Fields retain authored tree order. A higher `z_index` is in front, and a
later Field is in front when two indices match. Visibility and transform
compose from host to hosted value. These are Graphics submission rules rather
than extra Sprite identity or Scene declarations.

`Image -> decode(resource)` constructs one Image from retained Package bytes.
`image -> get_size_pixels()` returns the exact `Size2D` value used by Sprite.
Native image decoding is selected through the package's Foreign declarations
and native locators rather than hidden compiler knowledge. The native boundary
reports success and pixel lifetime explicitly. The Image Object remains the
language observation while target storage stays a runtime fact.

## Native and durable boundaries

Each standard package owns its Library source and reconstruction payload.
Package owns the Archive envelope and native locators. The relevant Perimortem
runtime component owns native behavior, and Linker owns the resulting object and
executable bytes.

A restored standard package constructs fresh semantic identities and reruns the
ordinary completion barriers. It reproduces public names, categories,
relationships, Layout behavior, and native locators without storing LLVM IR,
runtime handles, input snapshots, decoded images, or live Objects in the
Archive.

See [Package](../../tetrodotoxin/package/README.md) for dependency and Archive
selection, [Library](../../tetrodotoxin/library/README.md) for the concrete CPU
semantics, [Graphics](../../tetrodotoxin/graphics/README.md) for the hosting and
submission boundary, and [Linker](../../tetrodotoxin/linker/README.md) for
native product ownership.
