// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/dialect/shader/facts.hpp"

#include "perimortem/core/reader/textual.hpp"

#include "tetrodotoxin/resolution/import_scope.hpp"
#include "tetrodotoxin/syntax/expression/literal.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Dialect::Shader;

static auto parse_count(View::Bytes text, Count* out) -> Bool {
  if (!out || text.is_empty()) {
    return False;
  }

  Reader::Textual reader(text);
  Bits_64 result = reader.read_unsigned();
  if (!reader.is_valid() || !reader.is_empty()) {
    return False;
  }

  *out = Count(result);
  return True;
}

static auto find_field(
    View::Vector<Syntax::Pack::Field> fields,
    View::Bytes name) -> const Syntax::Pack::Field* {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name == name) {
      return &fields[i];
    }
  }

  return nullptr;
}

static auto find_attribute(
    View::Vector<Syntax::Ast::Attribute> attributes,
    View::Bytes name) -> const Syntax::Ast::Attribute* {
  for (Count i = 0; i < attributes.get_size(); i++) {
    if (attributes[i].get_name() == name) {
      return &attributes[i];
    }
  }

  return nullptr;
}

static auto align_to(Count offset, Count alignment) -> Count {
  if (alignment <= 1) {
    return offset;
  }

  Count remainder = offset % alignment;
  if (remainder == 0) {
    return offset;
  }

  return offset + alignment - remainder;
}

static auto source_type_name(const Syntax::Type::Ref& type) -> View::Bytes {
  View::Vector<View::Bytes> path = type.get_name_path();
  return path.is_empty() ? View::Bytes() : path[path.get_size() - 1];
}

static auto type_metadata_name(
    const Resolution::Record& record,
    const Syntax::Type::Ref& type) -> View::Bytes {
  Ttx::StandardPackage package =
      Resolution::ImportScope::standard_package_for_type(record, type);
  return package.exists()
             ? Resolution::ImportScope::standard_type_name(record, type)
             : source_type_name(type);
}

static auto parse_builtin(View::Vector<Syntax::Ast::Attribute> attributes)
    -> Tetrodotoxin::Dialect::Shader::Builtin {
  Tetrodotoxin::Dialect::Shader::Builtin result;
  const Syntax::Ast::Attribute* attr =
      find_attribute(attributes, "builtin"_view);
  if (!attr) {
    return result;
  }

  result.present = True;

  const auto* name = find_field(attr->get_fields(), "name"_view);
  if (name && name->value) {
    result.name =
        Syntax::Expression::Literal::inner_text(name->value->get_text());
  }

  const auto* slot = find_field(attr->get_fields(), "slot"_view);
  if (slot && slot->value) {
    Count parsed = 0;
    if (parse_count(slot->value->get_text(), &parsed)) {
      result.has_slot = True;
      result.slot = parsed;
    }
  }

  return result;
}

static auto find_function(
    View::Vector<Syntax::Ast::Function*> functions,
    View::Bytes name) -> const Syntax::Ast::Function* {
  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i] && functions[i]->get_definition().get_name() == name) {
      return functions[i];
    }
  }

  return nullptr;
}

static auto append_layout_fields(
    const Facts& shader,
    const Syntax::Ast::Layout& layout,
    Dynamic::Vector<InterfaceField>& out) -> void {
  if (layout.is_named()) {
    View::Vector<Syntax::Ast::Param> params = layout.get_params();
    for (Count i = 0; i < params.get_size(); i++) {
      out.insert({
        params[i].get_name(),
        &params[i].get_type(),
        shader.builtin_of(params[i]),
        out.get_size(),
        True,
      });
    }
    return;
  }

  if (layout.is_unnamed()) {
    View::Vector<Syntax::Type::Ref> types = layout.get_types();
    for (Count i = 0; i < types.get_size(); i++) {
      out.insert({
        View::Bytes(),
        &types[i],
        Tetrodotoxin::Dialect::Shader::Builtin(),
        out.get_size(),
        False,
      });
    }
    return;
  }

  const Syntax::Type::Ref& value = layout.get_value_type();
  if (!value.is_empty()) {
    out.insert({
      View::Bytes(),
      &value,
      Tetrodotoxin::Dialect::Shader::Builtin(),
      out.get_size(),
      False,
    });
  }
}

auto Facts::entry_points(const Resolution::Record& record) const
    -> Dynamic::Vector<EntryPoint> {
  Dynamic::Vector<EntryPoint> result;
  View::Vector<Syntax::Type::Declaration*> types =
      record.get_package().get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i]) {
      continue;
    }

    Stage stage = stage_of(*types[i]);
    if (stage == Stage::None) {
      continue;
    }

    result.insert({
      types[i],
      find_function(types[i]->get_scope_functions(), "main"_view),
      stage,
    });
  }

  return result;
}

auto Facts::interface_of(const Syntax::Ast::Function& function) const
    -> Interface {
  Interface result;
  result.function = &function;
  append_layout_fields(*this, function.get_params(), result.inputs);
  append_layout_fields(*this, function.get_returns(), result.outputs);
  return result;
}

auto Facts::push_constants(const Resolution::Record& record) const
    -> PushConstantMetadata {
  PushConstantMetadata result;
  View::Vector<Syntax::Type::Declaration*> types =
      record.get_package().get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i] || types[i]->is_disabled() || !is_push_constant(*types[i])) {
      continue;
    }

    PushConstantBlock block;
    block.name = types[i]->get_definition().get_name();
    block.type = types[i];
    block.field_start = result.fields.get_size();

    Count offset = 0;
    View::Vector<Syntax::Ast::Member*> members = types[i]->get_scope();
    for (Count j = 0; j < members.get_size(); j++) {
      if (!members[j] || members[j]->is_disabled()) {
        continue;
      }

      const Syntax::Ast::Definition& definition = members[j]->get_definition();
      const Syntax::Type::Ref& field_type = definition.get_type_ref();
      Ttx::StandardPackage package =
          Resolution::ImportScope::standard_package_for_type(
              record, field_type);
      Ttx::TypeQuery field_query =
          Resolution::ImportScope::standard_type_query(record, field_type);
      Ttx::Layout field_layout = package.type_layout(field_query);
      Count field_size =
          field_layout.is_valid() ? field_layout.get_byte_size() : 0;
      Count field_alignment =
          field_layout.is_valid() ? field_layout.get_alignment() : 1;

      offset = align_to(offset, field_alignment);
      result.fields.insert({
        definition.get_name(),
        &field_type,
        type_metadata_name(record, field_type),
        offset,
        field_size,
        field_alignment,
        members[j],
      });

      offset += field_size;
      if (field_alignment > block.alignment) {
        block.alignment = field_alignment;
      }
    }

    block.byte_size = align_to(offset, block.alignment);
    block.field_count = result.fields.get_size() - block.field_start;
    result.blocks.insert(block);
  }

  return result;
}

auto Facts::descriptors(const Resolution::Record& record) const
    -> Dynamic::Vector<DescriptorBinding> {
  Dynamic::Vector<DescriptorBinding> result;
  View::Vector<Syntax::Type::Declaration*> types =
      record.get_package().get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i] || types[i]->is_disabled() || !is_entry_point(*types[i])) {
      continue;
    }

    View::Vector<Syntax::Ast::Member*> members = types[i]->get_scope();
    for (Count j = 0; j < members.get_size(); j++) {
      if (!members[j] || members[j]->is_disabled()) {
        continue;
      }

      Descriptor descriptor = descriptor_of(*members[j]);
      if (!descriptor.present) {
        continue;
      }

      const Syntax::Ast::Definition& definition = members[j]->get_definition();
      const Syntax::Type::Ref& type = definition.get_type_ref();
      result.insert({
        definition.get_name(),
        &type,
        type_metadata_name(record, type),
        descriptor.set,
        descriptor.slot,
        types[i],
        members[j],
      });
    }
  }

  return result;
}

auto Facts::stage_of(const Syntax::Type::Declaration& type) const -> Stage {
  const Syntax::Ast::Attribute* attr =
      find_attribute(type.get_attributes(), "stage"_view);
  if (!attr) {
    return Stage::None;
  }

  const auto* field = find_field(attr->get_fields(), "kind"_view);
  if (!field || !field->value) {
    return Stage::None;
  }

  View::Bytes kind =
      Syntax::Expression::Literal::inner_text(field->value->get_text());
  if (kind == "vertex"_view) {
    return Stage::Vertex;
  }
  if (kind == "pixel"_view || kind == "fragment"_view) {
    return Stage::Pixel;
  }

  return Stage::None;
}

auto Facts::is_entry_point(const Syntax::Type::Declaration& type) const
    -> Bool {
  return stage_of(type) != Stage::None;
}

auto Facts::is_push_constant(const Syntax::Type::Declaration& type) const
    -> Bool {
  return find_attribute(type.get_attributes(), "push_constant"_view) != nullptr;
}

auto Facts::descriptor_of(const Syntax::Ast::Member& member) const
    -> Descriptor {
  Descriptor result;
  const Syntax::Ast::Attribute* attr =
      find_attribute(member.get_attributes(), "binding"_view);
  if (!attr) {
    return result;
  }

  const auto* set = find_field(attr->get_fields(), "set"_view);
  const auto* slot = find_field(attr->get_fields(), "slot"_view);
  if (!set || !slot || !set->value || !slot->value) {
    return result;
  }

  Count parsed_set = 0;
  Count parsed_slot = 0;
  if (!parse_count(set->value->get_text(), &parsed_set) ||
      !parse_count(slot->value->get_text(), &parsed_slot)) {
    return result;
  }

  result.present = True;
  result.set = parsed_set;
  result.slot = parsed_slot;
  return result;
}

auto Facts::builtin_of(const Syntax::Ast::Member& member) const -> Builtin {
  return parse_builtin(member.get_attributes());
}

auto Facts::builtin_of(const Syntax::Ast::Param& param) const -> Builtin {
  return parse_builtin(param.get_attributes());
}

auto Tetrodotoxin::Dialect::Shader::stage_name(Stage stage) -> View::Bytes {
  switch (stage) {
  case Stage::Vertex:
    return "vertex"_view;
  case Stage::Pixel:
    return "pixel"_view;
  case Stage::None:
    return "none"_view;
  }

  return "unknown"_view;
}
