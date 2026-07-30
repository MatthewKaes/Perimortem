// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/writer/textual.hpp"

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
      : tokenizer(tokenizer), errors(errors) {}

  constexpr auto current() const -> Lexical::Token {
    return tokenizer.get_tokens().get_data()[index];
  }

  constexpr auto get_token_index() const -> Count { return index; }

  // Advances at most to the tokenizer's terminal token and returns the
  // token that was current before advancing.
  constexpr auto consume() -> Lexical::Token {
    const auto tokens = tokenizer.get_tokens();
    Lexical::Token consumed = current();
    if (index + 1 < tokens.get_size()) {
      index++;
    }

    return consumed;
  }

  // Requires the current token to have the expected Code. Success consumes and
  // returns the token. Failure records the provided message on the mismatched
  // token and returns an invalid end of stream token.
  constexpr auto require(
      Code::Type type,
      Perimortem::Core::View::Bytes message = {}) -> Lexical::Token {
    if (!matches(type)) {
      Code code(type);
      Perimortem::Core::Static::Bytes<128> hint_buffer;
      Perimortem::Core::Writer::Textual hint_message(hint_buffer);
      hint_message << "Expected lexical token "_view << code.get_semantics()
                   << " but got "_view << current().get_code().get_semantics()
                   << "."_view;

      // If no message was supplied then upgrade the hint message to the error
      // message, but if a richer message was supplied then downgrade the info
      // to the hint message.
      if (message.is_empty()) {
        create_token_error(hint_message);
      } else {
        create_token_error(message, hint_message);
      }

      return Lexical::Token();
    }

    return consume();
  }

  // If the token isn't of the required type then log a message and give up on
  // trying to parse the statement as it's most likely in an unrecoverable state
  // that will just cause a cascade of errors.
  //
  // TODO: If it ever comes up that we need to bail on different kinds of
  // statements we should fold that in but not until we have a real use case.
  // Recover to balance braces was used in the old parser quite a bit.
  constexpr auto bail(
      Code::Type type,
      Perimortem::Core::View::Bytes message = {}) -> Bool {
    if (!require(type, message)) {
      recover_to_statement();
      return true;
    }

    return false;
  }

  // Checks if the token is a required semantic keyword instead of a token.
  //
  // TODO: If it ever comes up that we need to bail on different kinds of
  // statements we should fold that in but not until we have a real use case.
  // Recover to balance braces was used in the old parser quite a bit.
  constexpr auto bail(
      Perimortem::Core::View::Bytes keyword,
      Perimortem::Core::View::Bytes message = {}) -> Bool {
    auto candidate = require(Code::Type::Addressable, message);
    if (!candidate) {
      recover_to_statement();
      return true;
    }

    auto text = candidate.caculate_text(get_source_text());
    if (text != keyword) {
      Perimortem::Core::Static::Bytes<128> hint_buffer;
      Perimortem::Core::Writer::Textual hint_message(hint_buffer);
      hint_message << "Expected `"_view << keyword << "` but got `"_view << text
                   << "`."_view;
      if (message.is_empty()) {
        create_token_error(candidate, hint_message);
      } else {
        create_token_error(candidate, message, hint_message);
      }

      recover_to_statement();
      return true;
    }

    return false;
  }

  // Creates a source level error message.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(Token(), Token(), message, hint);
  }

  // Creates an error at the current token.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_token_error(current(), message, hint);
  }

  auto create_token_error(
      Lexical::Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(token, token, message, hint);
  }

  // Emits an error over an existing token range.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_expression_error(
      Lexical::Token start,
      Lexical::Token end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(), start,
        end);
    report << message;
    report.get_hint() << hint;
  }

  // Statement recovery is intentionally small.
  //
  // A malformed statement can skip to the next statement so later syntax still
  // reports errors in the same pass. Broader recovery belongs to the caller
  // because only that layer knows how much grammar is safe to skip.
  //
  // By default `Terminal`, `EndStatement` and `ScopeEnd` are used as the sync
  // points but this can very by dialect.
  constexpr auto recover_to_statement(
      Perimortem::Core::View::Vector<Code::Type> terminals = {{
        Code::Type::Terminal,
        Code::Type::EndStatement,
        Code::Type::ScopeEnd,
      }}) -> void {
    auto type = current().get_code();
    while (!type.is_one_of(terminals)) {
      consume();
      type = current().get_code();
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

  constexpr auto caculate_text(Token token) const
      -> Perimortem::Core::View::Bytes {
    return token.caculate_text(get_source_text());
  }

  // Checks if the current cursor is exactly one type.
  constexpr auto matches(Code::Type type) const -> Bool {
    return current().get_code() == type;
  }

  // Checks to see if the Code is an item in a range of possible values.
  constexpr auto is_one_of(
      Perimortem::Core::View::Vector<Code::Type> types) const -> Bool {
    return current().get_code().is_one_of(types);
  }

  constexpr auto get_code() const -> Code { return current().get_code(); }

  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return current().caculate_text(get_source_text());
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
