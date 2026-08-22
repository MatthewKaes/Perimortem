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

// Cursor gives a grammar one shared position in immutable authored Tokens and
// carries source Errors and Associations through the same transaction.
// Deterministic dispatch lets each grammar owner advance it after recognizing
// the form it owns.
class Cursor {
 public:
  constexpr Cursor(
      const Lexical::Tokenizer& tokenizer,
      Lexical::Errors& errors,
      Lexical::Associations& associations)
      : tokenizer(tokenizer), errors(errors), associations(associations) {}
  Cursor(const Cursor&) = delete;

  // Nested parsers can report a more precise error before returning. Capturing
  // this count lets the outer grammar notice that work and avoid adding a
  // second, less useful diagnostic.
  constexpr auto get_error_count() const -> Count { return errors.get_size(); }

  constexpr auto current() const -> Lexical::Token {
    return tokenizer.get_tokens()[index];
  }

  constexpr auto peek(S64 offset) const -> Lexical::Token {
    return tokenizer.get_tokens()[index + offset];
  }

  constexpr auto consume() -> Lexical::Token {
    Lexical::Token consumed = current();
    if (index + 1 < tokenizer.get_tokens().get_size()) {
      index++;
    }

    return consumed;
  }

  // Grammar code usually knows the friendlier message, while Cursor can always
  // describe the exact Token mismatch. A supplied message becomes the main
  // diagnostic and keeps that lexical comparison as a useful hint.
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

      // With no richer message, the lexical comparison is the useful error.
      // Otherwise the grammar message leads and the comparison adds context.
      if (message.is_empty()) {
        create_token_error(hint_message);
      } else {
        create_token_error(message, hint_message);
      }

      return Lexical::Token();
    }

    return consume();
  }

  // Errors copies message text into its Arena. Callers can safely assemble that
  // text in temporary buffers and choose the source focus that best explains
  // the failure.
  auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_expression_error(Anchor::create(Span()), message, hint);
  }

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

  // A Span uses its opening Token as the natural focus. Semantic owners that
  // know a more useful Token can pass an Anchor while keeping the broader range
  // for the editor.
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

  // A few semantic owners add structured detail directly to a Report. Cursor
  // lends them the same source and Errors context so those diagnostics join the
  // current operation log.
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

  // Statement recovery stops at the small boundaries every grammar can
  // recognize. That lets parsing continue after one malformed statement. A
  // caller can provide a different set when its Dialect knows a safer place to
  // resume.
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

    // The boundary belongs to the malformed statement too, so consuming it
    // leaves the next parser at fresh syntax. At the end of the stream Cursor
    // simply remains on its terminal Token.
    if (type.is_one_of(terminals)) {
      consume();
    }
  }

  constexpr auto caculate_text(Span span) const
      -> Perimortem::Core::View::Bytes {
    return span.caculate_text(get_source_text());
  }

  constexpr auto matches(Code::Type type) const -> Bool {
    return current().get_code() == type;
  }

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

  constexpr auto get_tokens() const
      -> Perimortem::Core::View::Vector<Lexical::Token> {
    return tokenizer.get_tokens();
  }

  // Semantic owners add authored identities while Cursor still has their exact
  // source context. Workspace later publishes these same Associations beside
  // the completed graph.
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
