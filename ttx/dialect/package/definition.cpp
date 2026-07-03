// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/package/definition.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Dialect;
using namespace Ttx::Lexical;

static auto is_allowed(
    Package::Definition::Kind kind,
    View::Vector<Package::Definition::Kind> allowed_kinds) -> Bool {
  for (Count kind_index = 0; kind_index < allowed_kinds.get_size();
       kind_index++) {
    if (allowed_kinds[kind_index] == kind) {
      return True;
    }
  }

  return False;
}

static auto parse_kind(
    Cursor& cursor,
    Package::Definition::Kind& output) -> Bool {
  if (cursor.matches(Class::Type::Alias)) {
    cursor.consume();
    output = Package::Definition::Kind::Alias;
    return True;
  }

  const Token* kind = cursor.require(
      Class::Type::Type,
      "Expected package definition kind `alias`, `Package`, or `Namespace`."_view);
  if (kind == nullptr) {
    return False;
  }

  if (kind->get_text() == "Namespace"_view) {
    output = Package::Definition::Kind::Namespace;
    return True;
  }

  if (kind->get_text() == "Package"_view) {
    output = Package::Definition::Kind::Package;
    return True;
  }

  cursor.token_error(
      "Expected package definition kind `alias`, `Package`, or `Namespace`."_view);
  return False;
}

auto Package::Definition::parse(
    Cursor& cursor,
    Ttx::Documentation documentation,
    View::Vector<Kind> allowed_kinds) -> Package::Definition {
  if (!cursor.require(
          Class::Type::ConstPublic,
          "Expected `@public` package definition."_view)) {
    return Package::Definition();
  }

  const Token* name = cursor.require(
      Class::Type::Type, "Expected package definition name."_view);
  if (name == nullptr) {
    return Package::Definition();
  }

  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` after package definition name."_view)) {
    return Package::Definition();
  }

  Kind kind = Kind::Alias;
  if (!parse_kind(cursor, kind)) {
    return Package::Definition();
  }

  if (!is_allowed(kind, allowed_kinds)) {
    cursor.token_error("Package definition kind is not allowed here."_view);
    return Package::Definition();
  }

  return Package::Definition(documentation, name->get_text(), kind);
}
