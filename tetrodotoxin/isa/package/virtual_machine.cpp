// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/virtual_machine.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/package/export.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto build_export_type(
    Base::Context& context,
    View::Bytes parent_path,
    const Package::Export& export_) -> const Ttx::Type* {
  const Base::Declaration& definition = export_.get_definition();
  Managed::Bytes display_name(context.get_arena(), parent_path);
  if (!display_name.get_view().is_empty()) {
    display_name.concat("::"_view);
  }

  display_name.concat(definition.get_name());

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count i = 0; i < export_.get_exports().get_size(); i++) {
    const Ttx::Type* nested = build_export_type(
        context, display_name.get_view(), export_.get_exports()[i]);
    types.insert(nested);
  }

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({Ttx::Type::display_name_attribute, display_name});
  if (definition.get_kind() == "group"_view) {
    return &context.get_arena().construct<Ttx::Type>(
        definition.get_name(), View::Vector<Ttx::Member>(), types.get_view(),
        View::Vector<Ttx::Function>(), definition.get_documentation(),
        attributes.get_view());
  }

  return &context.get_arena().construct<Ttx::Type>(Ttx::Type::alias(
      definition.get_name(), *export_.get_target(),
      definition.get_documentation(), attributes.get_view()));
}

auto Package::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  View::Bytes package_name = context.get_package_name();
  if (package_name.is_empty()) {
    cursor.error("Package compilation requires a package name."_view);
    return nullptr;
  }

  Managed::Vector<Package::Export> exports(cursor.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    if (cursor.matches(Class::Type::Expose)) {
      Package::Export export_ =
          Package::Export::evaluate(cursor, context, documentation);
      if (!export_.is_valid()) {
        return nullptr;
      }

      if (Package::Export::contains_name(
              exports.get_view(), export_.get_definition().get_name())) {
        cursor.token_error("Package export name is already defined."_view);
        return nullptr;
      }

      exports.insert(export_);
      continue;
    }

    cursor.token_error("Expected package export."_view);
    return nullptr;
  }

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count i = 0; i < exports.get_size(); i++) {
    types.insert(build_export_type(context, package_name, exports[i]));
  }

  Managed::Vector<Ttx::Attribute> package_attributes(context.get_arena());
  package_attributes.insert({Ttx::Type::display_name_attribute, package_name});

  auto& package_name_type =
      context.get_arena().construct<Ttx::Type>(package_name);
  Managed::Vector<Ttx::Member> members(context.get_arena());
  members.insert(Ttx::Member("package_name"_view, package_name_type));

  auto& package_type = context.get_arena().construct<Ttx::Type>(
      Package::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      View::Vector<Ttx::Function>(), Ttx::Documentation(),
      package_attributes.get_view());
  return &package_type;
}
