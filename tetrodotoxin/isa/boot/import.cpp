// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/boot/import.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/package/package_name.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Boot::Import::evaluate(Cursor& cursor, const Registry& registry)
    -> Boot::Import {
  Definition definition = Definition::evaluate(
      cursor, Ttx::Documentation(), {{Class::Type::Import}},
      {{Class::Type::Type}}, {{Class::Type::Type}});
  if (!definition.is_valid()) {
    return Boot::Import();
  }

  if (!registry.require_installed(cursor, definition.get_kind())) {
    cursor.recover_to_statement();
    return Boot::Import();
  }

  if (!cursor.require(
          Class::Type::Assign, "Expected `=` after import ISA."_view)) {
    cursor.recover_to_statement();
    return Boot::Import();
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
    auto name = Package::PackageName::evaluate(cursor);
    if (!name.is_valid()) {
      cursor.recover_to_statement();
      return Boot::Import();
    }

    import_name = name.get_name();
    break;
  }

  default:
    cursor.error(
        "Unknown import semantics. Expected either a string path or a package "
        "name."_view);
    return Boot::Import();
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after import."_view)) {
    cursor.recover_to_statement();
  }

  return {definition.get_name(), import_name, definition.get_kind(), package};
}
