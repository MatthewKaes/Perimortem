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
    Tetrodotoxin::Language::Definition& definition,
    Monograph& source,
    Materializations& materializations,
    const Composite& enclosing_scope)
    : Structure(domain, definition, source, materializations, enclosing_scope) {
}

auto Types::Object::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Monograph& source,
    Materializations& materializations,
    const Composite& enclosing_scope) -> Option<Object&> {
  auto transaction = cursor.branch();
  BAIL_IF(!validate_definition(transaction, definition));

  Token kind_token = transaction.require(
      Code::Type::Addressable,
      "Library Object definitions require the `object` qualifier."_view);
  BAIL_IF(!kind_token);
  View::Bytes kind = kind_token.caculate_text(transaction.get_source_text());
  if (kind != "object"_view) {
    transaction.create_token_error(
        kind_token,
        "Library Object definitions require the `object` qualifier."_view);
    return {};
  }

  Object& object = domain.construct_from<Object>([&]() -> Object {
    return Object(
        domain, definition, source, materializations, enclosing_scope);
  });
  BAIL_IF(!object.interpret_body(transaction, definition, kind_token));
  cursor.join(transaction);
  return object;
}
