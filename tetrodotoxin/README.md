# Tetrodotoxin

Tetrodotoxin is the concrete host for TTX. TTX supplies lexical bytecode and
the shared target independent semantic model. Tetrodotoxin owns concrete source
Dialects, Environment construction, generators, filesystem policy, and durable
Package products.

## Component map

1. [`language`](language/) owns one Source lifetime, the universal source
   envelope, static parser dispatch, and parser fragments shared by concrete
   Dialects.
2. [`environment`](environment/) owns the semantic Workspace, Namespace, and
   eventual finalized Graph.
3. [`library`](library/) owns Library grammar and the reusable CPU compiler and
   assembler.
4. [`package`](package/) owns authored Package grammar, confined filesystem
   Workspace, Distribution, Reader, and Writer.
5. [`app`](app/) owns startup profiles, lifecycle policy, and generated
   platform entry semantics.
6. [`scene`](scene/) owns managed state, signals, render facts, and Scene
   lifecycle roles.
7. Top level `render`, `shader`, and `foreign` folders own their concrete
   grammar, legality, and specialized tooling.
8. [`linker`](linker/) owns source independent objects, symbols, relocations,
   target encoding, and native archive construction.

Path, namespace, and Bazel target describe the same owner. Every top level
target owns the complete `folder/**/*.cpp` and `folder/**/*.hpp` tree.

## Source transaction

```text
authored text and diagnostic path
-> Language::Source::parse
-> one owned text copy, Tokenizer, Cursor, and Arena
-> opening Documentation and exact Dialect name
-> borrowed parser map lookup
-> selected static body parser
-> one concrete Abstract root
-> completed Language::Source owner
```

`Language::Source` is the lifetime root for everything derived from one
authored stream. It is not an Abstract. Its selected concrete root is the
semantic identity.

The parser map associates exact Dialect names with static function pointers and
is borrowed only for construction. There is no Dialect base class, Frontend
object, mutable parser registry, separate Container, second tokenization pass,
or retained parser bookmark.

A parse succeeds only when the selected parser returns a concrete root,
consumes the complete source body, and adds no diagnostic. Failure destroys the
candidate owner, so an incomplete Source cannot escape.

## Semantic construction

```text
Environment::Workspace
  supplies stable scalar Types, Generic formulas, materializations, and aliases

Language::Source owners
  retain each source Arena, Tokens, and concrete semantic root

Environment::Namespace
  retains roots, publishes exports, and validates sealing

future Environment::Graph
  connects completed roots and resolved Package inputs
```

Environment does not replace Source ownership. A living Source always owns the
Arena that backs its root. Environment supplies cross source semantic identity
and the immutable consumer boundary.

TTX Type, Layouts, Addressable, Callable, and shared value domains remain common
catagories. `Library::Language` owns Expression, Binding, Projection, Constant,
and typed Constant domains while retaining real TTX edges.

Scene, App, Shader, Foreign, and other producers do not clone the TTX model beneath
a concrete Dialect and can borrow `Library::Language` as a subdialect when required.

## Dependency direction

The foundational direction follows meaning:

```text
|-----Tetrodotoxin-----||--TTX--||--Runtime--|
Library  ->  Language  ->  TTX  ->  Perimortem
```

Other languages should never `dispatch` to `Library` directly. Instead they should
use `loan words` from the `Library::Language::Dialect` in the contexts that are
appropriate for them.

## Terminals

Outside of the `Library::Language` dialect Library compilation consumes completed
CPU facts retained by their real Library, Scene, or App owners and lowers them to
CPU centeric terminals. It never converts another Dialect into a Library however
it does support layout negotions with them to ensure "ABI" and calling conventions.
Key consumers such as `App` use Library layouts to negotiate startup and lifecycle policy
while consumers like `Shader` use Library to negotiate CPU <-> GPU terminals.

Linker currently owns the unified terminal platform artifacts to centeralize the logic.
However the Assembler should live in Library long term with the executable formats moving
to `App` and the archiving logic moving to `Package`.

See [tetrodotoxin_design.md](tetrodotoxin_design.md) for the complete ownership
and transaction contract.
