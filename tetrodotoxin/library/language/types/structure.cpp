// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Structure::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Structure&> {
  auto transaction = cursor.branch();
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    transaction.create_token_error(
        definition.get_name_token(),
        "Library Structure definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    transaction.create_token_error(
        definition.get_visibility_token(),
        "Library Structures accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    transaction.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Structures do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = transaction.require(
      Code::Type::Struct,
      "Library Structure definitions require the `struct` qualifier."_view);
  BAIL_IF(!kind_token);

  Structure& structure = domain.construct_from<Structure>(
      [&]() -> Structure { return Structure(domain, definition); });
  BAIL_IF(!structure.interpret_body(transaction, definition, kind_token));
  cursor.join(transaction);
  return structure;
}

auto Types::Structure::interpret_body(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Token kind_token) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Composite bodies require an opening `{`."_view));

  // The exact Type exists before shared body grammar so nested Functions retain
  // its final Arena identity. A rejected body leaves that private object
  // unreachable from the enclosing Composite.
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Composite body reached the end of source before `}`."_view);
      return False;
    }

    auto nested = Tetrodotoxin::Language::Definition::parse(cursor, *this);
    BAIL_IF(!nested || !interpret_definition(cursor, *nested));
  }

  Token closing = cursor.consume();
  return definition.complete(kind_token, closing);
}
