# Library

Library is Tetrodotoxin's reusable CPU language. It defines concrete scalar
Types, values, expressions, functions, Structs, Objects, Enumerations, and
Generic containers while retaining the shared TTX Type, Layout, Addressable,
and Callable contracts.

Use Library when reusable CPU logic must share exact semantic identities with
packages, applications, scenes, tools, and native compilation. A conventional
language frontend is a simpler fit when one language owns the whole program or
compatibility with an established language and tooling ecosystem matters more
than cross language composition.

Canonical grammar reference: [Library.g4](grammar/Library.g4).

```ttx
// A reusable Library source.
dialect : Library;

public twice : func = [.value : Unsigned_64] -> Unsigned_64 {
  return value * 2;
}
```

## Names and access

Library uses punctuation to select separate semantic domains:

| Syntax                               | Meaning                                                       |
| ------------------------------------ | ------------------------------------------------------------- |
| `value.name`                         | select one Addressable from an applicable named Layout        |
| `context::Type`                      | traverse an Abstract context to one Type                      |
| `receiver -> callable(arguments...)` | select and invoke one Callable                                |
| `value.[names...]`                   | select and reorder named Layout entries                       |
| `access[index]`                      | try indexed reference access and return an optional reference |
| `value:[index]`                      | return an element value or its default                        |
| `value:[start, count]`               | return a safe read only ranged value                          |

These domains never fall through to one another. A Field, Callable, and nested
Type may share a spelling because the operator already states which category is
being requested.

### Address access

`.` selects one exact TTX Addressable from any applicable named Layout. It is
not limited to Struct or Object declarations. A named value flow may expose the
same kind of entry.

```ttx
packet.width
self.progress
foreign.external_counter
```

The selected Addressable identifies one semantic address and its Type. A
compiler may realize it as a stack location, an offset from an inline Struct,
an offset from an Object reference, or a folded value. Those choices do not
change the source level selection.

Hosting grants access authority, not an implicit receiver. A hosted Function
still writes `self.field` or selects the Field through another explicit value.
A Static Function cannot read a host Field as a bare identifier.

### Type access

`::` follows contextual Type resolution:

```ttx
Graphics::Image
System::Terminal
Scene::Flow
```

Alias, Package, Monograph, Library source, and Type objects may all serve as
intermediate contexts. Only the result used in a Type position must prove Type.
The chain does not manufacture Type valued Expressions for its intermediate
steps.

### Callable access

`->` is the Callable access and invocation operator:

```ttx
Packet -> create(width, height)
packet -> resize(width, height)
System::Terminal -> write_line(message)
```

A Callable is not an Addressable and never appears in a value Layout. Argument
and result compatibility are established through their parameter and result
Layouts.

## Layouts and value flow

A Layout describes the ordered values supplied or required by an expression,
declaration, Function, or Type. Library reuses TTX Layouts directly.

Function parameters and results may be positional or named:

```ttx
public pair : func = [Unsigned_64, Bool] -> [Unsigned_64, Bool]

public classify : func = [.value : Unsigned_64] -> [
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

The consuming declaration or expression fits a source Layout directionally
against the Layout it requires. Producing several values creates value flow,
not an anonymous aggregate Type.

Swizzle selects and reorders named entries:

```ttx
state dimensions : Fixed[Unsigned_64, 2] = packet.[width, height];
```

Plain brackets are reference access on `Access[T]`. They never substitute a
default address:

```ttx
access[index]                 // optional element reference
```

This form does not introduce a Library `Option` Type. It is an address producing
request whose evaluation either finds one element reference or reports absence
to the statement that consumes it. Assignment writes through an engaged
reference and leaves the receiver unchanged when the reference is absent. An
indexed reference cannot be used as an ordinary value. Use `:[...]` when a
value is required.

Colon bracket value access selects values. A missing element yields its Type
default. A ranged selection with a start outside the receiver yields the
default empty View, while a count beyond the remaining values stops at the
receiver boundary. Neither form preserves writable `Access` in its result:

```ttx
bytes:[4]
bytes:[4, 16]
```

The operands must still have integer Types. A value that cannot represent a
valid index or extent selects the same safe default. Another operand Type is a
semantic error.

## Built in Types

Library provides these scalar families:

* `Bool`
* `Signed_8`, `Signed_16`, `Signed_32`, and `Signed_64`
* `Unsigned_8`, `Unsigned_16`, `Unsigned_32`, and `Unsigned_64`
* `Real_32` and `Real_64`
* `Void`

Scalar operations require the exact resolved Type identity expected by that
operation. Library does not silently widen, narrow, retag, or reinterpret a
Constant to make an operation legal.

Generic Types describe contiguous element flow:

```ttx
Fixed[Unsigned_8, 64]
View[Unsigned_8]
Access[Unsigned_8]
Range[Unsigned_64]
```

`Fixed` has a compile time element count. `View` is a borrowed contiguous view.
`Access` additionally carries the language's writable contiguous capability.
`Range` describes a lazy ascending integer sequence. Materializing the same
Generic with the same semantic arguments returns the same Type identity.

### Default values

A default is a Library Type fact. It is not inferred from target zero bits or
from the storage chosen by a compiler.

* `Bool` defaults to `false`.
* Every signed, unsigned, and real scalar Type defaults to its exact zero value.
* Every `View[T]` defaults to an empty View with that exact materialized Type.
* An Alias uses the default of the Type it represents.

`Void`, Enumerations, Structs, Objects, `Fixed[T, count]`, and `Access[T]` have
no implicit default. A missing `value:[index]` is therefore legal only when the
exact element Type admits a default. A missing ranged selection is always an
empty `View[T]`. A Field initializer is an authored value and never defines a
Type default for other declarations.

### Integer ranges

`start...end` constructs `Range[T]` when both endpoints have the same exact
signed or unsigned integer Type `T`. The sequence is half open and advances by
one, so it contains `start` and stops before `end`. It is empty when `start` is
not less than `end`.

A Range is lazy value flow rather than contiguous storage. It does not become a
`View`, an `Access`, or an anonymous aggregate Type. Library does not widen the
endpoints or use a general iterable registry. A `for` statement fits its one
entry binding Layout against the exact `T` carried by the Range.

## Source and Composite Types

Each Library Monograph owns one synthetic Source Type with an empty instance
Layout. Top level declarations enter its Static surface. Instance Fields
cannot. The exact `source` route returns that Source, while ordinary Monograph
lookup forwards only its externally visible Static entries.

Source, Structure, and Object share the Composite Type owner for member
inventories, category lookup, Layout completion, and lifecycle barriers.
Composite is not another declaration model. Each authored Structure retains
its exact Definition, and Object retains that same required Definition through
Structure. Source has no authored Definition: it supplies the fixed `source`
name, opening Documentation, empty instance Layout, and root publication
semantics directly.

Top level Field declarations are Static Addressables owned by that Source. They
retain the ordinary Field exposure, writability, Type, and initializer
contracts, but never enter the source instance Layout. Root Functions may
resolve those exact identities as bare source names. Private Fields remain
limited to their owning source context.

Type aliases use the exact declaration
`public|private TypeName : alias = TypeRoute;` in the synthetic Source or an
authored Structure. The receiving Composite retains one exact TTX Alias in its
Type category and authored order. Its target is the Type selected through that
Composite's private local and enclosing source context, so an Alias may
name a private Type without making that target independently public. Public
Type lookup exposes the same Alias identity while private aliases remain local
to their containing Composite.

Authored alias documentation leads the target documentation. An alias without
local prose borrows the target documentation directly, avoiding an empty
wrapper while preserving the visible documentation chain.

Source, Structure, and Object bodies use the same declaration language. Each
Field, Function, Struct, Object, Enumeration, or Alias becomes its exact
semantic identity, and the receiving Composite routes that identity by its TTX
category. The Monograph reaches those declarations only through the Source, so
there is no parallel declaration tree.

Completion follows the relationships in that tree. Enumeration storage and
explicit declaration Types settle first. An inferred Field then adopts the
exact completed Type of its initializer. Explicit Fields fit their initializers
against their declared Types. Every Field settles before Callable signatures,
and signatures settle before Function bodies.

Library owns the grammar that applies to a complete source. `using` selects
Package members through the Monograph and installs Aliases owned by the
importer in the Source without adding another declaration model.

The Source retains the exact Documentation that opens the Library source.
A Package member Alias can therefore route through `source` to one documented
root Type without copying the prose or becoming a Type itself.

A root Function is hosted by the Source but still retains its Monograph as the
source of diagnostics and imports. Hosting and source identity are separate
edges.

## Definitions

Every ordinary Library member begins with one shared Definition:

```ttx
@tooling("entry") public twice : func = [
  .value : Unsigned_64,
] -> Unsigned_64 {
  return value * 2;
}
```

The Definition greedily retains Documentation, every Attribute, all modifiers,
the name, and the qualifier after `:`. The containing Composite then dispatches
that qualifier. A Type route or `=` begins a Field, `alias`, `enum`, `struct`,
and `object` begin their Type forms, and `func` begins a Callable. Each concrete
Field, Structure, Object, Enumeration, or Function retains that same Definition
while owning the grammar and validation of the remaining form.

Attributes do not choose the definition category and are never rejected merely
because of that category. A consumer may interpret selected keys and leave all
others as authored facts. Repetition is likewise consumer policy rather than a
shared parser error.

## Fields

A Field is a TTX Addressable owned by one Composite. Structure and Object Fields
enter the instance Layout, while Source Fields remain Static. Exposure and
writability are independent.

```ttx
public width : Unsigned_64 = 0;
private checksum : Unsigned_64 = 0;
public const signature : Unsigned_64 = 1;
private state updates : Unsigned_64 = 0;
expose state progress : Unsigned_64 = 0;
```

Exposure controls selection:

* `private` is visible only to code hosted by the containing Type.
* `public` is visible outside the containing Type.
* `expose state` makes state readable externally while retaining internal write
  authority.

Writability has three states:

* an ordinary Field is fully writable by callers that can select it
* `state` is writable only by code hosted by the containing Type
* `const` is writable only during initialization

Every view exposes the same Field identity. Exposure does not create a public
copy, and writability does not change the underlying TTX Addressable.

A present initializer links through the Field in its containing Type's private
context and must fit the declared Field Type. It remains one
exact Expression supplying one value rather than a general value Flow.

A declaration written as `name := expression` has no declared Type to fit. The
Field retains the exact completed Type of that initializer without widening or
retagging it. `new` cannot be used here because Object initialization requires
an exact receiving Object Type before the initialization transaction begins.

## Structs

`struct` declares an inline value Type:

```ttx
public Packet : struct {
  public width : Unsigned_64 = 0;
  public height : Unsigned_64 = 0;
  private checksum : Unsigned_64 = 0;

  public area : func = [self] -> Unsigned_64 {
    return self.width * self.height;
  }
}
```

The Struct's instance Layout is a named Layout over its exact Fields in authored
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

  public advance : func = [self, .amount : Unsigned_64] -> Unsigned_64 {
    self.progress = self.progress + amount;
    return self.progress;
  }
}
```

An Object value is a nonnull managed reference identity. Assignment, parameter
passing, and return preserve that identity, so aliases observe the same
mutations. Object reuses the Structure model's Fields, Functions, Layout,
Exposure, and Writability rather than defining a parallel member model.

Library owns the lifetime semantics. Allocation strategy, pointer shape,
collector policy, and reclamation timing belong to the compiler and runtime.
Object exposes no finalizer, weak reference, explicit release, or observable
reclamation order.

### Object initialization

Inline Struct values use positional or named value flow and are fitted by the
typed declaration that receives them. Object initialization uses `new` only as
the initializer of a declaration that already names one exact Object Type:

```ttx
state session : Session = new;
state configured : Session = new(.progress = 4);
```

The receiving declaration owns the transaction. It initializes one unpublished
Object, applies Field writes in authored order, and publishes the
nonnull identity only after every initializer succeeds. `state session := new;`
is invalid because inference cannot supply the Type that initialization needs.
Calls and returns may carry an already initialized Object while preserving its
identity, but they do not infer an Object Type for a new transaction.

Named arguments fit the receiving Object Type's initialization Layout. An
external initializer can name its public and exposed Fields. Code hosted by
the Object Type can also name private Fields. An unknown, duplicate, or
inaccessible name fails the transaction. Every Field without an authored
initializer is required unless `new` supplies it.

Supplied expressions evaluate in source order. The Object then initializes
each Field exactly once in the Type's authored order, using the supplied fitted
value when present and otherwise the Field's own initializer. A missing required
Field or failed expression leaves no published Object identity.

A chain of required Object initializers must terminate. Library rejects a
mandatory initialization cycle during completion rather than recursing while a
runtime Object is being initialized.

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
public add : func = [
  .left : Unsigned_64,
  .right : Unsigned_64,
] -> Unsigned_64 {
  return left + right;
}
```

A Function without `self` is Static. Static means there is no implicit Self
value. Source still selects it through a Type or source context:

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
through `->`. The parameter Layout carries the role without a second
Callable category.

## Expressions and Constants

Library expressions retain authored value dependencies and resolve one result
Type. Constants cover Bytes, Bool, signed integers, unsigned integers, and real
values.

Arithmetic and comparison operate on exact compatible scalar Types. `and` and
`or` preserve short circuit reachability. Unary `!` accepts Bool. Unary `-`
accepts signed integer and real domains. Integer overflow and division by zero
are semantic failures in their owning operation. Safe `:[...]` selection
uses a default value instead of publishing a bounds failure.

Binary `+` accepts exact signed, unsigned, or real operands and returns that
same Type. It does not concatenate Bytes or Views. An output owner that accepts
several byte spans exposes that operation as a Callable instead of changing the
numeric operator.

Constant evaluation may cache a result, but it never replaces the authored
expression or its exact edges. A compiler may fold a complete expression,
address selection, or indexed byte value while the authored graph remains
available to tools.

## Statements and control flow

A Function body is a Library semantic object, not a lowered control flow graph.
It retains Blocks, local Addressables, effects, and control relationships in
source order. Lowering derives target blocks and branches only after the body is
complete.

A local `state` declaration creates one mutable Addressable. A local `const`
declaration can be initialized once. An explicit Type receives and fits the
initializer. An inferred local retains the initializer's exact completed Type
under the same rules as an inferred Field. A local becomes visible after its
declaration. A nested Block may shadow it with a different identity.

Assignment selects one exact writable Addressable. Compound assignment applies
the corresponding exact Type operation before writing the result. Indexed
assignment writes only when its optional reference is engaged. No assignment
falls through from Address access to Type or Callable lookup.

`return` fits its complete source Layout against the Function result Layout.
Bare return and ordinary fallthrough are legal only for concrete `Void`. A
Function with another result must return on every reachable path. An empty
Layout is not another spelling for `Void`.

`if` and `while` require one exact `Bool` expression. `for` consumes one
`Range[T]` and fits its loop binding Layout against the Range entry. `break` and
`continue` target the nearest enclosing loop and are illegal outside one.

`match` evaluates its input once and compares cases in source order. Each case
must fold to a Constant with the input's exact Type. The first equal case runs
and there is no fallthrough. `_` is the final default case. It may be omitted
only when Library can prove that the preceding cases cover the complete input
domain.

An expression statement must be a complete Callable invocation. Its effects
run in source order and the statement deliberately discards its result Layout.
A pure arithmetic, comparison, or access expression is not a statement merely
because it is followed by an end marker.

## Imports and resources

`using` imports the exposed Static surface selected through a Package context:

```ttx
using Core;
using Graphics::Utilities;
```

The route follows ordinary `::` contextual access. Package supplies its exact
member contexts. Library imports eligible public declarations without creating
a second Package path model.

An embedded operand asks the exact source Package for retained bytes:

```ttx
public const signature : Fixed[Unsigned_8, 4] = 0x[54 54 58 31];
public const table := $[resources/table.bin];
public const header := $[resources/table.bin]:[0, 64];
```

Library interprets a successful Resource as a Bytes Constant. Package retains
path confinement and acquisition policy. Library never opens Package storage
directly.

A Library Monograph completes the exact closure of Library providers reached
through its admitted imports. Every reachable declaration Type settles
before any Field is constructed, every Field and initializer settles before
Callable signatures, and every signature settles before any Function body in
that closure begins. Source discovery order therefore cannot change the
completed graph.

## Native publication

Semantic publication and native publication answer different questions. A
public Callable can be selected by another Monograph without promising an
unmangled platform symbol. Library recognizes two Function Attributes when a
native ABI surface is required:

```ttx
@abi("C")
@symbol("library_native")
public library_native : func = [] -> Unsigned_64 {
  return 42;
}
```

`@abi("C")` is legal only on a public Static Function. `@symbol` supplies its
exact requested external spelling and is legal only with `@abi`. Function
interprets those two keys, rejects repeated or malformed requests for either,
and retains every other Attribute without assigning it native meaning. The same
key on another definition is that definition consumer's concern. Function does
not decide whether another Function requests the same global name.

Library lowering proves that every parameter and result Type has a complete C
carrier for the selected target. Linker validates global symbol uniqueness over
the complete target product before emitting native bytes. Function owns neither
target carrier policy nor that product-wide symbol set.

A public Callable without `@abi` still participates in semantic lookup. The
compiler gives any native carrier it needs a deterministic internal symbol
derived from Package identity, member route, owning Type route, receiver role,
and exact Signature. Private Callables never enter the exported symbol
inventory.
[Linker](../linker/README.md) owns the resulting Symbol records and native
bytes.

## Persistence

Library is a persistent Dialect. Its payload records the owner facts needed to
construct Types, Fields, Functions, expressions, access relationships, and
publication policy in a fresh Workspace. It does not record parser state,
process addresses, fold caches, LLVM IR, or native symbols.

Restoration creates new semantic objects and reruns Library completion. The new
graph must reproduce the observable names, categories, identity relationships,
edges, order, Layout behavior, and concrete Library facts promised by the
Archive. It does not have to reproduce the old allocation or internal graph
shape.

## Compilation boundary

Library lowering consumes completed CPU facts owned by Library, App, or Scene.
It derives target object layouts, calling convention carriers, registers,
instructions, and relocations without changing their semantic identities.

Linker owns object modules and final native products. Package owns the Archive
envelope while each persistent Dialect owns its reconstruction payload. Runtime
allocation and execution remain separate from both.

See [TTX semantics](../../ttx/ttx_semantics.md) for the shared contracts and
[Package](../package/README.md) for `using` and resource contexts. The
[standard packages](../../packages/ttx/README.md) apply these contracts to the
provided Math, System, and Graphics surfaces.
