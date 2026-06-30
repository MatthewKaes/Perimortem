// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/ttx/graphics/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Ttx;

auto Graphics::type_layout(const TypeQuery& type) -> Layout {
  if (const Layout* layout = find_layout(type.get_name())) {
    return *layout;
  }

  return {};
}
