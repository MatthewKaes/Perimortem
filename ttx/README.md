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

The Tokenizer appends one zero length Terminal at the source end boundary.
`Cursor::peek` accepts a signed relative offset and returns an empty Token when
either stream boundary would be crossed. A consumer can pair its opening Token
with `peek(-1)` to describe an operation that ended immediately before the
current Token without synthesizing a boundary Token.

`Lexical::Anchor` pairs the complete Span relevant to a fact with the
independent Token a diagnostic should emphasize. Construction from a Span
defaults that focus to its opening Token. An empty or external Token remains a
valid request for source context. Errors emits carets only when the complete
Token lies inside both the Span and retained source. A synthetic semantic fact
omits the Anchor instead of fabricating source coordinates.

`Cursor::branch` starts a speculative transaction at the current position over
the same immutable Token stream. Its explicit Errors overload keeps provisional
diagnostics private. Only `Cursor::join` publishes the branch position, and a
consumer joins only after its complete parse transaction succeeds.

Tokens represent decoded source spans. They are not fixed width language
instructions, and their Codes are not an independent serialized format. A
consumer must use the same Lexer contract and source bytes that produced the
stream.

`Lexical::Errors` is the retained rendering format for errors in authored
text. Every `Errors::Report` receives an explicit source name, source body, and
`Lexical::Anchor`. Its Span selects the excerpt. When the complete Token lies
inside both that Span and the retained source, it selects the diagnostic
coordinate and complete caret width. It is not the status channel for
filesystem, archive,
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
