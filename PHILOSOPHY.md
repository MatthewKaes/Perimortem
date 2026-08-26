# Tetrodotoxin Philosophy

> **The common layer is meaning, not representation.**

Most software already contains several languages, even when the project only
calls one of them code. A Package source describes composition. A scene
describes lifecycle and relationships. A shader describes GPU behavior. A build
file describes how the product comes together. Each format carries rules that
matter to the finished system.

Those smaller languages usually face an awkward choice. They can remain data
files whose meaning is reimplemented by every editor and build tool, or they can
grow into isolated compilers with their own package model, diagnostics, and idea
of the program. One choice hides the language. The other fragments the
toolchain.

Tetrodotoxin is built around a third possibility: several languages keep the
meaning that makes them useful while taking part in one semantic Workspace and
one complete toolchain.

## Specialization without fragmentation

A purpose-built language lets a domain speak in the concepts its users already
understand. Scenes have signals and lifecycle. Render contracts have stages and
resources. Packages have dependencies and durable identities. Expressing those
ideas directly makes source smaller, clearer, and easier to validate.

The benefit disappears when every language becomes an island. A rename in a
Package may stop matching a scene. A Shader input may drift away from the CPU
value that supplies it. An editor may recognize the spelling while the compiler
resolves a different declaration. Every boundary becomes another place where
people have to keep two descriptions synchronized.

Tetrodotoxin gives each domain a **Dialect** with its own grammar and semantic
objects. Those objects meet inside the same Workspace, where Package routes,
editor navigation, cross-language references, and compilation all see the same
identities. Specialization changes how a domain expresses itself without
splitting the product into unrelated worlds.

## Federation rather than normalization

A common intermediate representation is a natural way to connect several
languages. Every frontend translates into one node vocabulary, and every
consumer reads the result. That works beautifully when the shared destination
is the point, as it is for progressive lowering into machine code.

It is a poorer fit for domains that still need their original meaning. A Shader
Stage and a Library Function may both be callable, but stage interfaces,
visibility, invocation, and execution belong to different languages. Flattening
both into one generic function either loses those distinctions or carries them
as metadata that only one consumer understands. Over time the common IR becomes
a second, less precise version of every source language.

Tetrodotoxin federates those models instead. TTX supplies a compact vocabulary
for the questions that cross domains: identity, resolution, Types, Packs,
Layouts, Addressables, Callables, documentation, and Interfaces. The original
Shader Stage or Library Function answers the question on its own identity.

This is **raising by participation**. A language brings shared meaning into the
Workspace without translating away the meaning it owns. Lowering remains
available later, once a concrete product has chosen a representation domain.

> **Pull upward every fact that is target neutral and genuinely shared, while
> leaving richer meaning with its concrete owner.**

## Identity is the integration boundary

Consider a Library Type reached through a Package Alias. Navigation needs its
source definition. Layout fitting needs its value shape. A compiler needs its
operations and physical requirements. An Archive needs enough durable meaning
to construct an equivalent Type in a fresh Workspace.

Copying that Type into an editor symbol, compiler node, Package record, and
serialization model creates four plausible answers to the same question.
Keeping them synchronized becomes a hidden part of every feature.

Tetrodotoxin carries the real Type identity through those relationships instead.
Each consumer asks it for the contract it understands. The Type remains free to
hold richer Library meaning that Package, Puffer, and a Terminal have no reason
to copy.

One graph does not mean one universal node shape. It means that every semantic
fact has an owner and every relationship reaches that owner. The common layer is
a conversation between objects rather than a warehouse of converted objects.

## Uncertainty is part of meaning

Source code spends much of its life incomplete. A person can pause after an
access operator, rename one side of a relationship before the other, or leave a
Type unresolved while sketching a function:

```ttx
state output := Dynamic::Bytes -> copy(prefix);
output->
```

The declaration of `output` still exists. Its spelling, location,
documentation, visibility, and position in the function remain useful even if
the current expression cannot establish its final Type.

Tetrodotoxin distinguishes that state from absence. An explicit unknown or
`Invalid` result means that the graph recognized the question but cannot settle
it with the meaning currently available. A missing object means there was
nothing there to answer. That distinction lets completion, hover, navigation,
and diagnostics expose partial understanding instead of choosing between a
perfect answer and silence.

A finished native program or GPU module has a different threshold. Terminal
production begins from completed meaning. The live Workspace can remain
informative while source is changing without pretending that an incomplete
program is ready to ship.

## Abstractions rise through evidence

Tetrodotoxin's shared vocabulary grows out of real cooperation between
independent domains. Type, Layout, Pack, Addressable, Callable, and Interface
exist because several languages or consumers ask those semantic questions.

Other ideas remain with the language that gives them meaning. Library owns its
numeric families, default construction, visibility, and Static and Self
receiver roles. Shader owns stages and GPU execution rules. Scene owns signals
and lifecycle. Their implementations may look similar in places without
describing the same concept.

This creates a useful direction of travel. A new capability begins near the
domain that understands it, where its semantics can become concrete. Another
independent use reveals which part is genuinely common. That shared part can
then rise without pulling the original domain model along with it.

The result is a small common language shaped by evidence rather than a framework
that predicts every future Dialect in advance.

## Interfaces preserve semantic compatibility

Layouts and Packs describe data flow. They can prove that values have compatible
shape and show how one flow fits another. In doing so they deliberately leave
richer domain meaning behind.

That is not enough for every relationship. A Render contract and a Shader
implementation may consume identical values while disagreeing about stages,
resources, or behavior. Equal layout proves that the data can fit. It does not
prove that the Shader implements the Render contract.

An **Interface** carries that higher-order relationship. Two semantic objects
can establish interchangeable behavior through a shared contract without
either Dialect learning the other's concrete model. Layout negotiation can
contribute evidence to the relationship while the Interface preserves what the
relationship means.

This combines structural negotiation like Zig with explicit runtime erasure
like Rust. Compatibility can be discovered while values remain concrete.
Runtime erasure enters only when a program needs to store or invoke a value
through that shared contract. The semantic relationship exists independently
from its eventual carrier.

## Representation begins at the Terminal boundary

A completed Workspace can feed several very different products. Library meaning
can become native code. Shader meaning can become SPIR-V embedded in that native
program. Package meaning can become an Archive that reconstructs the semantic
island without source. Formatting can produce canonical text for people.

Tetrodotoxin calls these output producers **Terminals** because they mark the
point where a product leaves the live graph. Dialects compose the languages a
toolchain understands. Terminals compose the products it can create. The two
directions meet at completed meaning.

This boundary leaves representation work in the domains built for it. LLVM IR
can remain an intermediate representation inside LLVM while acting as a
Terminal product relative to the Workspace. SPIR-V can follow its own lowering
and validation rules. Calling conventions, registers, object formats, and
Vulkan setup can change with the target without becoming facts every Dialect has
to understand.

Generated headers, ABI symbols, Archives, GPU modules, and executables are views
of the program for their next consumers. They come from the semantic graph, but
none replaces it as the source of meaning.

## Why Tetrodotoxin exists

Most language platforms specialize in one direction. One language may target
many machines, or many frontends may normalize into one compiler IR.
Tetrodotoxin is interested in the larger product around them: several semantic
domains, several purpose-built languages, one linked understanding, and many
independent outputs.

That shape makes a new DSL more than a parser attached to the edge of an
application. It can join the same Package graph, editor, build, compiler,
linker, and runtime as the rest of the system. It can reuse meaning from another
Dialect without becoming that Dialect, and it can contribute meaning to products
that were not designed into its grammar.

Tetrodotoxin exists to make that kind of language design practical. A project
gains the vocabulary its domains deserve without asking the people building it
to assemble and reconcile another toolchain for every new idea.
