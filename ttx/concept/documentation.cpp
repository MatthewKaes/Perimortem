// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/documentation.hpp"

auto Ttx::Concept::Documentation::get_interface() const -> Handle {
  static const Handle::Operations operations = {
    [](const void* source) -> Count {
      return static_cast<const Documentation*>(source)->line_count();
    },
    [](const void* source, Count index) -> perimortem_view_bytes {
      const auto line =
          static_cast<const Documentation*>(source)->get_line(index);
      return {line.get_data(), line.get_size()};
    },
  };
  return Handle({this, &operations});
}

auto Ttx::Concept::Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}
