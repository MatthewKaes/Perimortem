// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/children_2d.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::Children2D::child_count(U8* object) const -> Count {
  return object != nullptr && read_count != nullptr
             ? read_count(context, object)
             : 0;
}

auto Runtime::Children2D::child(U8* object, Count index) const
    -> Option<Child> {
  BAIL_IF(object == nullptr || read == nullptr || index >= child_count(object));
  U8* selected = nullptr;
  Count type_index = read(context, object, index, &selected);
  BAIL_IF(selected == nullptr || type_index == Count(-1));
  return Child(selected, type_index);
}
