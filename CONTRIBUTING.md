# Contributing to Tetrodotoxin

Tetrodotoxin is growing into a platform where several purpose-built languages
can take part in one program and one toolchain. There is room to contribute
across the language model, editor experience, compilers, Packages, native
runtime, documentation, and validation.

Start with the [project philosophy](PHILOSOPHY.md). It explains why
Tetrodotoxin preserves concrete semantic owners, shares meaning through TTX, and
treats generated outputs as Terminal products. The
[Tetrodotoxin overview](tetrodotoxin/README.md) maps the larger platform, while
the [TTX overview](ttx/README.md) introduces its shared graph vocabulary.

When you are changing the language itself, the detailed contract lives in
[TTX semantics](ttx/ttx_semantics.md). [TTX design](ttx/ttx_design.md) and
[Tetrodotoxin design](tetrodotoxin/tetrodotoxin_design.md) explain the tradeoffs
behind it. These documents describe the system we are building. Roadmaps tell
us how far along we are, but they do not redefine the destination.

## Begin with the meaning

Before choosing a class or file, write down the semantic question you are trying
to answer and the object that should own the answer. Most changes have a short
ownership story:

* A Dialect owns a new piece of domain meaning.
* TTX exposes a contract already shared by independent domains.
* Workspace connects or retains existing identities.
* A Terminal derives a product from completed meaning.
* Puffer coordinates those owners for a user request.

If the story gives the same fact to two objects, stop there. It is almost always
cheaper to correct the ownership than to keep two tables, trees, or forwarding
layers synchronized for the rest of the project's life.

Do not lift an abstraction just because two implementations happen to look
similar. Shared concepts earn their place through independent semantic use.
The reverse is also worth watching: if a Dialect has to copy a large amount of
another language's machinery, we may be missing a composition boundary. Reuse
the real owner when that keeps both languages honest. Do not turn the reused
language into a universal IR simply because it is already available.

## Review the shape before filling it in

Certain implementation shapes are not wrong, but they have a habit of hiding
ownership mistakes:

* A broad `Scope`, `Context`, `Descriptor`, `Builder`, `Registry`, or
  `Manager` may be collecting responsibilities that belong to several owners.
* A group of optional callbacks may actually describe several independent
  capabilities.
* A switch over every concrete semantic kind may be taking behavior away from
  the objects that understand it.
* A discovery table may be duplicating an existing resolution channel.
* Parser state kept for a later semantic pass may mean that the phase boundary
  is in the wrong place.
* Target configuration inside a Dialect may mean that lowering has crossed the
  Terminal boundary.
* Platform headers in a public API may mean that the native boundary is leaking.
* An adapter between two new APIs may be preserving a bad abstraction instead
  of correcting it.

Sometimes one of these shapes really is the cleanest answer. When it is, make
the reason easy to find. Its public contract should explain what it owns, which
invariant it protects, and why an existing graph query cannot do the job.

## Preserve progressive meaning

An unknown answer is still an answer. If we know that a declaration exists, we
should keep its name, location, documentation, and established relationships
even when its Type or initializer cannot be resolved yet. `Unknown` tells a
caller that the question was recognized and remains unsettled. The `None`
Constant instead proves completed absence.

This matters most while someone is editing. Completion, hover, navigation,
diagnostics, and semantic highlighting should ask the actual graph and its
owners for the strongest answer available. A private Puffer symbol table cannot
recover context that the semantic owner was asked to throw away, and it will
eventually disagree with the graph it copied.

Terminal production has a firmer boundary. Native code, GPU modules, Archives,
and executables are produced from a completed semantic island unless the
Terminal explicitly defines a useful partial product.

## Carry a change through the real boundary

Try to follow a change far enough to prove that its ownership works. A useful
slice may cross a Dialect, Workspace coordination, a Terminal, and an
independent consumer. Small builds between those steps are good at finding
mechanical mistakes, but polishing every intermediate seam can lock in the
wrong design before the whole path is visible.

Once the complete path exists, step back and read it as one system:

* Does every semantic fact have one owner?
* Do dependencies point toward that owner?
* Does each tool and Terminal query the real graph rather than a copy?
* Did the new path replace an older one, or merely settle beside it?
* Can you explain the design without a list of implementation exceptions?
* Does the final proof exercise a real consumer?

When the right contract is not obvious, build a reference implementation first.
The Perimortem folder is the canonical place to stage designs in a familiar
native language before moving them into TTX. A reference implementation gives
us behavior to compare against and exposes missing capabilities, but it does not
automatically define the final architecture.

## Use agents as tools and write for people

Coding agents are useful for exploration, mechanical changes, and implementation
work. They are still tools. Humans choose the architecture, review the result,
and live with the code after the context that produced it is gone. Treat
generated code and prose as a draft that owes the project the same clarity and
evidence as any other contribution.

Public documentation should begin with what becomes possible and why it matters.
Introduce Dialects, Monographs, Interfaces, and Terminals after the reader
understands the problem they solve. Tetrodotoxin is flexible enough that a
concrete example often explains the composition better than a page of formal
vocabulary.

A list of accurate implementation facts can still make lifeless documentation.
Give the reader a reason to care, show them the system in motion, and then name
the pieces. The main project pages describe the intended platform rather than
the current editing session. Temporary limitations and review checkpoints
belong in development tracking. Detailed contracts belong beside the subsystem
that owns them.

### Keep architectural context local

Comments should explain decisions that readable code cannot. Record ownership,
constraints, stage order, failure behavior, or a tradeoff that would otherwise
have to be rediscovered. The goal is to give the next person enough context to
change the code confidently.

A comment should not narrate the next statement, recount a patch, or address an
automation tool as if it were the maintainer. Write to the person reading the
code. Explain why the shape exists and what would become untrue if it changed.

Terminology deserves the same care. A flexible toolchain needs names that remain
useful as it grows. Prefer semantic names that still make sense when another
Dialect or Terminal arrives over names tied to the first implementation.

## Validate the claim being made

A successful build tells us that the pieces agree mechanically. It does not
prove that the semantic contract works. Focused tests should cover distinct
behavior and failure boundaries, while a complete product needs a real consumer
or independent validator.

The fewer prospective consumers a feature has, the closer it should stay to the
edge of the graph. We can raise it once another project demonstrates shared
meaning. This keeps one experimental need from becoming a permanent promise
that every Dialect has to understand.

When exact output bytes are part of the contract, inspect those bytes as well as
the graph that produced them. When behavior is the contract, exercise that
behavior through the production path rather than substituting an internal dump.

Use the repository entry points for builds and tests:

```sh
bazel build //...
bazel run //validation:unit_tests --config=debug
```

## Presenting a contribution

Tell reviewers what changed and why it belongs where it landed:

* The user or language capability the change provides
* The semantic owner and why that owner is the right one
* Which old path or duplicated model the change removes
* The independent behavior used to validate the result
* Any unresolved architectural question that should remain visible

The best contributions give Tetrodotoxin a clearer story while expanding what
it can do. If a change is difficult to explain without walking through several
parallel models, that difficulty is valuable design feedback. Do not hide it
behind more glue.

Even with modern coding tools, code is debt rather than an asset. Tools can make
code cheap to produce, but they cannot make it free to understand, test,
compile, or maintain. A feature earns its implementation when the capability it
adds is worth the surface the project will carry afterward.
