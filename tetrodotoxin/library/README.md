# Library

Library is Tetrodotoxin's reusable CPU language. It defines concrete scalar
Types, values, expressions, functions, Structs, Objects, Enumerations, and
Generic containers while retaining the shared TTX Type, Layout, Addressable,
and Callable contracts.

```ttx
// A reusable Library source.
dialect : Library;

public func twice[.value : Unsigned_64] -> Unsigned_64 {
  return value * 2;
}
```

## Names and access

Library uses punctuation to select separate semantic domains:

| Syntax                               | Meaning                                                       |
| ------------------------------------ | ------------------------------------------------------------- |
| `value.name`                         | select one Addressable from an applicable named Layout        |
| `context::Type`                      | traverse a Type-shaped route through an Abstract context      |
| `receiver -> callable(arguments...)` | select and invoke one Callable                                |
| `value.[names...]`                   | select and reorder named Layout entries                       |
| `access[index]`                      | try indexed reference access and return an optional reference |
| `value:[index]`                      | return an element value or its default                        |
| `value:[start, count]`               | return a safe read-only ranged value                          |

These domains never fall through to one another. A Field, Callable, and nested
Type may share a spelling because the operator already states which category is
being requested.

### Address access

`.` selects a real TTX Addressable from any applicable named Layout. It is not
limited to Struct or Object declarations; a named value flow may expose the same
kind of entry.

```ttx
packet.width
self.progress
foreign.external_counter
```

The selected Addressable identifies one semantic address and its Type. A
compiler may realize it as a stack location, an offset from an inline Struct,
an offset from an Object reference, or a folded value. Those choices do not
change the source-level selection.

Hosting grants access authority, not an implicit receiver. A hosted Function
still writes `self.field` or selects the Field through another explicit value;
a Static Function cannot read a host Field as a bare identifier.

### Type access

`::` follows contextual Type resolution:

```ttx
Graphics::Image
System::Terminal
Scene::Flow
```

Alias, Package, Monograph, Library source, and Type objects may all serve as
intermediate contexts. Only the result used in a Type position must prove Type;
the chain does not manufacture Type-valued Expressions for its intermediate
steps.

### Callable access

`->` is the Callable access and invocation operator:

```ttx
Packet -> create(width, height)
packet -> resize(width, height)
System::Terminal -> write_line(message)
```

A Callable is not an Addressable and never appears in a value Layout. Argument
and result compatibility are established through their real TTX Layouts.

## Layouts and value flow

A Layout describes the ordered values supplied or required by an expression,
declaration, Function, or Type. Library reuses TTX Layouts directly.

Function parameters and results may be positional or named:

```ttx
public func pair[Unsigned_64, Bool] -> [Unsigned_64, Bool]

public func classify[.value : Unsigned_64] -> [
  .accepted : Bool,
  .adjusted : Unsigned_64,
] {
  return (.accepted = value > 0, .adjusted = value + 1);
}
```

The leading `.accepted` and `.adjusted` spellings name Layout entries. They are
not postfix Address access because they have no receiver. A named value retains
its underlying expression and participates in fitting through that expression's
Type.

Receiving owners fit source Layouts directionally against the Layout they
require. Producing several values creates value flow, not an anonymous aggregate
Type.

Swizzle selects and reorders named entries:

```ttx
state dimensions : Fixed[Unsigned_64, 2] = packet.[width, height];
```

Plain brackets are reference access on `Access[T]`. They never substitute a
default address:

```ttx
access[index]                 // optional element reference
```

Colon bracket value access selects values. A missing element yields its Type
default. A ranged selection with a start outside the receiver yields the
default empty View, while a count beyond the remaining values stops at the
receiver boundary. Neither form preserves writable `Access` in its result:

```ttx
bytes:[4]
bytes:[4, 16]
```

The operands must still have integer Types. A value that cannot represent a
valid index or extent selects the same safe default; another operand Type is a
semantic error.

## Built-in Types

Library provides these scalar families:

- `Bool`
- `Signed_8`, `Signed_16`, `Signed_32`, and `Signed_64`
- `Unsigned_8`, `Unsigned_16`, `Unsigned_32`, and `Unsigned_64`
- `Real_32` and `Real_64`
- `Void`

Scalar operations require the exact resolved Type identity expected by that
operation. Library does not silently widen, narrow, retag, or reinterpret a
Constant to make an operation legal.

Generic Types describe contiguous element flow:

```ttx
Fixed[Unsigned_8, 64]
View[Unsigned_8]
Access[Unsigned_8]
```

`Fixed` has a compile-time element count. `View` is a borrowed contiguous view.
`Access` additionally carries the language's writable contiguous capability.
Materializing the same Generic with the same semantic arguments returns the
same Type identity.

## Source Structure

Each Library Monograph owns one synthetic `source` Structure with an empty
instance Layout. Top-level declarations enter its Static surface; instance
Fields cannot. The exact `source` route returns that Structure, while ordinary
Monograph lookup forwards only its externally visible Static entries.

Top-level Field declarations are Static Addressables owned by that source
Structure. They retain the ordinary Field exposure, writability, Type, and
initializer contracts, but never enter the source instance Layout. Root
Functions may resolve those exact identities as bare source names; private
Fields remain limited to the authenticated source context.

The Structure retains the exact Documentation that opens the Library source.
A Package member Alias can therefore route through `source` to one documented
root Type without copying the prose or becoming a Type itself.

A root Function is hosted by the source Structure but still retains its
Monograph as the source of diagnostics and imports. Hosting and source identity
are separate edges.

## Fields

A Field is a TTX Addressable owned by one Struct or Object. Its visibility and
writability are independent.

```ttx
public width : Unsigned_64 = 0;
private checksum : Unsigned_64 = 0;
public const signature : Unsigned_64 = 1;
private state updates : Unsigned_64 = 0;
expose state progress : Unsigned_64 = 0;
```

Visibility controls selection:

- `private` is visible only to code hosted by the containing Type.
- `public` is visible outside the containing Type.
- `expose state` makes state readable externally while retaining internal write
  authority.

Writability has three states:

- an ordinary Field is fully writable by callers that can select it;
- `state` is writable only by code hosted by the containing Type;
- `const` is writable only during initialization.

Every view exposes the same Field identity. Visibility does not create a public
copy, and writability does not change the underlying TTX Addressable.

A present initializer links through the Field in its containing Type's
authenticated context and must fit the declared Field Type. It remains one
exact Expression supplying one value rather than a general value Flow.

## Structs

`struct` declares an inline value Type:

```ttx
public Packet : struct {
  public width : Unsigned_64 = 0;
  public height : Unsigned_64 = 0;
  private checksum : Unsigned_64 = 0;

  public func area[self] -> Unsigned_64 {
    return self.width * self.height;
  }
}
```

The Struct's instance Layout is a named Layout over its real Fields in authored
order. Copying a Struct value copies its inline value semantics. Target offsets
and padding are derived later by the compiler.

The containing Type supplies complete access to its hosted Functions and an
external view to other callers. Nested Types, Callables, and Fields remain
separate query domains.

## Objects

`object` uses the same declaration and access model as `struct` while changing
value identity and lifetime:

```ttx
public Session : object {
  expose state progress : Unsigned_64 = 0;
  private state token : Unsigned_64 = 7;

  public func advance[self, .amount : Unsigned_64] -> Unsigned_64 {
    self.progress = self.progress + amount;
    return self.progress;
  }
}
```

An Object value is a nonnull managed reference identity. Assignment, parameter
passing, and return preserve that identity, so aliases observe the same
mutations. Object reuses Struct Fields, Functions, Layouts, visibility, and
writability rather than defining a parallel member model.

Library owns the lifetime semantics. Allocation strategy, pointer shape,
collector policy, and reclamation timing belong to the compiler and runtime.
Version 1 exposes no finalizer, weak reference, explicit release, or observable
reclamation order.

## Enumerations

An Enumeration selects an exact signed or unsigned storage Type and declares
named integer cases:

```ttx
public Mode : enum[Unsigned_8] {
  Idle = 0,
  Running = 1,
  Stopped = 2,
}
```

Each case has its own Alias and Constant identity. Two case names may carry the
same integer value without becoming the same semantic identity.

## Functions and invocation roles

A Function declares parameter and result Layouts followed by a body:

```ttx
public func add[
  .left : Unsigned_64,
  .right : Unsigned_64,
] -> Unsigned_64 {
  return left + right;
}
```

A Function without `self` is Static. Static means there is no implicit Self
value; source still selects it through a Type or source context:

```ttx
Math -> add(2, 3)
```

A Function whose parameter entry zero is the reserved `self` Addressable is
Self. That entry has the selected receiver's exact Type, and every following
parameter is named. The Function is selected through an addressable value:

```ttx
packet -> area()
```

Static and Self Callables may share a name because their receiver roles and
signatures distinguish the invocation. Both remain Callables reached only
through `->`; the real parameter Layout carries the role without a second
Callable category.

## Expressions and Constants

Library expressions retain authored value dependencies and resolve one result
Type. Constants cover Bytes, Bool, signed integers, unsigned integers, and real
values.

Arithmetic and comparison operate on exact compatible scalar Types. `and` and
`or` preserve short-circuit reachability. Unary `!` accepts Bool; unary `-`
accepts signed integer and real domains. Integer overflow and division by zero
are semantic failures in their owning operation. Safe `:[` selection
uses a default value instead of publishing a bounds failure.

Constant evaluation is ***nondestructive***. A compiler may fold a complete
expression, address selection, or indexed byte value, but the authored graph
and its real edges remain available to tools.

## Imports and resources

`using` imports the exposed Static surface selected through a Package context:

```ttx
using Core;
using Graphics::Utilities;
```

The route follows ordinary `::` contextual access. Package supplies its real
member contexts; Library imports eligible public declarations without creating
a second Package path model.

An embedded operand asks the exact source Package for retained bytes:

```ttx
public const signature : Fixed[Unsigned_8, 4] = 0x[54 54 58 31];
public const table := $[resources/table.bin];
public const header := $[resources/table.bin]:[0, 64];
```

Library interprets a successful Resource as a Bytes Constant. Package retains
path confinement and acquisition policy; Library never opens Package storage
directly.

A Library Monograph completes the exact closure of Library providers reached
through its authenticated imports. Every reachable declaration and Callable
signature settles before any Function body in that closure begins, so source
discovery order does not change the completed graph.

## Compilation boundary

Library lowering consumes completed CPU facts owned by Library, App, or Scene.
It derives target Layouts, calling convention carriers, registers, instructions,
and relocations without changing their semantic identities.

Linker owns object modules and final native products. Package Archive owns
durable semantic payloads. Runtime allocation and execution remain separate
from both.

See [TTX semantics](../../ttx/ttx_semantics.md) for the shared contracts and
[Package](../package/README.md) for `using` and resource contexts.
