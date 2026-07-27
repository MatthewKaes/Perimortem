# Tetrodotoxin Language

`tetrodotoxin/language` defines the source transaction and the syntax shared by
every top level Tetrodotoxin source dialect. The complete folder builds as
`//tetrodotoxin:language`.

Language depends only on TTX and Perimortem. It owns source lifetime and
dispatch, but no concrete dialect semantics, filesystem capability, compiler
target, finalized graph, or durable package.

## Source

`Language::Source` is one concrete lifetime transaction for one source
document. It is not a TTX Abstract, Type, Namespace, or generic semantic root.
It owns:

1. an arena copy of the diagnostic path and source text;
2. one Tokenizer over that owned text;
3. the Arena used by Tokens, Documentation, and every source local result; and
4. the selected concrete TTX Abstract root produced by the source dialect.

The selected Abstract may be a Package Source, Library Source, Scene Source, or
another concrete owner. That object owns its definitions, resolution rules,
state, and policy. Language assigns none of those facts a universal shape.

Destroying `Language::Source` destroys the complete downstream memory domain.
The selected root cannot outlive or independently invalidate its Arena.

## Static parser dispatch

`Language::Source::parse` takes a borrowed
`Perimortem::Memory::Dynamic::Map` from exact dialect names to static parser
function pointers. The compiled toolchain owns that map and keeps it stable for
the call. Source uses it for one lookup and never retains it.

Parser entry points are static functions. They do not inherit a common parser
class, construct parser objects, register through TTX Abstract resolution, or
carry state between calls. A selected function receives the same forward only
Cursor and opening Documentation, then constructs its concrete Abstract root in
the Source Arena.

The map is the complete parser family supplied by the toolchain. An unknown
dialect name produces a source diagnostic. Language contains no parser object
hierarchy and no forwarding dispatch owner.

## Universal Source parser

`Language::Source::parse` owns the universal source envelope:

```text
zero or more opening comment lines
dialect : Type;
concrete dialect body
end of document
```

It performs one forward only transaction:

```text
parse opening Documentation greedily
-> parse the exact dialect name
-> find its static parser function
-> give the same Cursor and Documentation to that function
-> require one complete concrete Abstract root
-> commit the owning Language::Source
```

Missing opening comments produce the shared empty Documentation. A concrete
dialect may reject empty Documentation when its own source contract requires an
authored document comment.

The transaction copies text before tokenization and never tokenizes again. It
stores no body bookmark and builds no intermediate syntax tree. A parser that
returns no root without a diagnostic receives a source diagnostic. A parser
that emits any diagnostic cannot commit its candidate Source.

Only a completed owning Source handle leaves `Source::parse`. The active
diagnostic borrow ends before a failed candidate is destroyed.

## Shared parser fragments

Shared parsers consume one forward only `Ttx::Lexical::Cursor` transaction.
They use Token Codes for grammar, construct complete owner shaped results in
the supplied arena, and return an explicit optional result when parser control
flow may fail.

The current reusable fragment is `Parser::Comment`. It consumes consecutive
comment lines into one compact Documentation value while preserving empty
authored lines. It returns None without advancing when no comment begins at the
current Cursor.

Type, Layout, Callable, and Generic are shared TTX semantic contracts rather
than Language Source mechanics. Namespace and Workspace belong to
Tetrodotoxin Environment. Concrete Type and executable grammar belongs to its
Dialect owner, including the Library Type parser in `//tetrodotoxin:library`.

## Ownership boundary

Concrete grammar belongs with its concrete dialect owner. That owner may depend
on Language without moving semantic policy into this common layer.

A shared parser belongs here only when at least two concrete dialects share
both its grammar and its result contract. Similar spelling alone is
insufficient.

## Current surface

The owning Source transaction, exact static parser lookup, universal source
envelope, and Comment parser form the common surface. Concrete dialect
evaluation remains outside this layer. A parser map entry proves only that the
compiled toolchain supplied that static function; it does not prove the
concrete dialect semantics or any terminal product.
