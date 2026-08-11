// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/documentation.hpp"

namespace Ttx::Lexical {
class Anchor;
}

namespace Ttx::Concept {

// Authorship is an identity-free source fact retained by a semantic Abstract.
// It preserves the exact authored Documentation and Anchor while the concrete
// language decides whether the authored object participates in publication.
class Authorship {
 public:
  constexpr virtual ~Authorship() = default;

  virtual constexpr auto get_documentation() const -> const Documentation& = 0;

  virtual constexpr auto get_anchor() const -> Ttx::Lexical::Anchor = 0;

  virtual constexpr auto is_published() const -> Bool = 0;
};

}  // namespace Ttx::Concept
