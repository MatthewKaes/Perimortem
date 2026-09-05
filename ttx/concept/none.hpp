// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/constant.hpp"

namespace Ttx::Concept {

// None proves that one route has completed without an answer. Returning itself
// for every later question makes that absence immutable evidence, which keeps
// it distinct from empty value flow and an answer which remains indeterminate.
class None : public Constant {
 public:
  static auto get_none() -> const None&;
  None(const None&) = delete;
  auto operator=(const None&) -> None& = delete;

  TTX_NAME("None"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_concept(ttx_borrowed_bytes) const -> ttx_abstract override {
    return get_abi();
  }
  void domain(ttx_abstract self, ttx_domain_result result) const override;

 private:
  None() = default;
};

}  // namespace Ttx::Concept
