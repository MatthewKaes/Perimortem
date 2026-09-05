// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/documentation.hpp"

namespace Ttx::Documentations {

// Comment lends one implementation-owned line through the same support
// contract used by authored blocks. Its canonical empty value lets generated
// owners answer the total documentation query without allocating a sentinel.
class Comment : public Documentation {
 public:
  static auto get_empty() -> const Comment&;

  constexpr Comment(Perimortem::Core::View::Bytes text) : text(text) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    return index == 0 ? text : Perimortem::Core::View::Bytes();
  }

  constexpr auto line_count() const -> Count override {
    return text.is_empty() ? 0 : 1;
  }

 private:
  Perimortem::Core::View::Bytes text;
};

}  // namespace Ttx::Documentations
