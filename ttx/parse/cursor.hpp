// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/tokenizer.hpp"
#include "ttx/parse/error.hpp"

namespace Ttx::Parse {

// Cursor is the non-owning parse state over a tokenizer result.
//
// It exists to keep the top level source parse small and regular. The source
// envelope is ordered as documentation, required dialect header, then zero or
// more imports. After the envelope parser stops, the same cursor remains
// positioned at the first dialect-owned token so Tetrodotoxin can continue with
// one parse state after resolution decides that continuing is useful. Cursor
// owns only the current token position and the parse errors allocated in the
// tokenizer arena.
//
// The cursor sees the token position at the moment a parse expectation fails,
// so it is the right place to record diagnostic facts. The stored Error objects
// are still presentation neutral and reusable by dialect parsers that use the
// same cursor.
//
// Diagnostic rendering, package resolution, and source record management belong
// to the layers that have that context. Cursor stays token position plus parse
// errors over one token stream.
class Cursor {
 public:
  explicit Cursor(const Lexical::Tokenizer& tokenizer);

  constexpr auto current() const -> const Lexical::Token& {
    return tokens[token_index];
  }

  // Advances at most to the tokenizer's end-of-stream token and returns the
  // token that was current before advancing.
  auto consume() -> const Lexical::Token&;
  auto require(
      Lexical::Class::Type type,
      Perimortem::Core::View::Bytes message) -> const Lexical::Token&;
  auto error(Perimortem::Core::View::Bytes message) -> void;
  auto error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint) -> void;

  // Envelope recovery is intentionally small.
  //
  // A malformed import can skip to the next statement so later imports still
  // report errors in the same pass. A malformed dialect header is not
  // recoverable because it selects the dialect that owns the remaining grammar.
  auto recover_to_statement() -> void;

  constexpr auto matches(Lexical::Class::Type type) const -> Bool {
    return current().get_class() == type;
  }

  // Parse errors belong to the cursor because the cursor is the parse context.
  //
  // Source envelopes, dialect ASTs, and lowered IR should not copy this into
  // their own validity state. Tetrodotoxin can inspect the cursor and decide
  // whether to stop, continue through recoverable errors, or render diagnostics
  // without caring which parser phase produced the error.
  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Error> {
    return errors.get_view();
  }

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return tokenizer.get_arena();
  }
  constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
    return tokenizer.get_source();
  }

 private:
  const Lexical::Tokenizer& tokenizer;
  Perimortem::Core::View::Vector<Lexical::Token> tokens;
  Perimortem::Memory::Managed::Vector<Error> errors;
  Count token_index = 0;
};

}  // namespace Ttx::Parse
