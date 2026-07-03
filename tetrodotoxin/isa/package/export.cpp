// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/export.hpp"

#include "tetrodotoxin/isa/package/namespace.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto evaluate_target(Cursor& cursor) -> QualifiedName {
  if (!cursor.require(
          Class::Type::Assign,
          "Expected `=` before package export target."_view)) {
    return QualifiedName();
  }

  QualifiedName target = QualifiedName::evaluate(cursor);
  if (!target.is_valid()) {
    return QualifiedName();
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after package export."_view)) {
    return QualifiedName();
  }

  return target;
}

auto Export::evaluate(
    Cursor& cursor,
    Ttx::Documentation documentation) -> Export {
  static constexpr Definition::Kind allowed_kinds[] = {
    Definition::Kind::Alias,
    Definition::Kind::Namespace,
    Definition::Kind::Package,
  };

  Definition definition =
      Definition::evaluate(cursor, documentation, allowed_kinds);
  if (!definition.is_valid()) {
    return Export();
  }

  if (definition.is_namespace()) {
    Namespace name_space = Namespace::evaluate(cursor);
    if (!name_space.is_valid()) {
      return Export();
    }

    return Export(definition, name_space.get_exports());
  }

  QualifiedName target = evaluate_target(cursor);
  if (!target.is_valid()) {
    return Export();
  }

  return Export(definition, target);
}
