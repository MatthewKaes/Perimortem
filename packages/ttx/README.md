# Standard Tetrodotoxin Packages

These Packages give a Tetrodotoxin program the everyday building blocks it
needs to own data, perform math, talk to its host, and submit graphics. They make
Perimortem runtime services feel like ordinary authored dependencies rather
than secret compiler features.

A source chooses the Packages it wants with local Alias imports and can use the
same public identities whether they came from source or a durable Archive.
Another host is free to provide different Packages for the same roles. That
keeps the platform useful beyond one runtime without making `Memory`, `Math`,
`System`, or `Graphics` appear by magic.

Similar shapes do not erase meaning. A four component math vector, a color
tone, and a Render Stage value may have matching Layouts while remaining
different Types.

## Perimortem.Memory

`Perimortem.Memory` publishes the exact `Dynamic::Bytes` identity shared by
Packages that exchange owned byte values. A dependent source imports Memory and
uses its context explicitly:

```ttx
public Memory : alias =
    package(.name = "Perimortem.Memory", .version = "1.0");
```

```ttx
using Memory;
```

The Memory source authors the two word byte carrier as one
`Object[U8]` plus its logical size. Ordinary recursive Structure
ownership retains and releases that Object without native lifecycle Attributes.
Consumers therefore share one Type identity rather than materializing matching
but unrelated byte carriers in every source root.

Native C++ consumers include `perimortem/memory/dynamic/bytes.hpp` and use the
same `Perimortem::Memory::Dynamic::Bytes` route. That class is generated from
this TTX declaration, and its compiled implementation crosses the raw C ABI on
the consumer's behalf.

Bytes transformations use a referenced Self receiver. `copy(view)` creates an
owned value, while `with_capacity(count)` prepares an empty value for later
writes. `append(byte, count)`, receiver `concat`, `resize`,
`forgetful_resize`, `shrink`, `clear`, `proxy`, `set`, `convert`, `reset`, and
`reserve` mutate that receiver and return `self`, so calls may be chained
without copying the Bytes value. Parameter defaults are not yet part of the
Function signature model, so TTX callers pass `1` for a single byte append:

```ttx
line -> concat(suffix) -> append(byte, 1);
buffer -> clear();
```

`self` is passed by reference, and the scalar result spelling `-> self` returns
that same reference. Reaching the end of such a Function returns `self`
implicitly. An explicit `return self;` remains available for early exit. Before
a buffer write, Bytes reserves the required size. Growth already supplies a
private Object buffer. Otherwise `is_shared()` causes an explicit `clone()`
before writable access. `detach()` makes that step explicit for the native C++
facade. Object itself remains an ordinary shared buffer rather than owning copy
on write policy.

Static and Self `concat` share one spelling because receiver role is part of
the Callable signature. Static `concat(left, right)` creates an owned value,
while receiver `concat(view)` extends a value. `clear` preserves capacity, and
`reset` returns to the empty zero capacity value. `get_size`, `get_capacity`,
`get_view`, `at`, `slice`, and `is_empty` inspect the result without changing
it.

The generated C++ facade follows the familiar Perimortem value API. Static one
input factories provide converting constructors and assignments. `get_view`
provides read only conversion, while `detach` and the logical size provide a
copy on write `Core::Access::Bytes`. C++ also derives indexing, equality,
hashing, single byte append, and `ensure_capacity` from those same public facts
without adding another TTX Callable inventory.

## Perimortem.Math

`Perimortem.Math` supplies the concrete Library numeric and vector Types used by
the provided Render, Shader, and Graphics contracts. `Math::Vec4D`, for example,
is an inline Struct with named `x`, `y`, `z`, and `w` entries of exact
`R32` Type.

These Types are ordinary Library Structs. Their operations obey Library's exact
Type rules, so structural coincidence does not convert a Graphics point or tone
into a math vector. Render and Shader sources name the same Math identity when a
Stage is meant to exchange that value.

## Perimortem.System

`Perimortem.System` publishes CPU facing Library Types and Callables backed by
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
const prefix : View[U8] = "Echo: ":[0, 6];
const quit : View[U8] = "quit":[0, 4];
const exit : View[U8] = "exit":[0, 4];

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

`System -> get_arguments()` returns one read only `System::Arguments` Object
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

`System::Input -> snapshot()` returns one immutable input snapshot. The
production window loop and deterministic application driver both supply the
same value shape. The snapshot exposes exact `current`, `pressed`, and
`released` queries over stable `System::Key` identities. Its exposed `pointer`,
`pointer_delta`, and `scroll` values use the shared `Math::Vec2D` Type, while
`pointer_active` reports whether the pointer currently belongs to the surface.

`System::Key` publishes stable values such as `space` and `shift`, which can be
imported through ordinary `using` resolution. Focus loss, key repeat, and frame
boundaries are System policy. Scene observes the completed snapshot and does
not receive a platform event stream in its update Signature.

The native boundary turns platform events into one completed key snapshot.
Platform event objects and window system addresses do not become part of
`System::Input` or its archived representation.

## Perimortem.Graphics

`Perimortem.Graphics` supplies the concrete Library Types used by the provided
Scene sources. Pixel, Point2D, Size2D, Tone, Transform2D, Placement2D, Image,
Texture2D, Sprite, Render contracts, and Shader Programs are real Package
members. Routes such as `Graphics::Pixel`, `Graphics::Sprite`, and
`Graphics::Shader::DefaultTexturedQuad2D::Instance` therefore select their
documented identities directly.

`Placement2D` is the ordinary Library Structure that describes the public
transform, visibility, and ordering state promised by a placed Object. It is
not a base class or allocated node. The Graphics Interface negotiates a
concrete Object against this real requirement, which lets Sprite retain its
exact identity and additional behavior. Runtime child traversal is a separate
Children2D Interface and is not implied by placement.

`Transform2D` carries translation, scale, and rotation as domain values. The
runtime copies their composed affine result into each stable frame submission,
so later Scene mutations cannot change a frame already being presented.

`Point2D`, `Size2D`, `Tone`, and `Image` are inline Struct values. Point
coordinates and Tone channels are `R64`. Size2D width and height are `U32`.
Image retains one shared `Object[Pixel]` buffer, its logical pixel count, its
dimensions, and its addressing policy. Its ordinary default is the empty image
value. Texture2D gives an Image stable rendering identity while the backend
keeps uploads and device images as independent runtime facts.

`Pixel` is the four byte RGBA value shared by decoded Images and native codecs.
Its transparent black default follows ordinary Structure construction.
`from_grey`, `from_grey_alpha`, `from_rgb`, and `from_rgba` make every other
construction explicit without relying on overloaded native constructors.

`Sprite` is a nonnull Object with public mutable `texture`, `shader`,
`size_pixels`, `transform`, `visible`, and `z_index` Fields. The shader Field is
`Implementation[Render::TexturedQuad2D]`. It retains one real Shader Instance
Object and the ABI Projection that selects its generated Program, Parameters
byte range, and ordered material resources. A Shader can therefore add a
Texture2D resource to its own Instance without adding an application specific
Field to Sprite. Construction creates a valid unconfigured Sprite with an empty
Shader implementation, zero size, identity transform, visible state, and zero
draw index. It produces no draw until its texture, Shader, and size are
configured. The transform Field satisfies Placement2D directly and avoids a
second position authority.

A Scene hosts a Sprite through the private state Field initialized with `new`.
The Scene changes the Sprite's public Fields through ordinary Library access,
and Graphics reads the same Object when it builds a frame. There is no second
node tree or global Sprite registry.

Scene Fields retain authored tree order. A higher `z_index` is in front, and a
later Field is in front when two indices match. Visibility and transform
compose from host to hosted value. These are Graphics submission rules rather
than extra Sprite identity or Scene declarations.

`Format::PNG -> decode(.bytes = resource)` returns an optional Image from
retained Package bytes. The format owns decoding while Image remains the shared
decoded value that another codec can produce as well. Absence reports malformed
or unsupported input without turning the valid empty Image default into an
error state.
The exposed `image.size` is the exact read only `Size2D` value used by Sprite,
while public `image.addressing` selects zero, clamp, or wrap sampling directly.
Native PNG decoding is selected through the package's Foreign declarations
and native locators rather than hidden compiler knowledge. The native boundary
returns an optional Image value and transfers one reservation for its pixel
buffer. Image sampling is ordinary Library behavior on that value, while a
Shader Terminal recognizes the same authored operation as a target image
sample. Target storage remains a separate runtime fact.

The Package also publishes the target neutral TexturedQuad2D Render contract
and the standard DefaultTexturedQuad2D Shader. Render owns the fixed resource,
host inputs, Stage signatures, vertex layout, topology, blend policy, geometry,
and vertex count. DefaultTexturedQuad2D owns its Vec4D tone uniform. An
application can publish another Shader Program against the same Graphics Render
contract without adding that application policy to the standard Package.

Package production compiles every Program into its own embedded SPIR V module
and native Shader child product, including Programs authored by an application
Package. Application production discovers every Program reached by Scene
Instance Fields and generates one Vulkan description for each module locator.
The application target supplies independent Placement2D, Children2D, and
Drawable2D providers for configured runtime Types. The Scene graph retains only
real Objects and semantic Interface proofs.

## Native and durable boundaries

Each standard Package owns its Library source and the data needed to rebuild it.
Package owns the Archive, its Complete or Contract profile, and native artifact
locations. The matching Perimortem runtime component provides native behavior,
the selected language compiler creates member objects, and the platform build
toolchain creates native archives and executables.

A Complete Archive rebuilds both public and private language objects. An
Contract Archive rebuilds the public contracts and compiled artifact locations
needed by other Packages without including executable bodies. Neither profile
stores LLVM IR, runtime handles, current input, decoded images, live Objects, or
source level debugging data.

See [Package](../../tetrodotoxin/package/README.md) for dependency and Archive
selection, [Library](../../tetrodotoxin/library/README.md) for the concrete CPU
semantics, [Graphics](../../tetrodotoxin/graphics/README.md) for the hosting and
submission boundary, and [Puffer](../../puffer/README.md) for command and
product coordination.
