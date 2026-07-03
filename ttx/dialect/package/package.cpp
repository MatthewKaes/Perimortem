// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/package/package.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/dialect/lingua_franca.hpp"
#include "ttx/dialect/package/export.hpp"
#include "ttx/dialect/symbol_path.hpp"
#include "ttx/dialect/source/documentation.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Dialect;
using namespace Ttx::Lexical;

const static LinguaFranca::Registration registration(
    Package::Package::get_name(),
    Package::Package::parse);

auto Package::Package::find_type(View::Bytes name) const -> const Ttx::Type* {
  for (Count type_index = 0; type_index < types.get_size(); type_index++) {
    if (types[type_index] != nullptr && types[type_index]->get_name() == name) {
      return types[type_index];
    }
  }

  return nullptr;
}

auto Package::Package::parse(Cursor& cursor) -> void* {
  Ttx::Documentation package_documentation;
  View::Bytes package_name;
  Managed::Vector<Export> exports(cursor.get_arena());
  Managed::Vector<const Ttx::Type*> types(cursor.get_arena());

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Source::Documentation::parse(cursor);
    const Token& token = cursor.current();
    if (token.get_class() == Class::Type::Attribute &&
        token.get_text() == "@package_name"_view) {
      package_documentation = documentation;
      cursor.consume();
      if (!cursor.require(
              Class::Type::Assign, "Expected `=` after @package_name."_view)) {
        return nullptr;
      }

      SymbolPath path = SymbolPath::parse(cursor);
      if (!path.is_valid()) {
        return nullptr;
      }
      package_name = path.get_text();

      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after package name."_view)) {
        return nullptr;
      }
      continue;
    }

    if (cursor.matches(Class::Type::ConstPublic)) {
      Export export_ = Export::parse(cursor, documentation);
      if (!export_.is_valid()) {
        return nullptr;
      }
      exports.insert(export_);
      continue;
    }

    cursor.token_error("Expected package directive or export."_view);
    return nullptr;
  }

  if (package_name.is_empty()) {
    cursor.error("Package source must declare `@package_name`."_view);
    return nullptr;
  }

  return &cursor.get_arena().construct<Package>(
      package_documentation, package_name, exports.get_view(),
      types.get_view());
}
