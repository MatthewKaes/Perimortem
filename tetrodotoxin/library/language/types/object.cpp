// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/object.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Object::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Object);
  Archive::Declaration declaration(get_definition());
  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Interface;
  BAIL_IF(
      !declaration.write(writer) ||
      !persist_declarations(writer, public_only) || !writer.finish(record));
  return True;
}

auto Types::Object::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Option<Object&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Object) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  BAIL_IF(!declaration);

  auto& definition = declaration->create_definition(arena, host);
  Object& object = arena.construct_from<Object>(
      [&]() -> Object { return Object(arena, definition, False); });
  BAIL_IF(!object.restore_declarations(contents, profile));
  object.complete_field_layout();
  return object;
}

static auto select_accessible_field(
    const Abstract& candidate,
    Option<const Abstract&> access_scope) -> Option<const Field&> {
  auto field = candidate.select<Field>();
  BAIL_IF(!field || field->get_writability() != Writability::Internal);

  const Abstract& host = access_scope.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& selected =
      field->get_host()
          .resolve_type_access(
              host, field->get_name(), Model::Type::Access::Self)
          .resolve();
  BAIL_IF(&selected != &*field);

  // Object construction shares ordinary receiver visibility. This admits
  // published state plus private state reached from a hosted descendant while
  // excluding Static and const Fields from the construction input surface.
  return *field;
}

static auto select_supplied(
    const Field& field,
    Model::Pack& arguments,
    View::Vector<Reference<const Abstract>> fitted_fields)
    -> Option<const Model::Pack&> {
  const Layout& inputs = arguments.get_layout();
  for (Count index = 0; index < fitted_fields.get_size(); index++) {
    if (&fitted_fields.get_data()[index].get() == &field) {
      return inputs.get_abstract(index).visit(
          []() -> Option<const Model::Pack&> { return {}; },
          [](const Abstract& selected) {
            return selected.select<Model::Pack>();
          });
    }
  }
  return {};
}

Types::Object::Object(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Bool provides_initialization)
    : Structure(domain, definition, provides_initialization) {}

auto Types::Object::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Object&> {
  Allocator::Arena& domain = cursor.get_arena();
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Object definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Objects accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Objects do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Object,
      "Library Object definitions require the `object` qualifier."_view);
  BAIL_IF(!kind_token);

  Object& object = domain.construct_from<Object>(
      [&]() -> Object { return Object(domain, definition); });
  BAIL_IF(!object.interpret_body(cursor, definition, kind_token));
  return object;
}

auto Types::Object::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  // Object owns this override so managed identity cannot become an accidental
  // invariant of inline Structure construction. The retained initialization
  // Pack remains the semantic input to the runtime allocation boundary.
  return Structure::create_default(arena);
}

auto Types::Object::create_supplied(
    Cursor& cursor,
    Model::Pack& arguments,
    Option<const Abstract&> access_scope,
    Option<Anchor> anchor) const -> Option<Model::Pack&> {
  Allocator::Arena& arena = cursor.get_arena();
  const Layout& inputs = arguments.get_layout();

  Managed::Vector<Reference<const Abstract>> accessible_fields(arena);
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto selected_field = select_accessible_field(selected.get(), access_scope);
    if (selected_field) {
      accessible_fields.insert(*selected_field);
    }
  }
  Layouts::Fluid accessible_layout(accessible_fields.get_view());

  Managed::Vector<Reference<const Abstract>> fitted_fields(arena);
  fitted_fields.reset(inputs.get_size());

  // Inputs retain evaluation order, while this fitted Field sequence records
  // which declaration owns each named value. The final Pack is assembled in
  // authored Field order so source argument order cannot alter Object layout.
  for (Count input_index = 0; input_index < inputs.get_size(); input_index++) {
    for (Count field_index = 0; field_index < accessible_fields.get_size();
         field_index++) {
      if (inputs.fits_entry(accessible_layout, input_index, field_index)) {
        fitted_fields.insert(accessible_fields.at(field_index));
        break;
      }
    }
  }

  Layouts::Fluid target_layout(fitted_fields.get_view());
  if (!arguments.fits(target_layout)) {
    cursor.create_expression_error(
        anchor,
        "Object initializer inputs do not fit the initialization Layout."_view,
        "Use unique accessible Fields with values accepted by their Types."_view);
    return {};
  }

  if (!owns_initialization()) {
    return Expressions::Initializer::create_provider(arena, *this, arguments);
  }

  // Object assembles only its owned mutable instance Fields in authored order.
  // A fitted supplied value wins, then the declaration initializer, then the
  // exact Field Type default. Const and Static facts never enter this inventory
  // and therefore cannot become construction inputs by accident.
  Managed::Vector<Reference<Model::Pack>> values(arena);
  values.reset(get_layout().get_size());
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = selected.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    auto supplied =
        select_supplied(*field, arguments, fitted_fields.get_view());
    if (supplied) {
      values.insert(const_cast<Model::Pack&>(*supplied));
      continue;
    }

    auto authored = field->get_initializer();
    if (authored) {
      values.insert(const_cast<Model::Pack&>(*authored));
      continue;
    }

    auto fallback = field->get_type().create_default(arena);
    if (!fallback) {
      cursor.create_expression_error(
          anchor,
          "Object initializer cannot complete one omitted state Field."_view,
          "Use a completed Field Type with a semantic default or supply an "
          "exact Field value."_view);
      return {};
    }
    values.insert(*fallback);
  }

  return Model::Pack::create_group(arena, values.get_view());
}

auto Types::Object::create_supplied_restored(
    Allocator::Arena& arena,
    Model::Pack& arguments,
    Option<const Abstract&> access_scope) const -> Option<Model::Pack&> {
  const Layout& inputs = arguments.get_layout();
  Managed::Vector<Reference<const Abstract>> accessible_fields(arena);
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = select_accessible_field(selected.get(), access_scope);
    if (field) {
      accessible_fields.insert(*field);
    }
  }
  Layouts::Fluid accessible_layout(accessible_fields.get_view());

  Managed::Vector<Reference<const Abstract>> fitted_fields(arena);
  fitted_fields.reset(inputs.get_size());
  for (Count input_index = 0; input_index < inputs.get_size(); input_index++) {
    for (Count field_index = 0; field_index < accessible_fields.get_size();
         field_index++) {
      if (inputs.fits_entry(accessible_layout, input_index, field_index)) {
        fitted_fields.insert(accessible_fields.at(field_index));
        break;
      }
    }
  }
  Layouts::Fluid target_layout(fitted_fields.get_view());
  BAIL_IF(!arguments.fits(target_layout));

  if (!owns_initialization()) {
    return Expressions::Initializer::create_provider(arena, *this, arguments);
  }

  Managed::Vector<Reference<Model::Pack>> values(arena);
  values.reset(get_layout().get_size());
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = selected.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    auto supplied =
        select_supplied(*field, arguments, fitted_fields.get_view());
    if (supplied) {
      values.insert(const_cast<Model::Pack&>(*supplied));
      continue;
    }
    auto authored = field->get_initializer();
    if (authored) {
      values.insert(const_cast<Model::Pack&>(*authored));
      continue;
    }
    auto fallback = field->get_type().create_default(arena);
    BAIL_IF(!fallback);
    values.insert(*fallback);
  }
  return Model::Pack::create_group(arena, values.get_view());
}
