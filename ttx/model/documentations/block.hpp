// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/documentation.hpp"

namespace Ttx::Documentations {

// Block preserves the authored separation between adjacent documentation
// lines. A parser can lend its source-backed views directly, allowing aliases
// and archives to compose the presentation without flattening it first.
class Block : public Documentation {
 public:
  constexpr Block(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines)
      : lines(lines) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    return index < lines.get_size() ? lines.get_data()[index]
                                    : Perimortem::Core::View::Bytes();
  }

  constexpr auto line_count() const -> Count override {
    return lines.get_size();
  }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Ttx::Documentations
