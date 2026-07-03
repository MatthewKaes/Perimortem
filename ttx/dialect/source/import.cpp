// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/source/import.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;
using namespace Ttx::Dialect;

auto Source::Import::should_parse(Cursor& cursor) -> Bool {
  return cursor.matches(Lexical::Class::Type::Import);
}

auto Source::Import::parse(Cursor& cursor) -> Source::Import {
  if (!should_parse(cursor)) {
    return Source::Import();
  }
  cursor.consume();

  // Get the name for the alias
  auto local_name = cursor.require(
      Class::Type::Type, "Type name for local import alias."_view);
  if (!local_name) {
    cursor.recover_to_statement();
    return Source::Import();
  }

  auto dialect = Source::Dialect::parse(cursor);
  if (!dialect.is_valid()) {
    cursor.recover_to_statement();
    return Source::Import();
  }

  if (!cursor.require(
          Class::Type::Assign, "Expected `=` after import dialect."_view)) {
    cursor.recover_to_statement();
    return Source::Import();
  }

  // Parses either a string which indicates a source file, or a Type which
  // implies a package name.
  View::Bytes import_name;
  Bool package = False;
  switch (cursor.current().get_class().get_type()) {
  case Class::Type::String: {
    auto source_text = cursor.current().get_text();
    import_name = source_text.slice(1, source_text.get_size() - 2);
    cursor.consume();
    break;
  }

  case Class::Type::Type: {
    package = True;
    auto name_start = cursor.require(
        Class::Type::Type,
        "Package name should start with a Type name."_view);
    if (!name_start) {
      cursor.recover_to_statement();
      return Source::Import();
    }

    const Token* name_end = name_start;
    while (cursor.matches(Class::Type::TypeAccessOp)) {
      cursor.consume();
      name_end = cursor.require(
          Class::Type::Type,
          "Package name segments should all be Type names."_view);
      if (!name_end) {
        cursor.recover_to_statement();
        return Source::Import();
      }
    }

    // Get the text range between tokens.
    auto start = name_start->get_text();
    auto end = name_end->get_text();
    import_name = View::Bytes(
        start.get_data(), end.get_data() - start.get_data() + end.get_size());
    break;
  }

  default:
    cursor.error(
        "Unknown import semantics. Expected either a string path or a package "
        "name."_view);
    return Source::Import();
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after import."_view)) {
    cursor.recover_to_statement();
  }

  return {local_name->get_text(), import_name, dialect, package};
}
