// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/cursor.hpp"

Ttx::Parse::Cursor::Cursor(const Lexical::Tokenizer& tokenizer)
    : tokenizer(tokenizer),
      tokens(tokenizer.get_tokens()),
      errors(tokenizer.get_arena()) {}

auto Ttx::Parse::Cursor::current() const -> const Lexical::Token& {
  return tokens[token_index];
}

auto Ttx::Parse::Cursor::consume() -> const Lexical::Token& {
  const Lexical::Token& token = current();
  if (token_index < tokens.get_size()) {
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
  while (!matches(Lexical::Class::Type::EndOfStream) &&
         !matches(Lexical::Class::Type::EndStatement) &&
         !matches(Lexical::Class::Type::ScopeEnd)) {
    consume();
  }

  if (matches(Lexical::Class::Type::EndStatement)) {
    consume();
  }
}

auto Ttx::Parse::Cursor::matches(Lexical::Class::Type type) const -> Bool {
  return current().get_class() == type;
}

auto Ttx::Parse::Cursor::has_error() const -> Bool {
  return !errors.is_empty();
}

auto Ttx::Parse::Cursor::get_errors() const
    -> Perimortem::Core::View::Vector<Error> {
  return errors.get_view();
}

auto Ttx::Parse::Cursor::remaining_tokens() const
    -> Perimortem::Core::View::Vector<Lexical::Token> {
  return tokens.slice(token_index);
}

auto Ttx::Parse::Cursor::get_arena() const
    -> Perimortem::Memory::Allocator::Arena& {
  return tokenizer.get_arena();
}

auto Ttx::Parse::Cursor::get_token_index() const -> Count {
  return token_index;
}

auto Ttx::Parse::Cursor::get_error_count() const -> Count {
  return errors.get_size();
}
