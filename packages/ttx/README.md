# Standard Tetrodotoxin Packages

The standard Tetrodotoxin packages provide reusable Math, System, and Graphics
features. They are ordinary Packages rather than hidden compiler built-ins. A
source declares them as dependencies, and an Archive can provide the same
public behavior when source is unavailable.

An application can use these packages for Perimortem's runtime services, while
another host can provide different Packages for the same roles. The compiler
does not create an implicit `System`, `Math`, or `Graphics` namespace.

Similar shapes do not erase meaning. A four-component math vector, a color
tone, and a Render Stage value may have matching Layouts while remaining
different Types.

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

`Perimortem.System` publishes CPU-facing Library Types and Callables backed by
explicit Foreign declarations. Package native locators connect those declared
symbols to the Perimortem System runtime. Neither Library nor Workspace knows a
special System namespace.

### Terminal lines

`System::Terminal -> read_line()` returns one nonnull `System::Line` Object. It
owns the returned bytes and keeps their read-only View stable for the Line
identity's lifetime. Its public observations are:

- `available` is true for a completed line, including an empty line
- `failed` is true for an input failure
- `value` is the `View[Unsigned_8]` without its line terminator

End of input leaves both flags false, while an input error sets `failed`. A
three-state Line is useful here because `Option[View[Unsigned_8]]` could not
distinguish the end of input from an error. This result belongs to Terminal
rather than introducing one universal error Type for unrelated systems.

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

The native boundary carries the line status, retained bytes, and Views
explicitly. Allocator objects and process addresses never become part of the
language contract.

### Process arguments

`System -> get_arguments()` returns one read-only `System::Arguments` Object
established before the Program entry Callable runs. Its `count` excludes the
platform executable name. `arguments -> at(.index = n)` returns the exact byte
View for an index below that count and the empty View at or above it.

The byte View excludes any platform terminator and remains stable for the
Arguments Object lifetime. An empty authored argument remains distinct from a
missing index because callers compare the index with `count` first. App does
not add these values to the Program entry Signature.

The native boundary keeps platform argument storage alive for the required
lifetime. The standard Package constructs the language Object instead of
treating a platform address as a language value.

### Input snapshots

`System -> get_input()` returns one read-only `System::Input` snapshot. The
production window loop and deterministic application driver both supply the
same value shape. The snapshot exposes exact `current`, `pressed`, and
`released` queries over stable `System::Key` identities.

`System::Key -> space()` and `System::Key -> shift()` return the identities used
by the Scene Lifetime sources. Focus loss, key repeat, and frame boundaries are
System policy. Scene observes the completed snapshot and does not receive a
platform event stream in its update Signature.

The native boundary turns platform events into one completed key snapshot.
Platform event objects and window-system addresses do not become part of
`System::Input` or its archived representation.

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

`Sprite` is a nonnull Object that supports Tetrodotoxin Graphics hosting. Its
public mutable Fields are `image`, `size_pixels`, `position`, `tone`, `visible`,
and `z_index`. Their Types are `Image`, `Size2D`,
`Point2D`, `Tone`, `Bool`, and `Signed_64` in that order. Construction creates a
valid unconfigured Sprite with an empty Image, zero size and position, opaque
white tone, visible state, and zero draw index. It produces no draw until it has
drawable content. Those authored Field initializers determine Sprite's semantic
default. Image, the inline Structs, and each scalar also retain their own total
Library defaults.

A Scene hosts a Sprite through the private state Field initialized with `new`.
The Scene changes the Sprite's public Fields through ordinary Library access,
and Graphics reads the same Object when it builds a frame. There is no second
node tree or global Sprite registry.

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

Each standard Package owns its Library source and the data needed to rebuild it.
Package owns the Archive, its Complete or Interface profile, and native artifact
locations. The matching Perimortem runtime component provides native behavior,
while Linker creates object files and executables for the chosen CPU and
operating system.

A Complete Archive rebuilds both public and private language objects. An
Interface Archive rebuilds the public contracts and compiled artifact locations
needed by other Packages without including executable bodies. Neither profile
stores LLVM IR, runtime handles, current input, decoded images, live Objects, or
source-level debugging data.

See [Package](../../tetrodotoxin/package/README.md) for dependency and Archive
selection, [Library](../../tetrodotoxin/library/README.md) for the concrete CPU
semantics, [Graphics](../../tetrodotoxin/graphics/README.md) for the hosting and
submission boundary, and [Linker](../../tetrodotoxin/linker/README.md) for
native product ownership.
