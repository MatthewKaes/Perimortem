// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/descriptor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

auto Graphics::Descriptor::placement(Object<> object) const
    -> Option<Placement> {
  BAIL_IF(object.is_empty() || !read_placement);
  return read_placement(product, object);
}

auto Graphics::Descriptor::child_count(Object<> object) const -> Count {
  return !object.is_empty() && read_child_count
             ? read_child_count(product, object)
             : 0;
}

auto Graphics::Descriptor::child(Object<> object, Count index) const
    -> Option<Child> {
  BAIL_IF(object.is_empty() || !read_child || index >= child_count(object));
  Child selected = read_child(product, object, index);
  BAIL_IF(selected.get_object().is_empty() || !selected.get_descriptor());
  return selected;
}

auto Graphics::Descriptor::draw_count(Object<> object) const -> Count {
  return !object.is_empty() && read_draw_count
             ? read_draw_count(product, object)
             : 0;
}

auto Graphics::Descriptor::draw(Object<> object, Count index) const
    -> Option<Draw> {
  BAIL_IF(object.is_empty() || !read_draw || index >= draw_count(object));
  Draw selected = read_draw(product, object, index);
  BAIL_IF(
      !selected.get_program().is_valid() || selected.get_vertex_count() == 0);
  return selected;
}
