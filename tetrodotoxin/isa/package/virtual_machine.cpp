// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/package/export.hpp"
#include "tetrodotoxin/isa/package/package_name.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto build_export_type(Context& context, const Package::Export& export_)
    -> const Ttx::Type* {
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count i = 0; i < export_.get_exports().get_size(); i++) {
    const Ttx::Type* nested =
        build_export_type(context, export_.get_exports()[i]);
    if (nested != nullptr) {
      types.insert(nested);
    }
  }

  const Definition& definition = export_.get_definition();
  if (definition.get_kind() == "group"_view) {
    return &context.get_arena().construct<Ttx::Type>(
        definition.get_name(), View::Vector<Ttx::Type::Member>(),
        types.get_view(), View::Vector<Ttx::Type::Function>(),
        definition.get_documentation());
  }

  const Ttx::Type* target = export_.get_target();
  if (target != nullptr) {
    return &context.get_arena().construct<Ttx::Type>(
        Ttx::Type::alias(
            definition.get_name(), *target, definition.get_documentation()));
  }

  // TODO: Link this export to the real nested Ttx::Type once every producer
  // ISA publishes enough type facts for the full `::` query to resolve. The
  // package and its imports are resolved here. This fallback only exists
  // because Library, Shader, and the other young ISAs may still publish a
  // shallow root type, so targets like `Shaders::Default2D` cannot always walk
  // to the inner type yet. Keep the authored package surface visible without
  // pretending alias identity was proven.
  return &context.get_arena().construct<Ttx::Type>(
      definition.get_name(), definition.get_documentation());
}

auto Package::VirtualMachine::evaluate(Cursor& cursor, Context& context)
    -> Ttx::Type* {
  Ttx::Documentation package_documentation;
  View::Bytes package_name;
  Managed::Vector<Package::Export> exports(cursor.get_arena());

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    if (cursor.current().get_class() == Class::Type::Attribute &&
        cursor.current().get_text() == "@package_name"_view) {
      if (!package_name.is_empty()) {
        cursor.token_error(
            "Package source already declared `@package_name`."_view);
        return nullptr;
      }

      cursor.consume();
      if (!cursor.require(
              Class::Type::Assign, "Expected `=` after @package_name."_view)) {
        return nullptr;
      }

      Package::PackageName name =
          Package::PackageName::evaluate(cursor, documentation);
      if (!name.is_valid()) {
        return nullptr;
      }

      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after package name."_view)) {
        return nullptr;
      }

      package_documentation = name.get_documentation();
      package_name = name.get_name();
      continue;
    }

    if (cursor.matches(Class::Type::Expose)) {
      Package::Export export_ =
          Package::Export::evaluate(cursor, context, documentation);
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

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count i = 0; i < exports.get_size(); i++) {
    const Ttx::Type* type = build_export_type(context, exports[i]);
    if (type != nullptr) {
      types.insert(type);
    }
  }

  auto& package_name_type = context.get_arena().construct<Ttx::Type>(
      package_name, package_documentation);
  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  members.insert(Ttx::Type::Member("package_name"_view, package_name_type));

  auto& package_type = context.get_arena().construct<Ttx::Type>(
      Package::VirtualMachine::get_name(), members.get_view(),
      types.get_view(), View::Vector<Ttx::Type::Function>(),
      package_documentation);
  return &package_type;
}
