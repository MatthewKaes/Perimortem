# TTX

TTX is the repository's host-neutral source IR. It owns the authored lexical
vocabulary, token bytecode, shared semantic identities, identity-free Layouts,
and the common executable Body representation.

TTX deliberately does not own source-file lifetime, package resolution,
filesystem access, concrete Dialects, runtime execution, target lowering,
linking, or archive formats. Those systems consume TTX; they do not extend its
ownership by being documented here.

## Representation

```text
authored bytes
-> Lexical::Tokenizer
-> ordered Token values with concrete Lexical::Code values
-> host-selected parsing and evaluation
-> one graph of real Abstract identities
   + identity-free Layout values
   + identity-free executable Body values
-> host-owned runtime, target, tool, or durable product
```

The Tokenizer performs the first semantic partition. Casing, fixed keywords,
operators, delimiters, comments, and literal forms become explicit Codes.
Payload-bearing Tokens retain the source location required to recover their
authored bytes. The Code stream is meaningful only with the exact Lexer
contract that emitted it.

A consumer decides how many Tokens form one instruction and which instructions
are legal. TTX supplies the common facts that consumer may construct:

- `Abstract` for semantic identity and contextual resolution;
- `Invalid` for failed semantic queries;
- `Reference<T>` for non-null semantic edges;
- `Documentation` for stable ordered comment lines;
- `Layout` for identity-free ordered shape and directional fitting;
- `Type` and its narrow Terminal and Generic contracts;
- `Expression` and immutable `Constant` values;
- `Addressable`, `Writable`, `Callable`, `Static`, and `Self`;
- `Exports` for an enumerable public definition surface; and
- `Body` for immutable executable blocks, values, and operations.

The graph has no universal Kind, class database, mutable Dialect registry,
copied member records, shadow Type graph, nullable semantic edges, or allocated
path history.

## Ownership boundary

TTX reserves lexical forms such as the `dialect` marker, definition modifiers,
attributes, calls, layouts, packs, and literal prefixes. A reserved Code does
not give TTX ownership of every meaning a host may attach to it.

For example, TTX can identify an envelope shaped like `dialect : Name;` without
defining the accepted names. It can identify an embedded resource token without
defining a filesystem root. It can represent a Callable Body without defining
which runtime executes it. It can expose Type and Layout facts without defining
SPIR-V, a native ABI, or an archive schema.

A concrete consumer owns every omitted policy. Sharing a repository with TTX
does not make that consumer or its behavior part of the TTX contract.

## Repository map

- [`lexical`](lexical/) owns Codes, Tokens, Tokenizer, Cursor, Lexicon, and
  lexical diagnostics.
- [`concept`](concept/) owns Abstract, Documentation, Invalid, Layout, and
  non-null Reference.
- [`model`](model/) owns the shared semantic identities and identity-free values
  built from those concepts.

The active libraries are `//ttx:lexical`, `//ttx:concept`, `//ttx:model`, and
the convenience target `//ttx:ttx`.

See [ttx_design.md](ttx_design.md) for the representation rationale and
[ttx_semantics.md](ttx_semantics.md) for the normative shared contracts.
