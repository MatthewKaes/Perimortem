// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/concept/unknown.h"

namespace Ttx::Concept {

// Unknown is the provisional answer to a semantic question that the current
// graph cannot settle. Repeating the original question may later produce a
// factual identity, so Unknown is not Constant and cannot be cached as absence.
class Unknown : public Abstract {
 public:
  TTX_CONTRACT(Unknown, Abstract);

  static auto get_unknown() -> const Unknown&;
  static auto prove(Abstract& candidate) -> Perimortem::Core::Option<Unknown&>;
  static auto prove(const Abstract& candidate)
      -> Perimortem::Core::Option<const Unknown&>;

  Unknown(const Unknown&) = delete;
  auto operator=(const Unknown&) -> Unknown& = delete;

  TTX_NAME("Unknown"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve() const -> const Abstract& override { return *this; }
  constexpr auto get_type() const -> const Abstract& override { return *this; }
  constexpr auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return *this;
  }

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;

 private:
  constexpr Unknown() = default;
};

}  // namespace Ttx::Concept
