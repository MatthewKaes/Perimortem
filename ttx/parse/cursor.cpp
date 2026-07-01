// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/cursor.hpp"

Ttx::Parse::Cursor::Cursor(const Lexical::Tokenizer& tokenizer)
    : tokenizer(tokenizer),
      tokens(tokenizer.get_tokens()),
      errors(tokenizer.get_arena()) {}

auto Ttx::Parse::Cursor::consume() -> const Lexical::Token& {
  const Lexical::Token& token = current();
  if (token_index + 1 < tokens.get_size()) {
    token_index++;
  }

  return token;
}

auto Ttx::Parse::Cursor::require(
    Lexical::Class::Type type,
    Perimortem::Core::View::Bytes message) -> const Lexical::Token& {
  if (!matches(type)) {
    error(message);
    return current();
  }

  return consume();
}

auto Ttx::Parse::Cursor::error(Perimortem::Core::View::Bytes message) -> void {
  error(message, Perimortem::Core::View::Bytes());
}

auto Ttx::Parse::Cursor::error(
    Perimortem::Core::View::Bytes message,
    Perimortem::Core::View::Bytes hint) -> void {
  errors.insert(Error(current(), token_index, message, hint));
}

auto Ttx::Parse::Cursor::recover_to_statement() -> void {
  // The source envelope only recovers at statement boundaries. That is enough
  // to keep independent import errors visible without pretending to understand
  // the dialect body after a malformed envelope item.
  while (!matches(Lexical::Class::Type::EndOfStream) &&
         !matches(Lexical::Class::Type::EndStatement) &&
         !matches(Lexical::Class::Type::ScopeEnd)) {
    consume();
  }

  if (matches(Lexical::Class::Type::EndStatement)) {
    consume();
  }
}
