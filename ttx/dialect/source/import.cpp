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
    auto name_start = cursor.current().get_text();
    while (cursor.current().get_class() != Class::Type::EndStatement) {
      cursor.require(
          Class::Type::TypeAccessOp,
          "Package name segments can only be seperated by `::`"_view);
      cursor.require(
          Class::Type::Type,
          "Package name segments should all be Type names."_view);
    }

    // Get the text range between tokens.
    auto name_end = cursor.current().get_text();
    auto length =
        name_end.get_data() - name_start.get_data() + name_end.get_size();
    import_name = View::Bytes(name_start.get_data(), length);
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
