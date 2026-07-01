// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/context.hpp"

using namespace Perimortem::Core;
using namespace Ttx;

auto Context::get_peek(Count ahead) const -> const Lexical::Token& {
  Count target = cursor + ahead;
  if (target >= tokens.get_size()) {
    return tokens[tokens.get_size() - 1];
  }

  return tokens[target];
}

auto Context::advance() -> const Lexical::Token& {
  if (cursor + 1 < tokens.get_size()) {
    cursor++;
  }

  return get_current();
}

auto Context::consume() -> const Lexical::Token& {
  const Lexical::Token& token = get_current();
  advance();
  return token;
}

auto Context::expect(Lexical::Class::Type type) -> Bool {
  if (matches(type)) {
    return True;
  }

  error("Unexpected TTX token."_view);
  return False;
}

auto Context::require(Lexical::Class::Type type) -> Bool {
  Bool matched = expect(type);
  advance();
  return matched;
}

auto Context::error(View::Bytes message) -> void {
  error_count++;
  diagnostics.concat(path);
  diagnostics.concat(":"_view);
  diagnostics.concat(message);
  diagnostics.append('\n');
}
