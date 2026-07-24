# Tetrodotoxin Parser

`tetrodotoxin/parser` owns deterministic consumption of TTX token bytecode for
the concrete source forms Tetrodotoxin accepts. It turns one forward-only
`Ttx::Lexical::Cursor` transaction into owner-shaped values and semantic facts.

The parser does not own source bytes, filesystem access, repository search,
package loading, runtime execution, target lowering, linking, or archive
formats.

## Transaction contract

Source lifetime and the Tokenizer belong to `Tetrodotoxin::Model::Source`.
Parsing borrows its Cursor, source projection, Arena, and Errors.

Every parser follows these rules:

- consume left to right without backtracking;
- use Token Codes to choose grammar rather than reclassifying text;
- report the required class and the actual class when a Token does not match;
- return `Utility::None` for parser failure;
- return references or owner-shaped values for success;
- use `Core::Static::Union` when a result has closed alternatives;
- never use a pointer as absence, failure, or delayed semantic state;
- never take an output parameter to smuggle a second result from a transaction;
- never retain a Cursor position, token index, or token range as unfinished
  semantic meaning; and
- recover only at an explicit grammar sequence point.

`Cursor::require()` consumes the required Token and returns a Token whose
validity can be tested directly. A caller that does not need the Token may use
that result as the condition. Parser code does not reproduce the Cursor's
validity or diagnostic state.

An authored form is consumed once. If later completion is required, the next
phase walks the semantic owners produced by parsing. It does not reopen the
Tokenizer or replay a Cursor.

## Shared grammar

The shared parser surface is restored one owner-shaped family at a time:

| Owner | Responsibility | Status |
| ----- | -------------- | ------ |
| `Comment` | consume ordered comment Tokens into one valid Documentation value | implemented |
| `Builtins` | immutable fast lookup for concrete scalar Types | implemented |
| `Type` | progressive Abstract resolution and Generic argument materialization | implemented |
| Package document | opening Documentation, exact envelope, resolutions, and member routes | implemented |
| Definitions | modifier/name/continuation consumption and direct semantic handoff | not implemented |
| Attributes | key plus optional scalar value | not implemented |
| Layouts and packs | expected shape and produced value flow | not implemented |
| Functions | Callable signature and direct body consumption | not implemented |
| Expressions and statements | one-pass production of semantic values and Body facts | not implemented |

`Type::parse()` first resolves a concrete builtin or queries the caller's real
Abstract context. Each `::` segment is resolved by the currently selected
Abstract. Generic arguments are consumed according to the Generic's immutable
parameterization and supplied to the transaction-owned
`Generic::Materializations` writer. The result is the real materialized Type
identity.

Definitions will use compile-time continuation composition. The parent parser
selects the continuation from the authored Type-shaped name and passes the
consumed documentation, modifiers, name, attributes, and owning context
directly to that parser. It does not construct an evaluator object, mutable
registry record, generic Member wrapper, or transient Abstract.

## Document contract

A complete Tetrodotoxin document begins with an opening Documentation block.
Its envelope follows immediately:

```text
Documentation
dialect : DialectName;
Dialect-owned body
```

The envelope parser validates the concrete Dialect expected by its entry point.
The lowercase `dialect` marker and punctuation are TTX Codes; the accepted
Dialect name and body grammar are Tetrodotoxin parser policy.

Subtree parsers such as Type parsing do not require a document envelope because
their caller has already selected the containing grammar.

## Dialect documents

Each concrete top-level Dialect owns its source contract locally:

- [Package](package/)
- [Library](library/)
- [Render](render/)
- [Shader](shader/)
- [App](app/)
- [Scene](scene/)

[Foreign](foreign/) is an embedded Dialect rather than a top-level envelope,
but it has its own parser contract for the same ownership reason.

These documents separate accepted source shape from Package construction,
Model, Runtime, Target, Archiver, and application behavior that consumes the
resulting facts.

## Evidence boundary

The active `//tetrodotoxin:parser` target contains Comment, Type, Builtins, and
Package parsing. Documentation for every other Dialect is a source contract and
implementation guide, not a claim that its parser or evaluator exists.

Unsupported syntax must fail visibly until its real owner can consume it
directly. A structural fixture, tokenization result, or README does not count as
semantic execution.
