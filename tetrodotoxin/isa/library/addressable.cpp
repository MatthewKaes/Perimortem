// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/addressable.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Addressable::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Definition& definition) -> Ttx::Type::Member {
  if (!definition.has_addressable_name()) {
    cursor.token_error("Expected library member name."_view);
    return Ttx::Type::Member();
  }

  if (definition.get_kind_class() != Class::Type::Type) {
    cursor.token_error("Expected library member type."_view);
    return Ttx::Type::Member();
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type = scope.resolve_type(cursor, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return Ttx::Type::Member();
  }

  if (!Expression::consume_initializer(
          cursor, "Expected `;` after library member initializer."_view)) {
    return Ttx::Type::Member();
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after library member."_view)) {
    return Ttx::Type::Member();
  }

  if (type != nullptr) {
    return Ttx::Type::Member(
        definition.get_name(), *type, definition.get_documentation());
  }

  cursor.token_error("Library member type could not be resolved."_view);
  return Ttx::Type::Member();
}
