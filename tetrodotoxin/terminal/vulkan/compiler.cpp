// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/vulkan/compiler.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/shader/language/binding.hpp"
#include "tetrodotoxin/terminal/spirv/layout.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static auto attribute(
    Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    Core::View::Bytes name)
    -> Core::Option<const Tetrodotoxin::Language::Attribute&> {
  for (Count index = 0; index < attributes.get_size(); index++) {
    const Tetrodotoxin::Language::Attribute& selected =
        attributes.get_data()[index];
    if (selected.get_key() == name) {
      return selected;
    }
  }
  return {};
}

static auto unsigned_attribute(
    Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    Core::View::Bytes name) -> Core::Option<Count> {
  auto selected = attribute(attributes, name);
  const U64* value = selected ? selected->get_value().find<U64>() : nullptr;
  BAIL_IF(!value || *value > U64(Count(-1)));
  return Count(*value);
}

static auto stage_of(const Library::Language::Function& function)
    -> Core::Option<Terminal::Vulkan::Products::Stage> {
  Core::View::Bytes name = function.get_name();
  auto capability =
      attribute(function.get_definition().get_attributes(), "capability"_view);
  if (capability) {
    const Core::View::Bytes* selected =
        capability->get_value().find<Core::View::Bytes>();
    BAIL_IF(!selected);
    name = *selected;
  }
  if (name == "vertex"_view) {
    return Terminal::Vulkan::Products::Stage::Vertex;
  }
  if (name == "fragment"_view) {
    return Terminal::Vulkan::Products::Stage::Pixel;
  }
  return {};
}

static auto align_up(Count value, Count alignment) -> Count {
  return (value + alignment - 1) / alignment * alignment;
}

static auto select_type(const Ttx::Concept::Abstract& semantic)
    -> Core::Option<const Library::Language::Model::Type&> {
  auto addressable = semantic.select<Library::Language::Model::Addressable>();
  return addressable
             ? Core::Option<const Library::Language::Model::Type&>(
                   addressable->get_type())
             : semantic.resolve().select<Library::Language::Model::Type>();
}

auto Terminal::Vulkan::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Shader::Language::Program& program,
    Core::View::Bytes symbol) const -> Core::Option<Products> {
  BAIL_IF(symbol.is_empty() || !program.get_contract());

  Memory::Managed::Vector<Products::Entry> entries(arena);
  const Library::Language::Function* vertex = nullptr;
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
       program.get_callables()) {
    auto function = candidate.get().select<Library::Language::Function>();
    auto stage =
        function ? stage_of(*function) : Core::Option<Products::Stage>();
    BAIL_IF(!function || !stage);
    entries.insert(Products::Entry{*stage, function->get_name()});
    if (*stage == Products::Stage::Vertex) {
      BAIL_IF(vertex);
      vertex = &*function;
    }
  }
  BAIL_IF(entries.get_size() != 2 || !vertex);

  Memory::Managed::Vector<Products::Descriptor> descriptors(arena);
  Memory::Managed::Vector<Products::HostField> host_fields(arena);
  Count host_size = 0;
  for (const Shader::Language::Binding& binding : program.get_bindings()) {
    const Library::Language::Field& field = binding.get_field();
    auto attributes = field.get_definition().get_attributes();
    if (binding.get_kind() == Render::Language::Binding::Kind::Resource) {
      auto set = unsigned_attribute(attributes, "set"_view);
      auto slot = unsigned_attribute(attributes, "slot"_view);
      BAIL_IF(!set || !slot);
      descriptors.insert(Products::Descriptor{field.get_name(), *set, *slot});
      continue;
    }
    BAIL_IF(
        binding.get_kind() != Render::Language::Binding::Kind::Push ||
        host_size != 0);
    auto structure =
        field.get_type().select<Library::Language::Types::Structure>();
    BAIL_IF(!structure);
    Count offset = 0;
    Count structure_alignment = 1;
    for (Count index = 0; index < structure->get_layout().get_size(); index++) {
      auto semantic = structure->get_layout().get_abstract(index);
      auto addressable =
          semantic
              ? semantic->select<Library::Language::Model::Addressable>()
              : Core::Option<const Library::Language::Model::Addressable&>();
      auto layout =
          addressable
              ? Terminal::Spirv::Layout::measure(addressable->get_type())
              : Core::Option<Terminal::Spirv::Layout::Measurement>();
      BAIL_IF(!addressable || !layout);
      offset = align_up(offset, layout->get_alignment());
      host_fields.insert(
          Products::HostField{
            addressable->get_name(), offset, layout->get_size()});
      offset += layout->get_size();
      structure_alignment =
          Core::Math::max(structure_alignment, layout->get_alignment());
    }
    host_size = align_up(offset, structure_alignment);
  }
  BAIL_IF(descriptors.get_size() != 1 || host_size == 0);

  Memory::Managed::Vector<Products::VertexInput> vertex_inputs(arena);
  Count stride = 0;
  const auto& parameters = vertex->get_signature().get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto location = unsigned_attribute(
        parameters.get_slot_attributes(index), "location"_view);
    auto semantic = parameters.get_abstract(index);
    auto type = semantic
                    ? select_type(*semantic)
                    : Core::Option<const Library::Language::Model::Type&>();
    auto components =
        type ? Terminal::Spirv::Layout::get_vector_components(*type)
             : Core::Option<Count>();
    BAIL_IF(!location || !components);
    vertex_inputs.insert(
        Products::VertexInput{*location, *components, stride, 0});
    stride += *components * sizeof(R32);
  }
  for (Count index = 0; index < vertex_inputs.get_size(); index++) {
    vertex_inputs[index].stride = stride;
  }
  return Products(
      symbol, entries.get_view(), descriptors.get_view(),
      vertex_inputs.get_view(), host_fields.get_view(), host_size);
}
