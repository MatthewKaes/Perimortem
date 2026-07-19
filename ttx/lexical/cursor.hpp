// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Ttx::Lexical {

// Cursor acts as a transitive iterator over a token stream and can be used to
// persist parse state during a transaction.
// Any errors created are pushed to the provided error collector.
class Cursor {
 public:
  Cursor(const Lexical::Tokenizer& tokenizer, Lexical::Errors& errors)
      : tokenizer(tokenizer), errors(errors) {
    errors.set_source_context(
        tokenizer.get_source_path(), tokenizer.get_source_text());
  };

  constexpr auto current() const -> Lexical::Token {
    return tokenizer.get_tokens().get_data()[index];
  }

  constexpr auto get_token_index() const -> Count { return index; }

  // Advances at most to the tokenizer's end-of-stream token and returns the
  // token that was current before advancing.
  constexpr auto consume() -> Lexical::Token {
    const auto tokens = tokenizer.get_tokens();
    Lexical::Token consumed = current();
    if (index + 1 < tokens.get_size()) {
      index++;
    }

    return consumed;
  }

  // Requires the current token to have the expected class. Success consumes and
  // returns the token. Failure records the provided message on the mismatched
  // token and returns an invalid end of stream token.
  constexpr auto require(
      Lexical::Class::Type type,
      Perimortem::Core::View::Bytes message) -> Lexical::Token {
    if (!matches(type)) {
      create_token_error(message);
      return Lexical::Token();
    }

    return consume();
  }

  // Creates a source level error message.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  constexpr auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void {
    errors.create_general_error(message, hint);
  }

  // Creates an error at the current token.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void {
    errors.create_token_error(current(), message, hint);
  }

  auto create_token_error(
      Lexical::Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void {
    errors.create_token_error(token, message, hint);
  }

  // Emits an error over an already-known token range.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_expression_error(
      Lexical::Token start,
      Lexical::Token end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = ""_view) -> void {
    errors.create_expression_error(start, end, message, hint);
  }

  // Statement recovery is intentionally small.
  //
  // A malformed statement can skip to the next statement so later syntax still
  // reports errors in the same pass. Broader recovery belongs to the caller
  // because only that layer knows how much grammar is safe to skip.
  constexpr auto recover_to_statement() -> void {
    // The source envelope only recovers at statement boundaries. That is enough
    // to keep independent import errors visible without pretending to
    // understand the dialect body after a malformed envelope item.
    constexpr Perimortem::Core::Static::Vector<Lexical::Class::Type, 3>
        terminals = {{
          Lexical::Class::Type::EndOfStream,
          Lexical::Class::Type::EndStatement,
          Lexical::Class::Type::ScopeEnd,
        }};

    auto type = current().get_class();
    while (!type.is_one_of(terminals)) {
      consume();
      type = current().get_class();
    }

    // Consume the terminal to get it out of the way to keep parser logic
    // simple.
    //
    // It's safe to consume end of stream since the cursor makes sure to never
    // pass the end of the token stream.
    if (type.is_one_of(terminals)) {
      consume();
    }
  }

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

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return tokenizer.get_arena();
  }

  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return tokenizer.get_source_text();
  }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return tokenizer.get_source_path();
  }

 private:
  const Lexical::Tokenizer& tokenizer;
  Lexical::Errors& errors;
  Count index = 0;
};

}  // namespace Ttx::Lexical
