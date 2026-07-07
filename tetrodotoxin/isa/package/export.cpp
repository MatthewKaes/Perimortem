// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/export.hpp"

#include "tetrodotoxin/isa/expression/type.hpp"
#include "tetrodotoxin/isa/package/group.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::Export::evaluate(
    Cursor& cursor,
    Context& context,
    Ttx::Documentation documentation) -> Package::Export {
  Definition definition = Definition::evaluate(
      cursor, documentation, {{Class::Type::Expose}},
      {{Class::Type::Type}},
      {{Class::Type::Alias, Class::Type::Type, Class::Type::Addressable}});
  if (!definition.is_valid()) {
    return Package::Export();
  }

  if (definition.get_kind() == "group"_view) {
    Package::Group group = Package::Group::evaluate(cursor, context);
    if (!group.is_valid()) {
      return Package::Export();
    }

    return Package::Export(definition, group.get_exports());
  }

  if (definition.get_kind() != "alias"_view) {
    cursor.token_error(
        "Expected package definition kind `alias` or `group`."_view);
    return Package::Export();
  }

  if (!cursor.require(
          Class::Type::Assign,
          "Expected `=` before package export target."_view)) {
    return Package::Export();
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* target = Expression::Type::evaluate(cursor, context);
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
