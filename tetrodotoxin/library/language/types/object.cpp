// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

Types::Object::Object(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition)
    : Structure(domain, definition) {}

auto Types::Object::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Object&> {
  auto transaction = cursor.branch();
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    transaction.create_token_error(
        definition.get_name_token(),
        "Library Object definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    transaction.create_token_error(
        definition.get_visibility_token(),
        "Library Objects accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    transaction.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Objects do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = transaction.require(
      Code::Type::Object,
      "Library Object definitions require the `object` qualifier."_view);
  BAIL_IF(!kind_token);

  Object& object = domain.construct_from<Object>(
      [&]() -> Object { return Object(domain, definition); });
  BAIL_IF(!object.interpret_body(transaction, definition, kind_token));
  cursor.join(transaction);
  return object;
}
