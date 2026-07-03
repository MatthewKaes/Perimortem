// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/cursor.hpp"

using namespace Ttx::Lexical;
using namespace Perimortem::Core;

auto Cursor::consume() -> const Lexical::Token& {
  const Lexical::Token& token = current();
  if (index + 1 < tokenizer.get_tokens().get_size()) {
    index++;
  }

  return token;
}

auto Cursor::require(Lexical::Class::Type type, View::Bytes message)
    -> const Lexical::Token* {
  if (!matches(type)) {
    error(message);
    return nullptr;
  }

  return &consume();
}

auto Cursor::error(View::Bytes message, View::Bytes hint) -> void {
  errors.insert(
      Lexical::Error(
          tokenizer.get_source_text(), tokenizer.get_source_name(), message,
          hint));
}

auto Cursor::token_error(View::Bytes message, View::Bytes hint) -> void {
  errors.insert(
      Lexical::Error(
          current(), tokenizer.get_source_text(), tokenizer.get_source_name(),
          message, hint));
}

auto Cursor::range_error(
    const Lexical::Token& start,
    const Lexical::Token& end,
    View::Bytes message,
    View::Bytes hint) -> void {
  errors.insert(
      Lexical::Error(
          start, end, tokenizer.get_source_text(), tokenizer.get_source_name(),
          message, hint));
}

auto Cursor::recover_to_statement() -> void {
  // The source envelope only recovers at statement boundaries. That is enough
  // to keep independent import errors visible without pretending to understand
  // the dialect body after a malformed envelope item.
  constexpr Static::Vector<Lexical::Class::Type, 3> terminals = {{
    Lexical::Class::Type::EndOfStream,
    Lexical::Class::Type::EndStatement,
    Lexical::Class::Type::ScopeEnd,
  }};

  auto type = current().get_class();
  while (!type.is_one_of(terminals)) {
    consume();
    type = current().get_class();
  }

  // Consume the terminal to get it out of the way to keep parser logic simple.
  //
  // It's safe to consume end of stream since the cursor makes sure to never
  // pass the end of the token stream.
  if (type.is_one_of(terminals)) {
    consume();
  }
}
