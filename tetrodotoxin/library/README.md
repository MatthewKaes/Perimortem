# Library

Library is Tetrodotoxin's reusable CPU language. It defines concrete scalar
Types, values, expressions, functions, Structs, Objects, Enumerations, and
Generic containers while retaining the shared TTX Type, Pack, Layout,
Addressable, and Callable contracts.

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

Every Library access evaluates the one Expression on its left. An Expression
exposes its exact semantic result separately from its output Type. Ordinary
value operations use the output Type, while an Expression whose result is a
semantic Type has the singleton `Descriptor` output Type. `Descriptor` does
not wrap or copy the selected Type. The Expression result retains that exact
identity for the next access.

Library uses punctuation to select separate semantic domains:

| Syntax                               | Meaning                                                       |
| ------------------------------------ | ------------------------------------------------------------- |
| `expression.name`                    | select one named value from the receiver Pack's Layout        |
| `expression::Type`                   | produce one exact Type result with `Descriptor` output        |
| `receiver -> callable(arguments...)` | fit one argument Pack and invoke one Callable                  |
| `value.[names...]`                   | select and reorder named Pack values                           |
| `access[index]`                      | produce one writable indexed address with the element Type    |
| `value:[index]`                      | return an element value or its default                        |
| `value:[start, count]`               | return a Ranged Pack whose size is known during linking        |

These domains never fall through to one another. A Field, Callable, and nested
Type may share a spelling because the operator already states which category is
being requested.

A declaration that requires a Type retains a type route with no identity. It
does not retain an access Expression. The route may carry one optional Generic
argument Layout, and each Type entry may recursively contain another route.
Without an argument
Layout the route must select a Type. With one it must select a Generic formula
that materializes the exact Type from those arguments. This keeps forward
declaration routes delayed without manufacturing runtime value flow or
conflating declaration qualification with postfix access.

### Address access

`.` evaluates its receiver and selects one exact TTX Addressable from that
identity. An Addressable receiver can select mutable instance Fields and const
Fields owned by its Type. An exact Type receiver can select only const Fields.
An exact Source receiver can select its mutable Static Fields and its const
Fields. Arbitrary computed values do not provide mutable member access. The
operation creates no group Type.

```ttx
packet.width
self.progress
foreign.external_counter
```

The selected mutable Addressable identifies one semantic address and its Type.
A compiler may realize it as a stack location, an offset from an inline Struct,
or an offset from an Object reference. A const Field instead identifies one
value completed during linking. Selection through its Type, an Addressable
instance, or Source returns that same foldable declaration value. It never
creates storage relative to the receiver.

Source cannot be instantiated, so its mutable Fields are Static values with
global construction and lifetime. Structure and Object Type results expose
only their const Field category. Mutable Fields require one exact Addressable
receiver.

The caller has private authority for every Composite in its Definition host
chain. That chain authorizes members selected from an explicit receiver. It
does not supply an implicit receiver or create another lookup path. A hosted
Function still writes `self.field` or selects the Field through another
explicit value. A Static Function cannot read a host Field as a bare
identifier.

### Type access

Postfix `::` is a Type access Expression:

```ttx
Graphics::Image
System::Terminal
Scene::Flow
```

It evaluates its receiver, requires that receiver's exact semantic result to be
a Type, and selects one Type from that context. The access result is the exact
selected Type and its output Type is `Descriptor`, so another `::` or a Static
invocation can use the result without treating the selected Type as one of its
own values. An ordinary value cannot use `::`, and `Descriptor` supplies no
instance Layout for `.`.

Contextual declaration routes through Alias, Package, Monograph, Library
source, and Type objects remain references with no identity. They do not become
Expressions.

Qualification preserves the original caller authority across every segment.
An Alias is opaque to access and declaration operations: they may only ask it
to resolve. Resolution may reveal another identity, but it does not add that
identity to the caller's host chain or transfer its private authority.

### Callable access

`->` is the Callable access and invocation operator:

```ttx
Packet -> create(width, height)
packet -> resize(width, height)
System::Terminal -> write_line(message)
```

An invocation evaluates one receiver Expression and retains one parenthesized
argument Pack. An exact Type result selects the Static Callable registered on
that Composite. A typed value receiver uses its output Type to select the
registered Self Callable. The caller's Definition host chain remains unchanged
while making that selection: it admits the receiver's private surface only when
that exact Composite is already in the chain. Resolving an Alias never
transfers private authority.

Static and Self are properties of each Callable's parameter Layout. A Callable
is Self exactly when parameter entry zero is the reserved `self` Addressable
with the receiver's exact Type. Otherwise it is Static. A Composite admits at
most one Callable for each spelling and receiver role, rejecting a duplicate
during registration. Static and Self Callables may share a spelling. The
invocation therefore selects one registered Callable and only then fits its
argument Pack against the remaining parameter entries. It never constructs an
overload set or reports ambiguity during a call.

A Callable is not an Addressable and never appears in a value Layout. Callable,
Addressable, and Type registration are independent spaces, so sharing a
spelling across those categories creates no collision or fallback. The
invocation is a Pack whose output follows the selected Callable's complete
result Layout, including an empty Layout or one with several entries. A scalar
consumer can use it only when that Pack proves one exact result Type. `->`
introduces neither
an implicit receiver nor a universal member resolver: `.`, `::`, and `->`
continue to ask their distinct semantic questions.

## Packs and Layouts

A Pack carries produced value flow. It retains the real producer identities and
exposes one output Layout for directional fitting. A Layout is a descriptor
with no identity. It promises the ordered shape accepted or exposed by a Type,
declaration, Function, or Pack. Producing several values therefore remains
fluid Pack flow rather than materializing an anonymous aggregate Type.

Parentheses group Packs and brackets describe Layouts:

```ttx
()                              // empty Pack
(value)                         // the same Pack as value
(left, right)                   // positional Pack
(.x = left, .y = right)         // named Pack

[]                              // empty Layout
[Unsigned_64, Bool]             // positional Layout
[.x : Unsigned_64, .y : Bool]   // named Layout
```

A Function always has an empty or Named parameter Layout. Its result may use
any empty, scalar, positional, or Named Layout:

```ttx
public pair : func = [
  .left : Unsigned_64,
  .right : Bool,
] -> [Unsigned_64, Bool]

public classify : func = [.value : Unsigned_64] -> [
  .accepted : Bool,
  .adjusted : Unsigned_64,
] {
  return (.accepted = value > 0, .adjusted = value + 1);
}
```

The leading `.accepted` and `.adjusted` spellings are not postfix Address
access because they have no receiver. `.accepted : Bool` names a descriptor
slot, while `.accepted = expression` names supplied Pack flow. A slot name need
not be the semantic name of its producer, and fitting still returns that exact
producer without a renamed value or Alias. Keeping `:` for descriptors and `=`
for Packs also reserves `.name : Type = expression` for an explicitly typed
default and `.name := expression` for an inferred one.

Both forms share rules for empty forms, separators, trailing commas, positional
and named entries, and duplicate names. They do not share one semantic owner. A
Generic application accepts a Layout of Type references and literal Constants. A
Function signature accepts descriptor Types and parameter names. Parenthesized
value flow accepts Packs. A Call requires those parentheses. Another context
may omit them when its grammar remains unambiguous.

A receiving declaration or operation fits the Pack's complete output Layout
directionally against the descriptor it requires. Empty `Void`, `()`, and `[]`
agree through that fitting without becoming one Type or one Pack identity.

Swizzle selects named Addressables and returns their values as one reordered
positional Pack:

```ttx
state dimensions : Fixed[Unsigned_64, 2] = packet.[width, height];
```

The result is a Pack over the real selected producers. It becomes
`Fixed[Unsigned_64, 2]` only because the receiving declaration deliberately
materializes that Type. The swizzle itself creates no aggregate Type.

Plain brackets are reference access on `Access[T]`. They never substitute a
default address:

```ttx
access[index]                 // optional element reference
```

This form does not introduce a Library `Option` Type. It is an Expression whose
Pack produces one writable address with the exact element Type. Runtime
bounds determine whether that address is engaged. Assignment writes through an
engaged address and leaves the receiver unchanged otherwise. A value consumer
reads through the same address. Use `:[...]` when a missing element should
instead produce the Type's default value.

Colon bracket value access selects values. A missing scalar element yields its
Type default. A ranged selection requires its count to fold during linking to
one supported nonnegative integer and returns a Ranged Pack with exactly that
many element values. It does not materialize `Fixed`, `View`, or an anonymous
aggregate Type merely to carry the range. Neither form preserves writable
`Access` in its result:

```ttx
bytes:[4]
bytes:[4, 16]
```

The operands must still have integer Types. A scalar index that cannot represent
a valid position selects the same safe default. A range count that does not
fold to a constant or cannot represent a supported nonnegative count is a semantic
error, as is another operand Type.

## Built in Types

Library provides these scalar families:

* `Bool`
* `Signed_8`, `Signed_16`, `Signed_32`, and `Signed_64`
* `Unsigned_8`, `Unsigned_16`, `Unsigned_32`, and `Unsigned_64`
* `Real_32` and `Real_64`

Library also names `Void` as its empty result Type. `Void` is not a scalar and
has no value entry.

Scalar operations require the exact resolved Type identity expected by that
operation. Library does not silently widen, narrow, retag, or reinterpret a
Constant to make an operation legal.

Generic formulas describe reusable Type families. A formula is not itself a
Type. Applying its ordered arguments materializes one exact Type. Type arguments
may recursively apply another formula:

```ttx
Fixed[Unsigned_8, 64]
View[Unsigned_8]
View[Fixed[Unsigned_8, 4]]
Access[Unsigned_8]
Range[Unsigned_64]
```

`Fixed[T, extent]` requires its `extent` to be an exact `Unsigned_64` value
known during linking. `View` is a borrowed contiguous view. `Access` additionally
carries the language's writable contiguous capability. `Range` describes a
lazy ascending integer sequence. An explicit empty list applies a formula with
no arguments. Omitting the list instead requires the route to name a Type.
Applying the same formula to the same semantic arguments returns the same Type
identity.

### Default values

A default is a Library Type fact. It is not inferred from target zero bits or
from the storage chosen by a compiler.

* `Bool` defaults to `false`.
* Every signed, unsigned, and real scalar Type defaults to its exact zero value.
* Every `View[T]` defaults to an empty View with that exact materialized Type.
* An Alias uses the default of the Type it represents.

`Void`, Enumerations, Structs, Objects, `Fixed[T, count]`, and `Access[T]` have
no implicit default. A missing `value:[index]` is therefore legal only when the
exact element Type admits a default. A ranged selection instead requires one
folded nonnegative count and produces exactly that many values. It has no
default for a missing selection. A Field initializer is an authored value and never
defines a Type default for other declarations.

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

Every Composite and Enumeration is a defined Type and retains exactly one
Definition. Source, Structure, and Object follow that same rule, while Fields,
Functions, and authored Aliases retain Definitions without changing their TTX
categories. A Definition contributes authorship and host authority to the real
semantic identity and never becomes a competing declaration identity or graph.
Composite owns member categories, Layout completion, and lifecycle barriers
without becoming another declaration model.

Each Monograph creates and retains its Source with one synthetic Definition.
That Definition uses the reserved name `<source>`, which cannot be emitted. It
also retains the exact opening Documentation and truthful source envelope
Anchor supplied by Environment. It
fabricates no authored Tokens, and Source exposes no authored Authorship. Its
host is the owning Monograph and its Visibility is public.

Top level mutable Field declarations are Static Addressables owned by that
Source. They retain the ordinary Field visibility, mutation policy, Type, and
initializer contracts. They have global construction and lifetime and never
enter the empty Source instance Layout. Top level const Fields retain one value
completed during linking and use no Source instance storage. A constant domain
may still retain the memory needed to represent that value. Root Functions may
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

Completion follows the relationships in that tree. Every reachable declaration
Type, including Enumeration storage, settles first. Every reachable Callable
signature then settles before any Field or initializer expression. An inferred
Field adopts the exact completed Type of its initializer, while an explicit
Field fits its initializer against its declared Type. All Fields and
initializers settle before Function bodies.

Library owns the grammar that applies to a complete source. `using` selects
Package members through the Monograph and installs Aliases owned by the
importer in the Source without adding another declaration model.

The Source retains the exact Documentation that opens the Library source.
A Package member Alias can therefore route through `source` to one documented
root Type without copying the prose or becoming a Type itself.

The Library Monograph exposes its exact Source and installed Library Dialect
directly, with no category scan or shadow source edge. A root Function's
Definition host is the Source, which already reaches the Monograph that owns
diagnostics, imports, and completion. The Function retains no duplicate source,
host, or parent edge.

## Definitions

Every ordinary Library member begins with one shared Definition:

```ttx
@tooling("entry") public twice : func = [
  .value : Unsigned_64,
] -> Unsigned_64 {
  return value * 2;
}
```

The Definition greedily retains Documentation, every Attribute, one exact
Visibility, ordered evaluation modifiers, the name, and the qualifier after
`:`. That qualifier selects the declaration category, and the containing
Composite retains the resulting identity. A Type route or `=` begins a Field.
`alias`, `enum`, `struct`, and `object` begin their Type forms, while `func` begins
a Callable. Each resulting Field, Structure, Object, Enumeration, or Function
retains that same Definition while its concrete language form owns the
remaining grammar and validation.

Definition retains the exact host that admits the declaration: the containing
Composite for ordinary members and the Monograph for Source. This supplies
transaction provenance and hosted access authority, not universal semantic
parentage or a required TTX graph path. Walking only those Composite hosts
grants a caller private authority over each containing Type while leaving the
selected receiver and semantic graph unchanged.

Defined Types retain their Definition as part of the Type identity. Fields,
Functions, and authored Aliases retain the same declaration facts while
remaining solely Addressable, Callable, and Alias identities. Once its grammar
is complete, an authored identity exposes the Definition's Documentation,
complete Anchor, and publication decision as Authorship with no identity.

Definition alone owns Library Visibility and authored lexical Tokens. Source's
required synthetic Definition does not become Authorship. Forwarding Aliases
created by imports remain synthetic TTX Alias identities with no Definition.

Attributes do not choose the definition category and are not rejected because
of that category. A consumer may interpret selected keys and leave all
others as authored facts. Repetition is likewise consumer policy rather than a
shared parser error.

## Fields

A Field is a TTX Addressable owned by one Composite. Mutable Structure and
Object Fields enter the instance Layout. Mutable Source Fields remain Static. A
const Field never enters the receiver's instance Layout. Its initializer must
fold before the Abstract DAG is complete. Address access through its declaring
Type, an Addressable instance, or Source selects the same immutable declaration
value. Visibility and evaluation policy remain independent.

A constant domain may retain memory for its completed representation. A
default constructed Object is a valid const value only when linking can produce
its complete immutable representation. The declaration
`const object : SomeObject = new;` is legal only when default construction
provides that proof during linking.

A Field Type must expose at least one Layout entry because an Addressable names
real value flow. `Void`, `Fixed[T, 0]`, and an empty Composite remain valid
Types with no values but cannot become Fields, named parameters, or `self`.
An empty Composite can still own Static Functions and nested Types, which makes
it a natural namespace without manufacturing a value for compatibility.

```ttx
public width : Unsigned_64 = 0;
private checksum : Unsigned_64 = 0;
public const signature : Unsigned_64 = 1;
private state updates : Unsigned_64 = 0;
expose state progress : Unsigned_64 = 0;
```

Visibility controls selection:

* `private` is visible only when the caller's Definition host chain contains the
  declaring Type.
* `public` is visible outside the containing Type.
* `expose state` makes state readable externally while retaining internal write
  authority.

Evaluation policy has three states:

* an ordinary Field is fully writable by callers that can select it
* `state` is writable only when the caller's Definition host chain contains the
  declaring Type
* `const` is never writable and must resolve completely at compile time

Every view exposes the same Field identity. Visibility does not create a public
copy, and evaluation policy does not change the underlying TTX Addressable.

A present initializer links through the Field in its containing Type's private
context and must fit the declared Field Type. A const initializer must also
fold completely during linking. It remains one exact Pack that produces one
value rather than a separate initializer inventory.

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
Visibility, and Writability rather than defining a parallel member model.

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

The initializer retains one argument Pack and fits its output against the
receiving Object Type's initialization Layout. An external initializer can name
its public and exposed Fields. Code hosted by the Object Type can also name
private Fields. An unknown, duplicate, or inaccessible name fails the
transaction. Every Field without an authored initializer is required unless
`new` supplies it.

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

A Function declares one Named parameter Layout and one arbitrary result Layout
followed by a body. `[]` is the empty parameter Layout. Every ordinary parameter
uses `.name : Type`. Only the reserved `self` entry may appear first without
that spelling. A scalar Type is shorthand for a result Layout with one entry:

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

A Callable derives type binding from its signature's parameter Layout: it is
type bound exactly when entry zero is the reserved `self` Addressable. A
Function with that shape is Self. That entry has the selected receiver's exact
Type, and every following parameter is named. The Function is selected through
an addressable value:

```ttx
packet -> area()
```

Static and Self Callables may share a name because their receiver roles
distinguish the invocation. A Composite rejects a second Callable with the same
name and role during registration, before any Call can observe the name. Both
remain Callables reached only through `->`. The parameter Layout carries the
role without a second Callable category.

## Expressions and Constants

Library expressions retain authored value dependencies and expose both their
exact semantic result and output Type. The result preserves the identity
selected by an access. The output Type states which value operations apply.
Results that select a Type use `Descriptor` as that output without replacing the
selected Type. Every Expression is also a Pack. Scalar expression consumers
require one exact produced value and output Type, while calls, swizzles, and
slices may preserve empty output or output with several values without
inventing a group Type.
Constants cover Bytes, Bool, signed integers, unsigned integers, and real
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
declaration requires an initializer that folds completely during linking. It
never creates mutable local storage or an assignment target. An explicit Type
receives and fits the initializer. An inferred local retains the initializer's
exact completed Type under the same rules as an inferred Field. A local becomes
visible after its declaration. A nested Block may shadow it with a different
identity.

Assignment selects one exact writable Addressable. Compound assignment applies
the corresponding exact Type operation before writing the result. Indexed
assignment writes only when its optional reference is engaged. No assignment
falls through from Address access to Type or Callable lookup.

`return` retains one Pack and fits its complete output Layout against the
Function result Layout. `return;` and `return ();` supply empty flow.
`return value;` supplies one value. Positional and named parenthesized forms may
supply several. Ordinary fallthrough is legal only for an empty result Layout.
The Library `Void` Type, every other empty Type, `[]`, and `()` therefore agree
as flow with no values without becoming the same Type identity. A Function with a
nonempty result must return on every reachable path.

`if` and `while` consume a Pack and use its first produced value for the control
decision. That value's Type must satisfy the Flag contract. Parentheses may be
omitted when the Pack is otherwise unambiguous, and additional produced values
do not change which entry controls the branch. `for` consumes one `Range[T]`
and fits its loop binding Layout against the Range entry. `break` and `continue`
target the nearest enclosing loop and are illegal outside one.

`match` evaluates its input once and compares cases in source order. Each case
must fold to a Constant with the input's exact Type. The first equal case runs
and there is no fallthrough. `_` is the final default case. It may be omitted
only when Library can prove that the preceding cases cover the complete input
domain.

An invocation statement must be a complete Callable invocation. Its effects
run in source order and the statement deliberately discards its result Pack.
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
through its admitted imports. Every reachable declaration Type settles before
every reachable Callable signature. Those signatures settle before any Field
or initializer expression, and all Fields and initializers settle before any
Function body in that closure begins. Source discovery order therefore cannot
change the completed graph.

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
target carrier policy nor the symbol set for the complete product.

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
