// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/foreign.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Foreign::evaluate(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  if (!skip_scope(
          cursor, "Expected `{` after library foreign declaration."_view)) {
    return nullptr;
  }

  auto& type = scope.get_context().get_arena().construct<Ttx::Type>(
      definition.get_name(), definition.get_documentation());
  return &type;
}

auto Library::Foreign::skip_scope(Cursor& cursor, View::Bytes message) -> Bool {
  if (!cursor.require(Class::Type::ScopeStart, message)) {
    return False;
  }

  Count depth = 1;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::ScopeEnd)) {
      cursor.consume();
      depth--;
      if (depth == 0) {
        return True;
      }
      continue;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `}` after library scope."_view);
  return False;
}
