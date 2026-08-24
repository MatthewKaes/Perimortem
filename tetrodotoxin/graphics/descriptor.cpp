// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/descriptor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

auto Graphics::Descriptor::placement(const U8* payload) const
    -> Option<Placement> {
  BAIL_IF(!payload || !read_placement);
  return read_placement(payload);
}

auto Graphics::Descriptor::child_count(const U8* payload) const -> Count {
  return payload && read_child_count ? read_child_count(payload) : 0;
}

auto Graphics::Descriptor::child(const U8* payload, Count index) const
    -> Option<Child> {
  BAIL_IF(!payload || !read_child || index >= child_count(payload));
  Child selected = read_child(payload, index);
  BAIL_IF(selected.get_object().is_empty() || !selected.get_descriptor());
  return selected;
}

auto Graphics::Descriptor::draw_count(const U8* payload) const -> Count {
  return payload && read_draw_count ? read_draw_count(payload) : 0;
}

auto Graphics::Descriptor::draw(const U8* payload, Count index) const
    -> Option<Draw> {
  BAIL_IF(!payload || !read_draw || index >= draw_count(payload));
  Draw selected = read_draw(payload, index);
  BAIL_IF(
      !selected.get_program().is_valid() || selected.get_vertex_count() == 0);
  return selected;
}
