// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/package.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/package/export.hpp"
#include "tetrodotoxin/isa/qualified_name.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto last_qualified_segment(View::Bytes name) -> View::Bytes {
  Count segment_start = 0;
  for (Count name_index = 0; name_index + 1 < name.get_size(); name_index++) {
    if (name[name_index] == ':' && name[name_index + 1] == ':') {
      segment_start = name_index + 2;
      name_index++;
    }
  }

  return name.slice(segment_start, name.get_size() - segment_start);
}

static auto build_export_type(Context& context, const Export& export_)
    -> const Ttx::Type* {
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count export_index = 0; export_index < export_.get_exports().get_size();
       export_index++) {
    const Ttx::Type* nested =
        build_export_type(context, export_.get_exports()[export_index]);
    if (nested != nullptr) {
      types.insert(nested);
    }
  }

  const Definition& definition = export_.get_definition();
  if (definition.is_namespace()) {
    return &context.get_arena().construct<Ttx::Type>(
        definition.get_name(), View::Vector<Ttx::Type::Member>(),
        types.get_view(), View::Vector<Ttx::Type::Function>(),
        definition.get_documentation());
  }

  const Ttx::Type* target = context.resolve_type(export_.get_target().get_text());
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

auto Package::evaluate(Context& context, Cursor& cursor) -> Bool {
  Ttx::Documentation package_documentation;
  View::Bytes package_name;
  Managed::Vector<Export> exports(cursor.get_arena());

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    const Token& token = cursor.current();
    // `@package_name` is package metadata. Other package body entries are
    // exports owned by the Package ISA.
    if (token.get_class() == Class::Type::Attribute &&
        token.get_text() == "@package_name"_view) {
      package_documentation = documentation;
      cursor.consume();
      if (!cursor.require(
              Class::Type::Assign, "Expected `=` after @package_name."_view)) {
        return False;
      }

      QualifiedName name = QualifiedName::evaluate(cursor);
      if (!name.is_valid()) {
        return False;
      }
      package_name = name.get_text();

      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after package name."_view)) {
        return False;
      }
      continue;
    }

    if (cursor.matches(Class::Type::ConstPublic)) {
      Export export_ = Export::evaluate(cursor, documentation);
      if (!export_.is_valid()) {
        return False;
      }
      exports.insert(export_);
      continue;
    }

    cursor.token_error("Expected package directive or export."_view);
    return False;
  }

  if (package_name.is_empty()) {
    cursor.error("Package source must declare `@package_name`."_view);
    return False;
  }

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  for (Count export_index = 0; export_index < exports.get_size();
       export_index++) {
    const Ttx::Type* type = build_export_type(context, exports[export_index]);
    if (type != nullptr) {
      types.insert(type);
    }
  }

  auto& package_type = context.get_arena().construct<Ttx::Type>(
      last_qualified_segment(package_name), View::Vector<Ttx::Type::Member>(),
      types.get_view(), View::Vector<Ttx::Type::Function>(),
      package_documentation);
  context.publish(package_type, package_name);
  return True;
}
