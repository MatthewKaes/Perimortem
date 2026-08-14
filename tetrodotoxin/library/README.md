# Library

Library is Tetrodotoxin's language for reusable CPU code. It provides scalar
Types, values, expressions, Functions, Structs, Objects, Enumerations, and
Generic containers. Packages, applications, Scenes, tools, and native compilers
all work with those same language objects instead of translating them through a
separate intermediate model.

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
| `option!`                            | return the payload or a fresh element default                  |
| `option?`                            | continue with the payload or return empty flow                 |

These domains never fall through to one another. A Field, Callable, and nested
Type may share a spelling because the operator already states which category is
being requested.

A declaration that requires a Type retains a type route with no identity. It
does not retain an access Expression. The route may carry one optional Generic
argument Layout, and each Type entry may recursively contain another route.
Without an argument Layout, the route must select a Type. With one, it must
select a Generic formula that creates the Type from those arguments. The route
can stay unresolved until linking without pretending to be a runtime value or
postfix access expression.

### Address access

`.` evaluates its receiver and selects one exact TTX Addressable from that
identity. An Addressable receiver can select state Fields and const Fields owned
by its Type. An exact Type receiver can select ordinary Static Fields and const
Fields. An exact Source receiver can select its ordinary Static Fields and const
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

Source cannot be instantiated and rejects state Fields. Its ordinary Fields
are Static values with global construction and lifetime. Structure and Object
Type results expose their ordinary Static and const Field categories. State
Fields require one exact Addressable receiver.

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
Option[Graphics::Image]
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
directionally against the descriptor it requires. `()` fits `[]` without
becoming a Type or sharing Pack identity.

Swizzle selects named Addressables and returns their values as one reordered
positional Pack:

```ttx
state dimensions : Fixed[Unsigned_64, 2] = packet.[width, height];
```

The result is a Pack over the real selected producers. It becomes
`Fixed[Unsigned_64, 2]` only because the receiving declaration chooses that
Type. The swizzle itself creates no aggregate Type.

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

## Built-in Types

Library provides these scalar families:

* `Bool`
* `Signed_8`, `Signed_16`, `Signed_32`, and `Signed_64`
* `Unsigned_8`, `Unsigned_16`, `Unsigned_32`, and `Unsigned_64`
* `Real_32` and `Real_64`

Library has no zero-value Type. An authored `[]` is the empty result Layout and
an empty Composite is a Static namespace rather than an instantiable value.

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
Option[View[Unsigned_8]]
```

`Fixed[T, extent]` requires its `extent` to be a positive exact `Unsigned_64`
value known during linking. Every generated container Type requires an element
Type with a nonempty Layout. `View` is a borrowed contiguous view.
`Access` additionally carries the language's writable contiguous capability.
`Range` describes a lazy ascending integer sequence. `Option[T]` represents a
value that may be absent in an otherwise nonnullable language. The Option Type
always has a nonempty Layout. Its state either carries one exact `T` or carries
no payload. An explicit empty list applies a formula with no arguments.
Omitting the list instead requires the route to name a Type. Applying the same
formula to the same semantic arguments returns the same Type identity.

Option construction belongs to target fitting:

```ttx
state absent : Option[Result] = ();
state present : Option[Result] = result;
```

The empty Pack creates the state with no payload. A Pack accepted by `T` creates
the state that carries its value. Option has no `some` or `empty` construction
Callables.

This is Pack fitting rather than Layout fitting. `[]` does not fit
`Option[T]`, and Option never acquires an empty Layout. Its absent state can
produce `()` only through the flow control owned by postfix `?`.

Option is a built-in Library Generic Type. It does not make Objects nullable and
it is not supplied by a standard Package. A user-defined operation that may fail
is written as a Static factory returning an Option. Object initialization itself
never publishes a partly initialized value.

### Default values

Every Library Type admitted to value flow has a default value. The language
defines that value independently of the storage chosen by a compiler:

- `Bool` is false and numeric Types use zero.
- An Enumeration uses its underlying zero value even when no case names
  zero.
- `View[T]` and `Access[T]` use empty read-only and writable views respectively.
- `Range[T]` uses the empty range.
- `Option[T]` uses the state with no payload and does not construct `T`.
- `Fixed[T, count]` contains `count` default `T` values.
- A Structure initializes state Fields in source order from each Field's
  authored initializer when present and otherwise from that Field Type's
  default.
- An Object default is one new nonnull Object initialized by the same Field
  rules.
- An Alias uses the default of the Type it represents.

`Descriptor` belongs to compile-time Type selection and cannot be used as an
ordinary source value. Library also rejects a chain of defaults that would have
to construct itself forever. `Option[T]` breaks such a chain because its absent
default does not construct `T`.

Cleared memory may make initialization faster, but it does not define these
defaults. Every initializer required by the Type still runs. An empty
`View[Unsigned_8]` is still one View value rather than a Pack with no values.

A missing scalar `value:[index]` returns the element Type's default. Slice
operates only on Types that provide contiguous storage. Postfix `option!`
returns its payload when present and creates a default `T` otherwise. Applying
it to an Option of an Object Type more than once can therefore create a
different Object each time. The Option itself does not change. A ranged
selection still returns exactly its declared count and does not fill missing
entries with defaults.

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

Each Library Monograph owns one generated Source Type with an empty instance
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

Each Monograph creates and retains its Source with one generated Definition.
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
`public|private TypeName : alias = TypeRoute;` in the generated Source or an
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

### Embedded Library layers

A top-level Library source is already a Library layer. Scene and Shader can also
contain a Library child built by the same Library language installed in the
Workspace. Reusing that language keeps Generic Types such as `Option[T]`,
`Fixed[T, count]`, and `View[T]` consistent everywhere they appear.

The child has its own Source context for imports, Foreign declarations, and
Package access. A Scene places its Object, state Fields, helpers, and lifecycle
Functions there. A Shader uses its child for CPU helpers and marshaling. Only
the outer Scene or Shader appears as a Package member, but Library tools can
inspect the real child directly. Nothing is copied into a second Type or member
list.

Scene and Shader remain responsible for the parts of their languages that are
not Library code. For example, Scene owns `emit` while the expressions and
ordinary statements around it still follow Library rules. This lets the Library
compiler handle the CPU code without making Library depend on Scene or Shader.

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

Definition remembers the host that admits the declaration. Ordinary members use
their containing Composite, while Source uses its Monograph. That host explains
where the declaration came from and which private members it may access. It is
not a universal parent link or an implicit receiver.

Defined Types retain their Definition as part of the Type identity. Fields,
Functions, and authored Aliases retain the same declaration facts while
remaining solely Addressable, Callable, and Alias identities. Once its grammar
is complete, an authored identity exposes the Definition's Documentation,
complete Anchor, and publication decision as Authorship with no identity.

Definition owns Library Visibility and the authored Tokens. Source uses a
generated Definition and does not pretend to have authored declaration text.
Forwarding Aliases created by imports are generated TTX Aliases without a
Definition.

Attributes do not choose the definition category and are not rejected because
of that category. A consumer may interpret selected keys and leave all
others as authored facts. Repetition is likewise consumer policy rather than a
shared parser error.

## Fields

A Field is a TTX Addressable owned by one Composite. `state` is the sole
authored discriminator for instance storage, so only state Structure and Object
Fields enter the instance Layout. An ordinary mutable Field is Static even when
hosted by a Structure or Object. Source rejects state Fields and retains only
ordinary Static or const Fields. A const Field never enters the receiver's
instance Layout. Its initializer must fold before the Abstract DAG is complete.
Address access through its declaring Type, an Addressable instance, or Source
selects the same immutable declaration value. Visibility and evaluation policy
remain independent.

A constant domain may retain memory for its completed representation. A
default constructed Object is a valid const value only when linking can produce
its complete immutable representation. The declaration
`const object : SomeObject = new[SomeObject];` is legal only when construction
provides that proof during linking.

A Field Type must expose at least one Layout entry because an Addressable names
real value flow. `Fixed[T, 0]` and a Generic application with an empty element
Type are invalid. An empty Composite remains a valid contextual Type but cannot
become a Field, named parameter, Function result entry, or `self`.
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

* an ordinary Field owns Static storage and is fully writable by callers that
  can select it
* `state` owns instance storage. `public state` is writable by external callers,
  while `private state` and `expose state` require authority from the declaring
  Type's Definition host chain
* `const` is never writable and must resolve completely at compile time

Every view exposes the same Field identity. Visibility does not create a public
copy, and evaluation policy does not change the underlying TTX Addressable.

A present initializer links through the Field in its containing Type's private
context and must fit the declared Field Type. A const initializer must also
fold completely during linking. It remains one exact Pack that produces one
value rather than a separate initializer inventory.

A declaration written as `name := expression` has no declared Type to fit. The
Field retains the exact completed Type of that initializer without widening or
retagging it. `new[ObjectType]` carries its exact result Type, so an inferred
declaration may use it.

## Structs

`struct` declares an inline value Type:

```ttx
public Packet : struct {
  public state width : Unsigned_64 = 0;
  public state height : Unsigned_64 = 0;
  private state checksum : Unsigned_64 = 0;

  public area : func = [self] -> Unsigned_64 {
    return self.width * self.height;
  }
}
```

The Struct's instance Layout is a named Layout over its exact state Fields in
authored order. Ordinary Static and const Fields remain on the same authored
inventory but do not enter that Layout. Copying a Struct value copies its inline
value semantics. Target offsets and padding are derived later by the compiler.

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

An Object value is a nonnull managed reference. Assignment, parameter passing,
and return all preserve that reference, so every alias sees the same mutations.
Object uses the same Fields, Functions, Layout, Visibility, and Writability as a
Structure instead of defining a second member system.

Library defines what Object lifetime means to a program. The runtime chooses how
Objects are allocated, represented, traced, and eventually reclaimed. Source
code has no finalizer, weak reference, explicit release, or observable
reclamation order.

At runtime, related Objects live in a Garbage Realm owned by one worker. The
whole Realm can move to another worker without invalidating any of its Object
references. A single Object cannot cross on its own. Code must instead copy it
into a new identity, transfer its complete Realm, or use separately shared
read-only storage. These choices do not change the Library Type or add
source-visible lifetime operations.

### Object initialization

Inline Struct values use positional or named values and are checked against the
declaration that receives them. Object initialization spells its exact
nonempty Object Type in `new[ObjectType]`:

```ttx
state session : Session = new[Session];
state configured := new[Session](.progress = 4);
```

The declaration creates one private Object, initializes its Fields in source
order, and makes the nonnull reference visible only when initialization is
complete. Bare `new` is invalid. The explicit Type makes inferred declarations
unambiguous while construction remains a declaration initializer rather than a
general expression. An Object Type with an empty Layout cannot be constructed.

The arguments to `new[ObjectType]` can name public and exposed state Fields.
Code hosted by the Object Type can also name its private state Fields. Unknown,
repeated, or inaccessible names are errors, as are Static or const Fields. A
state Field not supplied by `new[ObjectType]` uses its own initializer when
present and otherwise its Type's default. Static Fields are initialized
separately and are never inputs to construction. Omitting the argument list
requests those defaults. An explicit empty argument list is invalid, so `()`
never becomes a default initialization marker.

Arguments are evaluated in source order. The Object then initializes each state
Field once in its declared order. It uses the supplied value first, then the
Field's initializer, and finally the Field Type's default. Initialization has no
recoverable failure path, so it needs no rollback behavior. Running out of
memory is a fatal diagnostic. Cleared memory may speed up allocation, but the
language defaults still determine the finished values.

A chain of Structure or Object defaults must eventually end. Library rejects a
cycle while completing the program instead of discovering it during runtime
initialization. An `Option[T]` Field breaks the cycle because its default has no
payload. Construction that can reject input belongs in a Static factory
returning `Option[T]`, not in `new`.

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
The Enumeration default is the exact Enumeration value whose underlying
integer is zero. That representable value remains valid even when no case Alias
names it.

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

Arithmetic and comparison operate on exact compatible scalar Types. The
keyword forms `and` and `or` alone own short-circuit Boolean semantics. The
host-neutral `&` and `|` Tokens remain reserved for future bitwise operators and
are not alternate spellings of those Library Operations. Prefix `!value`
accepts Bool. Postfix `option!` accepts `Option[T]` and produces exact `T`,
using the Type default when the Option has no payload. Unary `-` accepts signed
integer and real domains. Integer overflow and division by zero are semantic
failures in their owning operation. Safe
`:[...]` selection uses a default value instead of publishing a bounds failure.

Postfix `?` makes a chain of fallible operations concise without introducing
nullable values or truthiness. Its left side must be `Option[T]`. A present
payload continues the chain as exact `T`. A state with no payload returns an
empty Pack from the enclosing Function and evaluates nothing to the right.

The enclosing Function must accept that empty flow. Its result is either `[]`
or one `Option[R]`. An empty Pack fits the latter as absence, while a successful
`R` Pack fits it as presence:

```ttx
state parsed := Parser -> parse(source)?;
return parsed -> finish();
```

The same operator works as an early return in a Function with result `[]` when
the successful value is consumed before the final `return;`. Propagation into
a Function with several result entries is reserved for future language support.

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
`()` fits `[]` directly. It also fits a single `Option[T]` result by creating
the state with no payload. A Function with a nonempty result must return on
every reachable path.

`if` and `while` consume a Pack and use its first produced value for the control
decision. That value's Type must satisfy the Flag contract. Parentheses may be
omitted when the Pack is otherwise unambiguous, and additional produced values
do not change which entry controls the branch. `for` consumes one `Range[T]`
and fits its loop binding Layout against the Range entry. `break` and `continue`
target the nearest enclosing loop and are illegal outside one.

`match` evaluates its input once and compares cases in source order. An ordinary
case must fold to a Constant with the input's exact Type. The first equal case
runs and there is no fallthrough. `_` is the final default case. It may be
omitted only when Library can prove that the preceding cases cover the complete
input domain.

An `Option[T]` input instead admits one value binding and one final discard
case:

```ttx
match value {
  case item: {
    item -> consume();
  }
  case _: {
  }
}
```

The first case makes the stored `T` available only inside that branch. The
discard case observes the state with no payload and introduces no binding.
Option element Types always have nonempty Layouts. General runtime Type patterns
require a real sum or dynamic-Type domain. They are not meaningful for ordinary
values that already have one known static Type.

An invocation statement must be a complete Callable invocation. Its effects
run in source order and the statement discards its result Pack.
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

The Library compiler checks that every parameter and result Type has a valid C
representation for the selected target. Linker checks that exported symbol
names are unique before it emits native bytes. A Function records its own
request, but it does not decide either target representation or whole-program
symbol policy.

A public Callable without `@abi` is still available to language lookup. The
compiler gives it a stable internal symbol when native code needs one. Private
Callables never enter the exported symbol list.
[Linker](../linker/README.md) owns the resulting Symbol records and native
bytes.

## Persistence

Library can be stored in a Package Archive and rebuilt without its source file.
A Complete payload keeps public and private Types, Fields, Functions,
expressions, control flow, access relationships, Foreign declarations, and the
connections needed to compile the Library again. An Interface payload keeps the
public Types, Layouts, Fields, Function signatures, folded public constants,
ABI requests, and native artifact locations, but leaves out Function bodies.

Neither profile stores parser state, process addresses, compiler caches, LLVM
IR, native bytes, live Object references, or source-level debugging data.

Restoring an Archive creates new Library objects and completes them through the
same rules used for source. The result preserves all names, categories,
relationships, ordering, Layout behavior, and other visible facts promised by
the selected profile. Its in-memory arrangement does not need to match the old
process. A Library child inside Scene or Shader uses the same Complete or
Interface profile as its parent.

## Compilation boundary

The Library compiler accepts either a top-level Library or the Library child
inside Scene or Shader. App may select one Static Callable as the program entry,
but App itself does not become Library code. Compilation chooses object layouts,
calling conventions, registers, instructions, and relocations without changing
the language objects seen by tools.

The CPU target chooses the instruction set, data layout, and calling convention.
x86-64 System V and x86-64 Win64 are separate targets. Tetrodotoxin can compile
through LLVM IR or through its direct x86-64 compiler. Both produce the same
kind of object module for Linker. LLVM does not own Package locations, operating
system startup, or linking rules.

Linux and Windows hosts provide process entry, runtime and System services,
loader inputs, and the executable format around the CPU code. Linker owns ELF,
COFF, PE, symbols, relocations, and final native files. Package owns the Archive
that stores language payloads. Runtime allocation and execution happen after
both compilation and restoration.

See [TTX semantics](../../ttx/ttx_semantics.md) for the shared contracts and
[Package](../package/README.md) for `using` and resource contexts. The
[standard packages](../../packages/ttx/README.md) apply these contracts to the
provided Math, System, and Graphics surfaces.
