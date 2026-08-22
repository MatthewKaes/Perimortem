// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Ttx::Lexical {

// Cursor is the one mutable position over an immutable authored Token stream.
// Grammar owners consume that position only after deterministic dispatch, and
// every failure is published to the operation error log.
class Cursor {
 public:
  constexpr Cursor(
      const Lexical::Tokenizer& tokenizer,
      Lexical::Errors& errors,
      Lexical::Associations& associations)
      : tokenizer(tokenizer), errors(errors), associations(associations) {}
  Cursor(const Cursor&) = delete;

  // A grammar owner may add one required syntax diagnostic only when a nested
  // parser did not already publish the more precise reason for rejection.
  constexpr auto get_error_count() const -> Count { return errors.get_size(); }

  // Gets the token from the tokenizer at the current location.
  // If the current index is out of bounds then an empty token is returned.
  constexpr auto current() const -> Lexical::Token {
    return tokenizer.get_tokens()[index];
  }

  // Looks at a Token relative to the current position. Positive offsets look
  // forward and negative offsets look backward. Passing zero returns current,
  // while either stream boundary returns an empty Token.
  constexpr auto peek(S64 offset) const -> Lexical::Token {
    return tokenizer.get_tokens()[index + offset];
  }

  // Advances at most to the tokenizer's terminal token and returns the
  // token that was current before advancing.
  constexpr auto consume() -> Lexical::Token {
    Lexical::Token consumed = current();
    if (index + 1 < tokenizer.get_tokens().get_size()) {
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

  // Creates a source level error message.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_expression_error(Anchor::create(Span()), message, hint);
  }

  // Creates an error at the current token.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_expression_error(Anchor::create(Span(current())), message, hint);
  }

  auto create_token_error(
      Lexical::Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_expression_error(Anchor::create(Span(token)), message, hint);
  }

  // A Span defaults its diagnostic focus to the opening Token. Callers with a
  // more precise semantic Token provide the Anchor overload directly.
  // Views can be temporary as the error context copies the data into its local
  // memory space in case the error outlives the source.
  auto create_expression_error(
      Lexical::Span span,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_expression_error(Anchor::create(span), message, hint);
  }

  auto create_expression_error(
      Perimortem::Core::Option<Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    if (!anchor) {
      create_error(message, hint);
      return;
    }
    create_expression_error(*anchor, message, hint);
  }

  auto create_expression_error(
      Lexical::Anchor anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        anchor);
    report << message;
    report.get_hint() << hint;
  }

  // Some semantic errors contribute directly to a Report. Cursor supplies the
  // authored source facts without exposing its Errors owner to the consumer.
  auto create_report(Lexical::Span span) const -> Errors::Report {
    return create_report(Anchor::create(span));
  }

  auto create_report(Perimortem::Core::Option<Lexical::Anchor> anchor) const
      -> Errors::Report {
    return create_report(anchor ? *anchor : Anchor::create(Lexical::Span()));
  }

  auto create_report(Lexical::Anchor anchor) const -> Errors::Report {
    return Errors::Report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        anchor);
  }

  // Statement recovery is intentionally small.
  //
  // A malformed statement can skip to the next statement so later syntax still
  // reports errors in the same pass. Broader recovery belongs to the caller
  // because only that layer knows how much grammar is safe to skip.
  //
  // By default `Terminal`, `EndStatement` and `ScopeEnd` are used as the sync
  // points but this can vary by dialect.
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

  constexpr auto caculate_text(Span span) const
      -> Perimortem::Core::View::Bytes {
    return span.caculate_text(get_source_text());
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

  // Returns the source artifact populated by semantic owners while this Cursor
  // drives the transaction. Workspace publishes the artifact, not the Cursor.
  constexpr auto get_associations() -> Lexical::Associations& {
    return associations;
  }

 private:
  const Lexical::Tokenizer& tokenizer;
  Lexical::Errors& errors;
  Lexical::Associations& associations;
  Count index = 0;
};

}  // namespace Ttx::Lexical
