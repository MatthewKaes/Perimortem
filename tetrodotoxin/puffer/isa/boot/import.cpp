// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/isa/boot/import.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/package/package_name.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer;
using namespace Ttx::Lexical;

auto Isa::Boot::Import::evaluate(
    Cursor& cursor,
    const Tetrodotoxin::Isa::Registry& registry) -> Isa::Boot::Import {
  Tetrodotoxin::Isa::Base::Declaration definition =
      Tetrodotoxin::Isa::Base::Declaration::evaluate(
          cursor, Ttx::Documentation(), {{Code::Type::Import}},
          {{Code::Type::Type}}, {{Code::Type::Type}});
  if (definition.is_empty()) {
    return Isa::Boot::Import();
  }

  Bool installed = registry.require_installed(cursor, definition.get_kind());
  if (!installed) {
    cursor.recover_to_statement();
    return Isa::Boot::Import();
  }

  Bool has_assignment =
      cursor.require(Code::Type::Assign, "Expected `=` after import ISA."_view);
  if (!has_assignment) {
    cursor.recover_to_statement();
    return Isa::Boot::Import();
  }

  // Strings are file imports. Qualified type-shaped names are package imports.
  View::Bytes import_name;
  Bool package = False;
  switch (cursor.current().get_code().get_type()) {
  case Code::Type::String: {
    auto source_text = cursor.current().get_text();
    import_name = source_text.slice(1, source_text.get_size() - 2);
    cursor.consume();
    break;
  }

  case Code::Type::Type: {
    package = True;
    auto name = Tetrodotoxin::Isa::Package::PackageName::evaluate(cursor);
    if (name.is_empty()) {
      cursor.recover_to_statement();
      return Isa::Boot::Import();
    }

    import_name = name.get_name();
    break;
  }

  default:
    cursor.error(
        "Unknown import semantics. Expected either a string path or a package "
        "name."_view);
    return Isa::Boot::Import();
  }

  Bool has_statement_end = cursor.require(
      Code::Type::EndStatement, "Expected `;` after import."_view);
  if (!has_statement_end) {
    cursor.recover_to_statement();
  }

  return {definition.get_name(), import_name, definition.get_kind(), package};
}
