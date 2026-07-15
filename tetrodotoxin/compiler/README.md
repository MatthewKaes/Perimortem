# Compiler

The Compiler is Tetrodotoxin's memory and terminal-product boundary. It owns the
semantic objects created for one build and lowers those objects through a
selected target until only durable artifacts remain.

```text
Toolchain ClassDB + ISA schemas
              |
              v
Compiler-owned Abstract DAG
              |
              v
Type/Layout terminal planning
              |
              v
target execution + linker products
```

The compiler is not a second semantic model beside TTX. It does not rebuild
Types as ABI projections, infer ownership from pointer-keyed maps, or make
linker names the authority for resolved objects.

## Ownership Boundary

The Toolchain owns the long-lived ClassDB and installed ISA, target, and
language schemas. One Compiler borrows those immutable schemas and owns:

- the arena and every Abstract instance created during the build
- the semantic DAG and its import, alias, and contract-qualified edges
- Resolution routes and selected public routes
- concrete Generic instantiations and Layouts
- Callable bodies, Addresses, and unresolved-linkage objects
- diagnostics and Invalid objects
- target-independent execution facts
- target planners, linker state, and terminal products.

ISAs construct registered Abstract-derived objects inside this boundary. A
Library ISA can attach an execution-body contract to a Callable. Foreign can
attach an external Address. Shader can attach a stage body and terminal GPU
contract. The compiler consumes those interfaces without switching on the
producer ISA.

Only final artifacts escape: machine objects, archives, generated language
interfaces, SPIR-V modules, editor products, or another explicitly requested
terminal. Local object handles, Class indices, and compiler addresses do not.

## Semantic Contracts

The compiler operates on the same hierarchy as the language:

```text
Abstract
├── Invalid
├── Type
│   ├── Alias
│   ├── Generic
│   └── ISA-defined types
├── Callable
│   ├── Free
│   └── Self
└── Address
```

`Abstract` supplies named, contract-filtered resolution. `Type` supplies total
canonicalization and a concrete Layout. `Callable` supplies complete parameter
and result Layouts plus an Address query. `Invalid` carries a failed Route and
diagnostic cause.

There is no `Typed` marker and no universal Type interface containing dispatch,
ABI, package, or target behavior. A lower compiler that asks for Type need not
know whether the object is an Alias, Generic, shader scalar, or another
registered subtype. Specialized consumers can query those richer contracts
through ClassDB.

ClassDB ancestry replaces C++ RTTI. After `is<Contract>()` proves ancestry,
`as<Contract>()` returns a reference. Failed contract conversion returns a
Resolution containing Invalid rather than a nullable pointer. Foreign-language
objects use opaque handles and registered callbacks; C++ vtables never cross
the ABI.

## Resolution Routes

A Route records each named query step and the registered contract layer used to
take it. The same Abstract may be reachable through multiple import or Alias
routes. Canonicalizing a Type changes the object used for equivalence and
Layout, but it does not erase the authored Route.

Free and Self calls occupy real contract layers:

```text
Widget / Callable.Free / open
Widget / Callable.Self / open
```

`Widget -> open()` selects Free. `widget -> open()` canonicalizes the receiver
Type and selects Self. Both layers independently require unique child names.
The compiler does not reconstruct this distinction later as `.Type` and
`.Addressable` path strings.

Diagnostics and generated source normally use the route through which the
object was reached. Package publication chooses an explicit public route. It
does not select the lexicographically first Alias as an implicit identity.

## Callables And Execution

Callable owns the complete semantic call boundary:

| Contract | Meaning |
| -------- | ------- |
| `Callable` | parameter Layout, result Layout, and Address query |
| `Free` | call without an addressable receiver |
| `Self` | call whose receiver is parameter zero |
| `Address` | resolved local, external, interpreted, or runtime invocation endpoint |

The Library `self` shorthand constructs a Self callable and publishes the
receiver as the first ordinary parameter entry. `Self::get_parameters()` always
returns that complete Layout. Reflection, pack fitting, generated bindings,
register allocation, and call lowering consume the same truth. No compiler pass
prepends `self`, slices it away, or derives receiver semantics from its name.

A Free callable can be owned by a Type. Ownership is its Route; it does not
create a third “typed function” subtype.

An implementation enriches the Callable with an Address object. A declaration
whose address is not yet known returns an explicit unresolved Address or
Invalid. It never uses `nullptr` as a pseudo-address. Restored packages and
foreign runtimes publish the same Address contract as locally compiled bodies,
so callers do not branch on the producer.

Target-independent executable bodies may use dense immutable tables for blocks,
operations, operands, and SSA bindings. Those tables are an execution contract
attached to a Callable, not a replacement tree that owns Type identity,
publication routes, or package meaning.

## Type And Layout Lowering

Every terminal planner follows the same recursive algorithm:

```text
canonical = type.canonicalize()
layout = canonical.get_layout()

if layout has entries:
    lower every entry Type in layout order
else:
    lower canonical through the selected terminal Type contract
```

A concrete Layout carries kind, size, alignment, ordered entries, child Types,
and offsets. Alias delegates to the canonical Type. A bare Generic is valid as a
compile-time query receiver but not as a value; it creates or finds a concrete
Compiler-owned Type before lowering.

The semantic aggregate remains one value. Its terminal representation is an
ordered projection of its entries. Calls, returns, stack placement, register
classification, generated host declarations, and archive descriptions must use
that same projection.

A non-empty composite may not be collapsed to an invented scalar carrier merely
because a backend recognizes the outer Type name. Views, vectors, structs,
render contracts, and user aggregates are recursively deconstructed. The leaf
contract determines whether an empty-layout terminal is an integer, real,
opaque handle, zero-width value, resource, or another target-defined form.

Source `@abi` values and a global `Abi::Lowering` enum are not terminal Type
facts. They force every target and language to share one closed switch and lose
the recursive shape of composites. A target or ISA instead registers the
terminal contracts it understands and enriches the Type DAG with that meaning.

## ABI And Machine Planning

ABI planning consumes the recursively lowered terminal projection. It is a
target operation, not a source Type classification.

For every callable the planner must produce one immutable call plan describing:

- the ordered terminal components of each parameter and result
- register classes and physical locations
- stack offsets, alignment, and spill or hidden-result storage
- caller and callee movement rules
- unsupported terminal contracts before object publication.

Call emission and function entry consume the same plan. Generated foreign
interfaces use the same component projection. A backend must not independently
reclassify a Type at each of those sites.

The x86-64 System V planner recursively classifies complete aggregate layouts
into ABI components and applies the ABI's register, memory, rollback, and hidden
result rules. Mixed integer/SSE aggregates, values spanning several eightbytes,
and memory-class results are properties of that planner. Scalar-only shortcuts
are not a complete System V model.

Allocation occurs after the terminal call plan exists. The allocator deals in
the physical components and lifetimes described by that plan; it does not ask a
source attribute how many registers a semantic Type should consume. Assemblers
encode selected operations and physical operands without making semantic or ABI
decisions.

## Symbols And Publication

Public and internal symbols are reversible encodings of Routes. They are not
hashes of names, signatures, Type addresses, or package contents.

A readable route can be printed directly:

```text
Ttx1.Perimortem.Math.Vector.Callable.Free.from
```

A binary or linker-safe form may length-prefix and escape its segments, but it
must decode to the same route without a collision table. If incompatible ABI or
package versions coexist, the version is an explicit route segment. Runtime
dense indices may accelerate lookup inside one process, but they never become
durable identity.

The selected public Route is authored package surface. Aliases may deliberately
publish several language-facing projections of the same canonical Type or
Callable, but publication never invents an identity by sorting all reachable
paths. Internal implementation Routes remain distinct from public Routes and
may redirect to the same Address.

## Cross-Language ABI

The cross-language boundary combines ClassDB, Abstract handles, and
Layout-described calls. A language-neutral handle contains an opaque object
identity and a Class handle and always designates a real Abstract. Failure
returns the registered Invalid object.

The C ABI exposes explicitly versioned operations equivalent to:

```text
ttx1_class_register
ttx1_class_resolve
ttx1_class_is_a
ttx1_abstract_resolve
ttx1_abstract_get_class
ttx1_callable_invoke
```

These names describe the required surface, not permission to expose C++ object
layout. Registration supplies schema Route, parent schema, operation table,
construction/destruction callbacks, and reflected Layouts. A foreign extension
receives local handles from the loaded ClassDB and Compiler. It does not choose
serialized numeric class ids.

Generated C++ is one projection over this ABI. It may provide ergonomic
namespaces and wrappers, but raw calls use terminal carriers described by the
same Layout plan as the machine target. C++ class layout, compiler mangling,
vtable layout, and `dynamic_cast` are not part of the contract.

## Directory Ownership

- [`execution`](execution/) owns target-independent body operations and SSA
  tables attached to Callable objects.
- [`allocation`](allocation/) owns physical-location planning after terminal
  projection.
- [`target`](target/) owns ABI planning, terminal lowering, and target-specific
  interface projection.
- [`assembler`](assembler/) owns instruction encoding after semantic and ABI
  decisions are complete.
- [`engine.hpp`](engine.hpp) coordinates the selected target and linker inside
  the Compiler boundary.
- [`../linker`](../linker/) owns durable object records and archive formats.

Root compiler objects coordinate these owners. They do not become caches of
parallel Type, Export, linkage, or implementation projections.

## Invariants

- The Toolchain owns ClassDB; the Compiler owns every per-build Abstract and
  terminal product.
- Semantic resolution returns Abstract references and Invalid failures, never
  null pseudo-objects.
- Type is narrow: canonicalization and Layout. ISA and target meaning arrive as
  registered contracts.
- Free and Self are Callable subtypes; Self's receiver is parameter zero in the
  complete Layout.
- Routes preserve authored resolution and supply reversible durable names.
- No symbol, class, package, or callable identity depends on hashing.
- Every non-empty concrete Layout is recursively lowered before terminal leaf
  classification.
- One call plan is shared by caller, callee, generated interfaces, allocation,
  and object emission.
- Pointer-keyed side tables and ABI projections cannot become a second semantic
  authority.
- Assemblers encode decisions without making semantic or ABI decisions.
- Derived archives and interfaces are returned terminal products, not cached
  semantic state.
