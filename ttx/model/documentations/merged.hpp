// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/documentation.hpp"

namespace Ttx::Documentations {

// Merged keeps two complete documentation owners in order, which allows an
// Alias or another layered owner to add local explanation without copying or
// flattening the presentation supplied by its referent.
class Merged : public Documentation {
 public:
  constexpr Merged(const Documentation& first, const Documentation& second)
      : first(first), second(second) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    const Count first_count = first.line_count();
    return index < first_count ? first.get_line(index)
                               : second.get_line(index - first_count);
  }

  constexpr auto line_count() const -> Count override {
    return first.line_count() + second.line_count();
  }

 private:
  const Documentation& first;
  const Documentation& second;
};

}  // namespace Ttx::Documentations
