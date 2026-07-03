// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/error.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Ttx::Lexical {

// Cursor is the transient parse state over a token view.
//
// It exists to keep the top level source parse small and regular. The source
// envelope is ordered as documentation, required dialect header, then zero or
// more imports. After the envelope parser stops, callers can preserve the
// remaining token view as a cheap continuation for dialect body parsing.
// Cursor owns only the current token position and the parse errors allocated in
// the provided arena.
//
// The cursor sees the token position at the moment a parse expectation fails,
// so it is the right place to record error facts. The stored Error objects
// are still presentation neutral and reusable by dialect parsers that use the
// same cursor.
//
// Error rendering, package resolution, and source record management belong
// to the layers that have that context. Cursor stays token position plus parse
// errors over one token stream.
class Cursor {
 public:
  Cursor(const Lexical::Tokenizer& tokenizer)
      : tokenizer(tokenizer), errors(error_arena) {};

  constexpr auto current() const -> const Lexical::Token& {
    return tokenizer.get_tokens().get_data()[index];
  }

  // Advances at most to the tokenizer's end-of-stream token and returns the
  // token that was current before advancing.
  auto consume() -> const Lexical::Token&;

  // Attempts to bind the current token based on a given type.
  // If the token can't bind then an error is reported using the provided
  // message and null is returned.
  // If the token can bind the token then it's consumed and returned.
  auto require(Lexical::Class::Type type, Perimortem::Core::View::Bytes message)
      -> const Lexical::Token*;

  // Emits a context free error.
  auto error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

  // Emits an error on the current token which is a common parse use case.
  auto token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

  // Emits an error on the current token which is a common parse use case.
  auto range_error(
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

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
  // whether to stop, continue through recoverable errors, or render error
  // output without caring which parser phase produced the error.
  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Lexical::Error> {
    return errors.get_view();
  }

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return tokenizer.get_arena();
  }
  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return tokenizer.get_source_text();
  }
  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return tokenizer.get_source_name();
  }

 private:
  // Tokenizer's just hold strutured views over their arena state after
  // construction so we can make a clone to avoid a level of redirection.
  const Lexical::Tokenizer tokenizer;

  // A tokenizer can have multiple arenas so create an arena
  Perimortem::Memory::Allocator::Arena error_arena;
  Perimortem::Memory::Managed::Vector<Lexical::Error> errors;
  Count index = 0;
};

}  // namespace Ttx::Lexical
