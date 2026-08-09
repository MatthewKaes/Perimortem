# TTX Design

TTX is the shared boundary between lexical producers, concrete languages, and
semantic consumers. It owns facts that remain meaningful without knowing which
package, compiler, target, or runtime will eventually consume them.

## One graph

Every semantic identity begins with `Abstract`. Narrower categories add only the
queries needed to exchange that identity with another domain:

```text
Abstract
├── Invalid
├── Alias
├── Type
│   └── Value
│       ├── Flag
│       ├── Real
│       ├── Signed
│       └── Unsigned
├── Addressable
└── Callable
```

An owner retains the real identity through a borrowed `Reference`. Names,
documentation, and category-specific edges remain on their semantic owners, so
tooling and lowering observe the same graph rather than synchronized copies.
Alias exposes its immediate borrowed target when graph construction needs that
exact edge; ordinary `resolve()` still follows represented identity.

`Documentation`, `Layout`, `Reference`, and `Attribute` are supporting values.
They describe or connect identities without acquiring independent semantic
identity.

## Contextual resolution

`resolve()` follows represented identity. `resolve_context(route)` asks the
receiving Abstract to interpret a borrowed route in its own domain. A result may
itself answer another contextual query, so traversal can cross any sequence of
Abstract identities without requiring the intermediate and terminal categories
to match.

The caller owns the expected contract. It proves the returned identity through
`is<Category>()` or `visit<Category>()`; contextual resolution does not infer a
Type, Addressable, Callable, or another category from route spelling. Likewise,
a Layout exposes real Abstract entries without assigning them one universal
member or access meaning.

TTX therefore defines no shared member table, collision domain, or source
operator semantics. A concrete language assigns grammar and result contracts to
lexical Codes while using the TTX queries and categories it requires. Target
realization of a selected identity remains a later consumer decision.

## Progressive construction

A graph owner may reserve a stable identity before all of its edges are ready.
An incomplete total query returns the shared `Invalid` object. Later work may
make an unanswered query valid, but an identity that was already returned does
not change.

Parser rejection, a failed Layout fit, and other operation failures use their
owners' result contracts rather than becoming semantic identities. `Invalid` is
reserved for queries whose contract always returns an Abstract.

## Layouts as value shape

A Layout retains an ordered view of real Abstracts and answers whether one view
fits another. Fitting is directional: a source Layout supplies the values
required by a target Layout.

- `Fluid` compares entries in order.
- `Named` compares uniquely named entries by name and represented identity.
- `Ranged` repeats one entry across a fixed interval.
- `Composite` preserves two complete child Layouts.

A consumer may select a real entry from a Layout and then prove whichever TTX
category its own operation requires. `Named` supplies name based fitting without
becoming a universal lookup, member, or access interface.

Layouts do not contain target offsets, alignments, registers, address spaces, or
calling convention carriers. A lowering terminal derives those facts after the
semantic Layout is known.

## Lexical ranges

Tokens describe decoded source spans. A Tokenizer terminates its stream with one
zero length `Terminal` Token. A Cursor provides bounded relative observation and
speculative branches; a branch changes its parent position only when joined.

An `Anchor` combines the complete Span relevant to a fact with the Token a
diagnostic should emphasize. Synthetic facts omit the Anchor because they have
no authored coordinate.

## Concrete language boundary

TTX supplies the vocabulary shared by languages. A concrete language decides
which Types exist, how names are published, which writes are legal, how
Callables are selected and invoked, and how expressions evaluate.

Target lowering may add offsets, pointer representations, ABI carriers, and
machine symbols. Runtime systems may add frames, managed cells, collectors, and
scheduling state. Those facts consume the TTX graph without becoming TTX
semantic categories.

The normative contracts are defined in [ttx_semantics.md](ttx_semantics.md).
