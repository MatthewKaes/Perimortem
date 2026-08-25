// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/graphics/descriptor.hpp"

namespace Tetrodotoxin::Graphics {

// CompiledDescriptor adapts one Terminal generated child access function to
// the stable Descriptor protocol. The function already contains authored Field
// order and physical offsets, so this runtime owner retains only the configured
// Type behaviors needed to interpret its returned index.
class CompiledDescriptor {
 public:
  using ReadChild = Count (*)(void*, Count, void**);

  CompiledDescriptor(
      Count child_count,
      ReadChild read_child,
      Perimortem::Core::View::Vector<const Descriptor*> types);

  CompiledDescriptor(const CompiledDescriptor&) = delete;
  CompiledDescriptor(CompiledDescriptor&&) = delete;
  auto operator=(const CompiledDescriptor&) -> CompiledDescriptor& = delete;
  auto operator=(CompiledDescriptor&&) -> CompiledDescriptor& = delete;

  constexpr auto get_descriptor() const -> const Descriptor& {
    return descriptor;
  }

 private:
  static auto read_placement(
      const U8* product,
      Perimortem::Core::Object<> object) -> Descriptor::Placement;
  static auto read_child_count(
      const U8* product,
      Perimortem::Core::Object<> object) -> Count;
  static auto read_selected_child(
      const U8* product,
      Perimortem::Core::Object<> object,
      Count index) -> Descriptor::Child;

  Count retained_child_count;
  ReadChild retained_read_child;
  Perimortem::Core::View::Vector<const Descriptor*> retained_types;
  Descriptor descriptor;
};

}  // namespace Tetrodotoxin::Graphics
