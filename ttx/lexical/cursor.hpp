// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Ttx::Lexical {

// Cursor is the transient parse state over one token stream.
//
// It exists to keep dialect parsing small and regular. Callers can preserve any
// remaining token view as a cheap continuation for a later parser. Cursor owns
// only the current token position and an Errors collection backed by the
// caller's transaction arena.
//
// The cursor sees the token position at the moment a parse expectation fails,
// so it is the right place to record error facts. The stored Error objects
// are still presentation neutral and reusable by parsers that use the
// same cursor.
//
// Error rendering, package resolution, and source record management belong
// to the layers that have that context. Cursor stays token position plus
// errors over one token stream.
class Cursor {
 public:
  Cursor(
      const Lexical::Tokenizer& tokenizer,
      Perimortem::Memory::Allocator::Arena& error_arena)
      : tokenizer(tokenizer), errors(error_arena) {};

  constexpr auto current() const -> const Lexical::Token& {
    return tokenizer.get_tokens().get_data()[index];
  }
  constexpr auto get_token_index() const -> Count { return index; }
  constexpr auto get_token_span(Count start, Count end) const
      -> Perimortem::Core::View::Vector<Lexical::Token> {
    Perimortem::Core::View::Vector<Lexical::Token> tokens =
        tokenizer.get_tokens();
    if (start >= end || start >= tokens.get_size()) {
      return Perimortem::Core::View::Vector<Lexical::Token>();
    }

    Count bounded_end = end < tokens.get_size() ? end : tokens.get_size();
    return tokens.slice(start, bounded_end - start);
  }
  auto seek_token(Count token_index) -> void;

  // Advances at most to the tokenizer's end-of-stream token and returns the
  // token that was current before advancing.
  auto consume() -> const Lexical::Token&;

  // Requires the current token to have the expected class. Success consumes and
  // returns the token. Failure records the provided message and returns null.
  auto require(Lexical::Class::Type type, Perimortem::Core::View::Bytes message)
      -> const Lexical::Token*;

  // Emits an error that is about the stream rather than one specific token.
  auto error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

  // Emits an error on the current token.
  auto token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

  // Emits an error over an already-known token range.
  auto range_error(
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void;

  // Statement recovery is intentionally small.
  //
  // A malformed statement can skip to the next statement so later syntax still
  // reports errors in the same pass. Broader recovery belongs to the caller
  // because only that layer knows how much grammar is safe to skip.
  auto recover_to_statement() -> void;

  // Checks if the current cursor is exactly one type.
  constexpr auto matches(Lexical::Class::Type type) const -> Bool {
    return current().get_class() == type;
  }

  // Checks to see if the class is an item in a range of possible values.
  constexpr auto is_one_of(
      Perimortem::Core::View::Vector<Lexical::Class::Type> types) const
      -> Bool {
    return current().get_class().is_one_of(types);
  }

  // Evaluation errors belong to the cursor because the cursor is the local
  // token context which all executors interact with for a single transaction.
  //
  // Parsed facts and lowered output should not copy errors into persisted
  // state. Consumers that need diagnostics after ending this transaction must
  // explicitly migrate them into their own lifetime.
  constexpr auto get_errors() const -> const Lexical::Errors& {
    return errors;
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
  // Tokenizers only hold structured views over their arena state after
  // construction, so the cursor keeps a copy and avoids one pointer hop.
  const Lexical::Tokenizer tokenizer;

  Lexical::Errors errors;
  Count index = 0;
};

}  // namespace Ttx::Lexical
