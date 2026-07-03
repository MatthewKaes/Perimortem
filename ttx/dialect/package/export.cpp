// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/package/export.hpp"

#include "ttx/dialect/package/namespace.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Dialect;
using namespace Ttx::Lexical;

static auto parse_target(Cursor& cursor) -> SymbolPath {
  if (!cursor.require(
          Class::Type::Assign,
          "Expected `=` before package export target."_view)) {
    return SymbolPath();
  }

  SymbolPath target = SymbolPath::parse(cursor);
  if (!target.is_valid()) {
    return SymbolPath();
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after package export."_view)) {
    return SymbolPath();
  }

  return target;
}

auto Package::Export::parse(
    Cursor& cursor,
    Ttx::Documentation documentation) -> Package::Export {
  static constexpr Package::Definition::Kind allowed_kinds[] = {
    Package::Definition::Kind::Alias,
    Package::Definition::Kind::Namespace,
    Package::Definition::Kind::Package,
  };

  Package::Definition definition =
      Package::Definition::parse(cursor, documentation, allowed_kinds);
  if (!definition.is_valid()) {
    return Package::Export();
  }

  if (definition.is_namespace()) {
    Package::Namespace name_space = Package::Namespace::parse(cursor);
    if (!name_space.is_valid()) {
      return Package::Export();
    }

    return Package::Export(definition, name_space.get_exports());
  }

  SymbolPath target = parse_target(cursor);
  if (!target.is_valid()) {
    return Package::Export();
  }

  return Package::Export(definition, target);
}
