# TTX

TTX is a host neutral source IR. It owns the authored lexical vocabulary, token
semantics, shared semantic identities, identity free Layouts, and the core data
model for abstract representations.

## Representation

TTX is a simple format for expressing dynamic abstract representations rather than
provide a concrete IR model. TTX is composed of various domain layers that can
provide both frontend and backend extensions.

This means TTX does not provide a canonical frontend language, intermediate
representation, or backend. Instead TTX provides the composability rules that cover
the creation and interoperability of the systems that generate those layers.

## Specification

TTX formally defines two systems: The TTX Bytecode format and the Abstract Data model.

TTX is interpreted as a fixed width bytecode stream with the code format
itself being simple enough:

* The decoder maps each code token to a Unsigned_8 semantic range (0 to 256)
* Each code token provides a cannonical source context mapping, this is not required
  to be source text and can be empty or `Invalid`.
* Semantic values `0` and `255` are reserved for `Terminal` and `Unknown` respectively.


### Lexical format

To ease progressive bring up of a TTX system a bootstrap `lexer` format is provide.
It currently uses `72` code points of the total `256`. As a template lexer this balances
a combination of optimizing semantic packing while leaving ample room for extensions.

The `Tokenizer` class provides the required semantic partitioning. It provides examples
of fixed keywords, operators, delimiters, comments, and literal forms as explicit Codes.
The TTX `Tokenizer` also provides some unique mapings such as variable length byte formats
and using casing (snake_case and PascalCase) to split text into seperate domains (Addressable
and Type).

Payload-bearing Tokens retain the source location required to recover their authored bytes
as their source context. The Code stream is meaningful only with the exact Lexer contract
that emitted it.

### Data format

The data model is built out of composable classes that must provide four morphisms:
* `resolve` - Returns the canonical node that holds the identity class in the current context.
  * Must always exist, even if it's just the class identity morphism.
* `resolve_context` - Returns a possibly non-canonical node based on a name in that class.
  * Must always resolve and be a stable identity as long as the graph has not been mutated.
* `implements` - Checks if two classes live in the same domain.
  * This does not represent inheritence as abstracts can exist in multiple domains.
  * The domain a class lives in can also be contextual.

Both `resolve` functions must terminate which means resolution loops are not support.
As an example `Alias` is an abstract class that has exactly one edge to another class.
That means by construction calling `resolve` on an `Alias` always results in a node in
a non-`Alias` class unless there is a loop in the graph.

The graph has no universal kind, class database, mutable registry, copied member records,
shadow Type graph, nullable semantic edges, or allocated path history.

## Practical TTX ssage

See [ttx_design.md](ttx_design.md) for the representation rationale and
[ttx_semantics.md](ttx_semantics.md) for the normative shared contracts.
