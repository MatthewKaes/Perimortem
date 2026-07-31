# TTX

TTX is a host neutral lexical and semantic substrate. It defines a compact
Token stream and the closed shared contracts used to exchange semantic
identities without copying them into private models.

TTX does not define a source transaction, concrete language, package,
filesystem, compiler, linker, runtime, or archive format. Those systems may
construct and consume TTX objects, but their policy remains outside this
project.

## Lexical layer

Every Token carries an eight bit `Lexical::Code` and coordinates into the
source view borrowed by its Tokenizer. `Terminal` is `0x00`, `Unknown` is
`0xFF`, and the remaining values belong to the exact Lexer and Lexicon
contract that assigned them.

Tokens represent decoded source spans. They are not fixed width language
instructions, and their Codes are not an independent serialized format. A
consumer must use the same Lexer contract and source bytes that produced the
stream.

`Lexical::Errors` is the retained rendering format for errors in authored
text. Every `Errors::Report` receives an explicit source name, source body, and
`Lexical::Span`. It is not the status channel for filesystem, archive,
repository, compiler, or runtime validation. Context free owners log their
local failure facts through `Diagnostics::Log` and return failure. The caller
that owns an authored request decides whether that failure becomes a source
diagnostic.

## Semantic layer

The identity bearing graph consists of `Abstract`, `Invalid`, `Alias`, `Type`,
`Value`, `Flag`, `Real`, `Signed`, `Unsigned`, `Addressable`, and `Callable`.

`Documentation`, `Layout`, `Reference`, and `Attribute` are identity free
supporting contracts and values. TTX also supplies common Layout
implementations.

Concrete languages may define expressions, constants, generic formulas,
mutability, invocation roles, executable bodies, and concrete scalar Types.
Those additions retain real TTX edges without becoming part of the shared TTX
model.

See [ttx_design.md](ttx_design.md) for the ownership rationale and
[ttx_semantics.md](ttx_semantics.md) for the normative contracts.
