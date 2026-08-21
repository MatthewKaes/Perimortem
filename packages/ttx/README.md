# Standard Tetrodotoxin Packages

The standard Tetrodotoxin packages provide reusable Memory, Math, System, and
Graphics features. They are ordinary Packages rather than hidden compiler
built-ins. A source declares them as dependencies, and an Archive can provide
the same public behavior when source is unavailable.

An application can use these packages for Perimortem's runtime services, while
another host can provide different Packages for the same roles. The compiler
does not create an implicit `Memory`, `System`, `Math`, or `Graphics` namespace.

Similar shapes do not erase meaning. A four-component math vector, a color
tone, and a Render Stage value may have matching Layouts while remaining
different Types.

## Perimortem.Memory

`Perimortem.Memory` publishes the exact `Dynamic::Bytes` identity shared by
Packages that exchange owned byte values. A dependent Package declares Memory
in its manifest and uses its context explicitly:

```ttx
resolve Memory : Perimortem.Memory = "1.0";
```

```ttx
using Memory;
```

The Memory source authors the two-word byte carrier as one
`Object[Unsigned_8]` plus its logical size. Ordinary recursive Structure
ownership retains and releases that Object without native lifecycle Attributes.
Consumers therefore share one Type identity rather than materializing matching
but unrelated byte carriers in every source root.

Bytes transformations use a referenced Self receiver. `copy(view)` creates an
owned value. `append(byte, count)`, receiver `concat`, `resize`, `shrink`,
`clear`, and `reserve` mutate that receiver and return `self`, so calls may be
chained without copying the Bytes value. Parameter defaults are not yet part of
the Function signature model, so callers pass `1` for a single-byte append:

```ttx
line -> concat(suffix) -> append(byte, 1);
buffer -> clear();
```

`self` is passed by reference, and the scalar result spelling `-> self` returns
that same reference. Reaching the end of such a Function returns `self`
implicitly; an explicit `return self;` remains available for early exit. Before
a buffer write, Bytes reserves the required size. Growth already supplies a
private Object buffer; otherwise `is_shared()` causes an explicit `clone()`
before writable access. Object itself remains an ordinary shared buffer rather
than owning copy-on-write policy.

Static and Self `concat` share one spelling because receiver role is part of
the Callable signature. Static `concat(left, right)` creates an owned value,
while receiver `concat(view)` extends a value. `clear` preserves capacity;
ordinary default construction creates the empty zero-capacity reset value.
`get_size`, `get_capacity`, `get_view`, `slice`, and `is_empty` inspect the
result without changing it.

The package deliberately exposes no writable Access to the backing capacity:
that would bypass the logical size owned by Bytes. Safe element reads remain
available through `get_view():[index]`. It also has no forgetful resize that
would expose invalid elements and no host-specific hash operation without a
Library hash contract.

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
special System namespace. System depends on `Perimortem.Memory`, so its Terminal
Callables and their callers exchange the same `Dynamic::Bytes` identity.

The build selects `Perimortem.System.Host` as the logical provider for the
current target. Package records that choice beside each imported symbol in its
Archive artifact and Linker ABI Manifest, while the Foreign graph remains the
same across host implementations.

### Terminal lines

`System::Terminal -> read_line()` returns `Option[Dynamic::Bytes]`. A selected
value owns the complete line without its terminator. Immediate end of input or
a read failure is absent. The caller may borrow a View from the owned bytes and
uses ordinary Option propagation when either condition ends its current flow.

`System::Terminal -> write_line(line)` borrows one `Dynamic::Bytes`, appends one
line terminator, flushes the terminal, and returns its completion as `Bool`.
`Dynamic::Bytes -> concat(left, right)` copies two byte Views into one owned
value entirely through authored Memory Package behavior.

The canonical Echo loop therefore remains ordinary Library control flow:

```ttx
const prefix : View[Unsigned_8] = "Echo: ":[0, 6];
const quit : View[Unsigned_8] = "quit":[0, 4];
const exit : View[Unsigned_8] = "exit":[0, 4];

while true {
  state line := System::Terminal -> read_line()?;
  state view := line -> get_view();
  if view == quit or view == exit : return;

  state output := Dynamic::Bytes -> copy(prefix);
  output = output -> concat(view);
  System::Terminal -> write_line(output)?;
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
the selected language compiler creates member objects, and the platform build
toolchain creates native archives and executables.

A Complete Archive rebuilds both public and private language objects. An
Interface Archive rebuilds the public contracts and compiled artifact locations
needed by other Packages without including executable bodies. Neither profile
stores LLVM IR, runtime handles, current input, decoded images, live Objects, or
source-level debugging data.

See [Package](../../tetrodotoxin/package/README.md) for dependency and Archive
selection, [Library](../../tetrodotoxin/library/README.md) for the concrete CPU
semantics, [Graphics](../../tetrodotoxin/graphics/README.md) for the hosting and
submission boundary, and [Puffer](../../puffer/README.md) for command and
product coordination.
