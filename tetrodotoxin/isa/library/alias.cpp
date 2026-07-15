// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/alias.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Alias::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition)
    -> const Ttx::Type* {
  Bool has_assignment = cursor.require(
      Class::Type::Assign, "Expected `=` before library alias target."_view);
  if (!has_assignment) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* target = scope.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  Bool has_statement_end = cursor.require(
      Class::Type::EndStatement, "Expected `;` after library alias."_view);
  if (!has_statement_end) {
    return nullptr;
  }

  if (target == nullptr) {
    cursor.token_error("Library alias target could not be resolved."_view);
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  Base::Attribute::append_all(definition.get_attributes(), attributes);
  auto& type =
      scope.get_context().get_arena().construct<Ttx::Type>(Ttx::Type::alias(
          definition.get_name(), *target, definition.get_documentation(),
          attributes.get_view()));
  return &type;
}
