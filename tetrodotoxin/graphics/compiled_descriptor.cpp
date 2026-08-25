// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/compiled_descriptor.hpp"

#include "perimortem/core/data.hpp"

using namespace Tetrodotoxin;

Graphics::CompiledDescriptor::CompiledDescriptor(
    Count child_count,
    Graphics::CompiledDescriptor::ReadChild read_child,
    Perimortem::Core::View::Vector<const Graphics::Descriptor*> types)
    : retained_child_count(child_count),
      retained_read_child(read_child),
      retained_types(types),
      descriptor(
          Perimortem::Core::Data::cast<const U8>(this),
          read_placement,
          read_child_count,
          read_selected_child,
          nullptr,
          nullptr) {}

auto Graphics::CompiledDescriptor::read_placement(
    const U8*,
    Perimortem::Core::Object<>) -> Graphics::Descriptor::Placement {
  return Descriptor::Placement();
}

auto Graphics::CompiledDescriptor::read_child_count(
    const U8* product,
    Perimortem::Core::Object<> object) -> Count {
  const auto* selected =
      Perimortem::Core::Data::cast<const Graphics::CompiledDescriptor>(product);
  return object.is_empty() || !selected->retained_read_child
             ? 0
             : selected->retained_child_count;
}

auto Graphics::CompiledDescriptor::read_selected_child(
    const U8* product,
    Perimortem::Core::Object<> object,
    Count index) -> Graphics::Descriptor::Child {
  const auto* selected =
      Perimortem::Core::Data::cast<const Graphics::CompiledDescriptor>(product);
  if (object.is_empty() || !selected->retained_read_child ||
      index >= selected->retained_child_count) {
    return {};
  }

  void* child = nullptr;
  Count type_index =
      selected->retained_read_child(object.get_payload(), index, &child);
  if (!child || type_index >= selected->retained_types.get_size()) {
    return {};
  }
  const Graphics::Descriptor* descriptor =
      selected->retained_types.get_data()[type_index];
  return descriptor ? Graphics::Descriptor::Child(
                          Perimortem::Core::Object<>(
                              Perimortem::Core::Data::cast<U8>(child)),
                          *descriptor)
                    : Graphics::Descriptor::Child();
}
