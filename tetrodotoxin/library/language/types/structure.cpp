// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Structure::validate_definition(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Type definitions require a Type shaped name."_view);
    return False;
  }

  Count publications = 0;
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    Token modifier = modifiers.get_data()[i];
    if (modifier.get_code() == Code::Type::Public ||
        modifier.get_code() == Code::Type::Private) {
      publications++;
      continue;
    }

    cursor.create_token_error(
        modifier,
        "Library Type definitions accept only visibility modifiers."_view);
    return False;
  }

  if (publications != 1) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Type definitions require one visibility modifier."_view);
    return False;
  }

  return True;
}

Types::Structure::Structure(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Monograph& source,
    Materializations& materializations,
    const Composite& enclosing_scope)
    : Composite(domain, source, materializations, enclosing_scope),
      definition(definition),
      anchor(definition.get_anchor()) {}

auto Types::Structure::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Monograph& source,
    Materializations& materializations,
    const Composite& enclosing_scope) -> Option<Structure&> {
  auto transaction = cursor.branch();
  BAIL_IF(!validate_definition(transaction, definition));

  Token kind_token = transaction.require(
      Code::Type::Addressable,
      "Library Structure definitions require the `struct` qualifier."_view);
  BAIL_IF(!kind_token);
  View::Bytes kind = kind_token.caculate_text(transaction.get_source_text());
  if (kind != "struct"_view) {
    transaction.create_token_error(
        kind_token,
        "Library Structure definitions require the `struct` qualifier."_view);
    return {};
  }

  Structure& structure = domain.construct_from<Structure>([&]() -> Structure {
    return Structure(
        domain, definition, source, materializations, enclosing_scope);
  });
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

    auto nested = Tetrodotoxin::Language::Definition::parse(cursor);
    BAIL_IF(!nested || !interpret_definition(cursor, *nested));
  }

  Token closing = cursor.consume();
  complete_definition(
      Anchor::create(
          kind_token,
          Span(definition.get_anchor().get_span().get_start(), closing)));
  return True;
}

auto Types::Structure::complete_definition(Anchor complete_anchor) -> void {
  anchor = complete_anchor;
}
