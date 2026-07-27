# Library

Library owns the Library source Dialect and the reusable native CPU compilation
path. The complete folder builds as `//tetrodotoxin:library`.

Library consumes shared TTX Type, Layout, Generic, Addressable, and Callable
contracts. `Library::Language` owns Expression, Binding, Projection, Constant,
and the typed Constant domains required by Library grammar. These objects
retain real TTX edges and do not create a second Type or Layout graph.
Environment continues to own Namespace and Workspace.

## Language boundary

The Library parser owns:

1. Library declarations and definition grammar;
2. source order independent name discovery;
3. Library legality for Types, values, variables, Callables, and Bodies;
4. embedded Foreign CPU syntax; and
5. construction of the concrete Library Source root.

The parser constructs real TTX and `Library::Language` identities in the
Source Arena. It uses `Environment::Workspace` for stable scalar Types, Generic
formulas, materializations, and cross source Alias bindings. Completed
definitions are retained and published through `Environment::Namespace`.

The Type parser is Library grammar even though it returns a shared
`Ttx::Model::Type`. Another Dialect may earn a common parser only after both
grammar and result contract are proven identical.

Library language contracts compose TTX Type, Layout, Addressable, and Callable
identities directly. There is no copied TTX model and no conversion step
between competing Type graphs.

## Shared CPU compilation

Library Sources provide CPU executable inputs directly. Scene lifecycle
Callables remain on their Scene owners. App generated entry and lifecycle
driver facts remain on the App owner. The Library compiler lowers all selected
CPU executable facts after Environment Graph finalization.

```text
Library Callables and selected Static ----\
Scene lifecycle and helper Callables ------> Library compiler -> Linker inputs
App entry and lifecycle driver facts -----/
```

App and Scene are never converted into a Library Source, Namespace, or shadow
executable graph. Their parsers do not emit generated Library text, and the
compiler never reopens their Tokens.

The exact durable Body representation remains unresolved. This compiler
placement does not authorize a placeholder IR or copied Type graph.

## Compiler boundary

Library compilation begins only after Environment has finalized and sealed
every selected Callable, signature, Body fact, Type, Layout, and dependency
edge. It owns:

1. CPU target and ABI planning;
2. representation sizes, alignments, offsets, and carriers;
3. calling convention and register allocation decisions;
4. lowering selected completed executable facts;
5. native instruction and relocation emission; and
6. diagnostics for target decisions.

It does not own the Environment graph, Package construction, source loading,
runtime objects, App or Scene policy, Shader lowering, or final object and
archive assembly.

Library compiler output is source independent Linker input. Linker owns
objects, symbols, relocations, ELF encoding, and System V archive construction.

## Semantic input

Compilation selects real semantic identities from a finalized Environment
Graph. Aliases resolve through `Abstract::resolve()`. Consumers use
`visit<Catagory>()` to prove the required Type, Callable, Layout, or producing
Dialect contract.

The compiler does not infer meaning from C++ type names, formatted TTX names,
structural coincidence, a central Kind, or a pointer keyed implementation
table. It does not build an ABI shadow Type graph.

A source backed Graph keeps every owning `Tetrodotoxin::Language::Source`
alive for the complete compile. A source free Graph restored by Environment
owns equivalent facts without parser state. Both routes must produce the same
target decisions.

## Target lowering

```text
resolve Abstract
-> prove the required semantic contract
-> derive scalar or aggregate representation
-> plan ABI placement
-> emit native instructions and Linker records
```

The target derives storage size and alignment. Aggregate offsets, register
carriers, stack placement, and wire policy remain target decisions.

Callable publishes complete parameter and result Layouts. Static invocation
has no receiver. Self invocation includes its receiver as parameter zero.
Caller, callee, generated interfaces, register allocation, and object emission
must share one call plan.

Assemblers encode decisions already made by semantic and target owners. They
do not decide Type identity, aggregate shape, calling convention, App policy,
or Scene lifecycle roles.

## Current surface

The Library target currently owns `Library::Assembler::x86_64` and the Library
Type parser. The assembler unit tests prove instruction byte encodings and
relocation slot shape.

The complete Library Source parser, semantic compiler transaction, finalized
Environment Graph, CPU target planner, Graph to Linker lowering, and App or
Scene delegation do not exist yet.

Future code enters through the complete `library/**/*.cpp` and
`library/**/*.hpp` target. Do not recreate a top level Compiler target, split
the assembler into another artifact, or retain forwarding model headers.
