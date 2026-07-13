// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/addressable.hpp"

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Addressable::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition,
    Base::Definition& implementation) -> const Ttx::Member* {
  if (!definition.has_addressable_name()) {
    cursor.token_error("Expected library member name."_view);
    return nullptr;
  }

  if (definition.get_kind_class() != Class::Type::Type) {
    cursor.token_error("Expected library member type."_view);
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type = scope.resolve_type(cursor, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  const Base::Expression::Value* initializer = nullptr;
  if (cursor.matches(Class::Type::Assign)) {
    cursor.consume();
    Base::Expression::Value value =
        Base::Expression::Value::evaluate(cursor, scope.get_context());
    if (value.is_empty()) {
      return nullptr;
    }

    initializer =
        &scope.get_context().get_arena().construct<Base::Expression::Value>(
            value);
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after library member."_view)) {
    return nullptr;
  }

  if (type != nullptr) {
    implementation = Base::Definition(
        definition.get_modifier(), definition.get_attributes(), initializer);
    return &scope.get_context().get_arena().construct<Ttx::Member>(
        definition.get_name(), *type, definition.get_documentation(), False,
        definition.get_attributes());
  }

  cursor.token_error("Library member type could not be resolved."_view);
  return nullptr;
}
