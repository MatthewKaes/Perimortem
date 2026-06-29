// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/ttx/core/type.hpp"

#include "perimortem/core/reader/textual.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Ttx;

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

static auto semantic_type_name(const Syntax::Type::Ref& type) -> View::Bytes {
  View::Vector<View::Bytes> path = type.get_name_path();
  return path.is_empty() ? View::Bytes() : path[path.get_size() - 1];
}

auto Core::type_layout(const Syntax::Type::Ref& type) -> Layout {
  View::Bytes name = semantic_type_name(type);
  if (const Layout* layout = find_layout(name)) {
    return *layout;
  }

  if (name == "Vec"_view) {
    View::Vector<Syntax::Type::Ref> params = type.get_params();
    if (params.get_size() < 2 || !params[1].is_size_literal()) {
      return {};
    }

    Count count = 0;
    Layout element_layout = type_layout(params[0]);
    if (!element_layout.is_valid() ||
        !parse_count(params[1].get_name(), &count)) {
      return {};
    }

    return {
      Layout::Kind::Concrete,
      element_layout.get_byte_size() * count,
      element_layout.get_alignment(),
    };
  }

  return {};
}
