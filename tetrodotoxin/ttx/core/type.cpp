// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/ttx/core/type.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Ttx;

namespace {

class GenericLayout {
 public:
  Count element_size = 0;
  Count element_alignment = 0;
  Count element_count = 0;
  Layout layout;
};

auto find_cached_layout(
    View::Vector<GenericLayout> layouts,
    const Layout& element,
    Count element_count) -> const Layout* {
  for (Count i = 0; i < layouts.get_size(); i++) {
    if (layouts[i].element_size == element.get_byte_size() &&
        layouts[i].element_alignment == element.get_alignment() &&
        layouts[i].element_count == element_count) {
      return &layouts[i].layout;
    }
  }

  return nullptr;
}

auto generated_layout_cache() -> Dynamic::Vector<GenericLayout>& {
  // Process-lifetime provider cache. Avoid exit-time destruction because these
  // facts can be referenced by generated/static toolchain state during
  // teardown.
  static auto* layouts = new Dynamic::Vector<GenericLayout>();
  return *layouts;
}

}  // namespace

auto Core::type_layout(const TypeQuery& type) -> Layout {
  View::Bytes name = type.get_name();
  if (const Layout* layout = find_layout(name)) {
    return *layout;
  }

  if (name == "Vec"_view) {
    View::Vector<TypeArgument> params = type.get_params();
    if (params.get_size() < 2 || !params[0].is_type() ||
        !params[1].is_size_literal()) {
      return {};
    }

    Layout element_layout = params[0].get_layout();
    if (!element_layout.is_valid()) {
      return {};
    }

    Dynamic::Vector<GenericLayout>& cache = generated_layout_cache();
    if (const Layout* cached = find_cached_layout(
            cache.get_view(), element_layout, params[1].get_size_value())) {
      return *cached;
    }

    // Generic layouts are provider facts. Cache them after resolution has
    // reduced syntax to a TypeQuery so generated Core facts do not keep raw
    // syntax references alive.
    Layout layout = {
      Layout::Kind::Concrete,
      element_layout.get_byte_size() * params[1].get_size_value(),
      element_layout.get_alignment(),
    };
    cache.insert({
      element_layout.get_byte_size(),
      element_layout.get_alignment(),
      params[1].get_size_value(),
      layout,
    });
    return layout;
  }

  return {};
}
