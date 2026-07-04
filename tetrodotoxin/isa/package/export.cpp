// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/export.hpp"

#include "tetrodotoxin/isa/package/namespace.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::Export::evaluate(
    Context& context,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Package::Export {
  Definition definition = Definition::evaluate(
      cursor, documentation, {{Class::Type::Expose}},
      {{Class::Type::Type}}, {{Class::Type::Alias, Class::Type::Type}});
  if (!definition.is_valid()) {
    return Package::Export();
  }

  if (definition.get_kind() == "Namespace"_view) {
    Package::Namespace name_space = Package::Namespace::evaluate(
        context, cursor);
    if (!name_space.is_valid()) {
      return Package::Export();
    }

    return Package::Export(definition, name_space.get_exports());
  }

  if (!(definition.get_kind() == "alias"_view ||
        definition.get_kind() == "Package"_view)) {
    cursor.token_error(
        "Expected package definition kind `alias`, `Package`, or "
        "`Namespace`."_view);
    return Package::Export();
  }

  if (!cursor.require(
          Class::Type::Assign,
          "Expected `=` before package export target."_view)) {
    return Package::Export();
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* target = context.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return Package::Export();
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after package export."_view)) {
    return Package::Export();
  }

  return Package::Export(definition, target);
}
