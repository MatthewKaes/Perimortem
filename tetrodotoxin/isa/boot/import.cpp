// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/boot/import.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/isa/qualified_name.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Import::should_evaluate(Cursor& cursor) -> Bool {
  return cursor.matches(Class::Type::Import);
}

auto Import::evaluate(Cursor& cursor, const Registry& registry) -> Import {
  if (!should_evaluate(cursor)) {
    return Import();
  }
  cursor.consume();

  auto local_name = cursor.require(
      Class::Type::Type, "Type name for local import alias."_view);
  if (!local_name) {
    cursor.recover_to_statement();
    return Import();
  }

  auto isa = Selection::evaluate(cursor, registry);
  if (!isa.is_valid()) {
    cursor.recover_to_statement();
    return Import();
  }

  if (!cursor.require(
          Class::Type::Assign, "Expected `=` after import ISA."_view)) {
    cursor.recover_to_statement();
    return Import();
  }

  // Strings are file imports. Qualified type-shaped names are package imports.
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
    auto name = QualifiedName::evaluate(cursor);
    if (!name.is_valid()) {
      cursor.recover_to_statement();
      return Import();
    }

    import_name = name.get_text();
    break;
  }

  default:
    cursor.error(
        "Unknown import semantics. Expected either a string path or a package "
        "name."_view);
    return Import();
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after import."_view)) {
    cursor.recover_to_statement();
  }

  return {local_name->get_text(), import_name, isa, package};
}
