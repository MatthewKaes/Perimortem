// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/build/monograph.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

Build::Monograph::Monograph(
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& language,
    const Ttx::Concept::Documentation& documentation,
    Ttx::Concept::Abstract& context,
    View::Bytes source_path,
    const Environment::Plan& environment,
    Perimortem::Memory::Managed::Vector<ProductDescription> products)
    : Language::Monograph(arena, language, documentation, context),
      source_path(source_path),
      environment(environment),
      products(products) {}

auto Build::Monograph::retain_import(
    const Language::Import::Description& description,
    Option<Ttx::Lexical::Associations&> associations) -> Bool {
  if (description.get_name() != "Product"_view ||
      description.get_kind() != Language::Import::Kind::Source ||
      description.get_visibility() != Language::Visibility::Private ||
      !description.get_route().is_empty() || !package_locator.is_empty()) {
    return False;
  }
  package_locator = description.get_locator();
  if (associations) {
    associations->create(description.get_declaration_anchor(), *this);
    associations->create(description.get_expression_anchor(), *this);
  }
  return True;
}

auto Build::Monograph::link(Ttx::Lexical::Cursor& cursor) -> Bool {
  Bool valid = True;
  if (package_locator.is_empty()) {
    cursor.create_error(
        "Build requires one private Product source locator."_view,
        "Declare `private Product : alias = source(\"package.ttx\");`."_view);
    valid = False;
  }
  if (products.is_empty()) {
    cursor.create_error(
        "Build requires at least one public product request."_view,
        "Map one exported Package identity to an explicit Terminal export."_view);
    valid = False;
  }
  return valid;
}
