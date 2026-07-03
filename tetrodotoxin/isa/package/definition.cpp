// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/definition.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto is_allowed(
    Definition::Kind kind,
    View::Vector<Definition::Kind> allowed_kinds) -> Bool {
  for (Count kind_index = 0; kind_index < allowed_kinds.get_size();
       kind_index++) {
    if (allowed_kinds[kind_index] == kind) {
      return True;
    }
  }

  return False;
}

static auto evaluate_kind(
    Cursor& cursor,
    Definition::Kind& output) -> Bool {
  if (cursor.matches(Class::Type::Alias)) {
    cursor.consume();
    output = Definition::Kind::Alias;
    return True;
  }

  const Token* kind = cursor.require(
      Class::Type::Type,
      "Expected package definition kind `alias`, `Package`, or `Namespace`."_view);
  if (kind == nullptr) {
    return False;
  }

  if (kind->get_text() == "Namespace"_view) {
    output = Definition::Kind::Namespace;
    return True;
  }

  if (kind->get_text() == "Package"_view) {
    output = Definition::Kind::Package;
    return True;
  }

  cursor.token_error(
      "Expected package definition kind `alias`, `Package`, or `Namespace`."_view);
  return False;
}

auto Definition::evaluate(
    Cursor& cursor,
    Ttx::Documentation documentation,
    View::Vector<Kind> allowed_kinds) -> Definition {
  if (!cursor.require(
          Class::Type::ConstPublic,
          "Expected `@public` package definition."_view)) {
    return Definition();
  }

  const Token* name = cursor.require(
      Class::Type::Type, "Expected package definition name."_view);
  if (name == nullptr) {
    return Definition();
  }

  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` after package definition name."_view)) {
    return Definition();
  }

  Kind kind = Kind::Alias;
  if (!evaluate_kind(cursor, kind)) {
    return Definition();
  }

  if (!is_allowed(kind, allowed_kinds)) {
    cursor.token_error("Package definition kind is not allowed here."_view);
    return Definition();
  }

  return Definition(documentation, name->get_text(), kind);
}
