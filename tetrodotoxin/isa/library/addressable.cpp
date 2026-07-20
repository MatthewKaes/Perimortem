// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/addressable.hpp"

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"
#include "tetrodotoxin/standard/types.hpp"

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

  if (definition.get_kind_class() != Code::Type::Type) {
    cursor.token_error("Expected library member type."_view);
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type = scope.resolve_type(cursor, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  Base::Expression::Value initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();

    const Ttx::Type* meta_type =
        Tetrodotoxin::Standard::Types::find_type("Type"_view);
    if (type != nullptr && meta_type != nullptr &&
        type->equivalent_to(*meta_type) && cursor.matches(Code::Type::Type)) {
      // A Type initializer is a semantic type query, not a runtime reference
      // expression. Resolve it through the Library scope so imports, nested
      // types, and aliases produce the same canonical identity used by every
      // other type query.
      const Ttx::Type* reflected = scope.resolve_type(cursor);
      if (reflected == nullptr) {
        cursor.token_error("Reflected Type could not be resolved."_view);
        return nullptr;
      }

      initializer = Base::Expression::Value::type(*reflected);
    } else {
      initializer =
          Base::Expression::Value::evaluate(cursor, scope.get_context());
    }

    if (initializer.is_empty()) {
      return nullptr;
    }
  }

  Bool has_statement_end = cursor.require(
      Code::Type::EndStatement, "Expected `;` after library member."_view);
  if (!has_statement_end) {
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
