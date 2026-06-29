// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/ttx/graphics/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Ttx;

auto Graphics::type_layout(const Syntax::Type::Ref& type) -> Layout {
  View::Vector<View::Bytes> path = type.get_name_path();
  View::Bytes name =
      path.is_empty() ? View::Bytes() : path[path.get_size() - 1];
  if (const Layout* layout = find_layout(name)) {
    return *layout;
  }

  return {};
}
