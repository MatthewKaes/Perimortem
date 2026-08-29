// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/bootstrap/concept/constant.hpp"
#include "ttx/concept/none.h"

namespace Ttx::Concept {

// None is the axiomatic answer that a completed concept has no semantic value.
// Its Type answer terminates on the same Constant rather than inventing a Type
// or returning a provisional Unknown fact.
class None : public Constant {
 public:
  TTX_CONTRACT(None, Constant);

  static auto get_none() -> const None&;
  static auto prove(Abstract& candidate) -> Perimortem::Core::Option<None&>;
  static auto prove(const Abstract& candidate)
      -> Perimortem::Core::Option<const None&>;

  None(const None&) = delete;
  auto operator=(const None&) -> None& = delete;

  TTX_NAME("None"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Abstract& override { return *this; }

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;

 private:
  constexpr None() = default;
};

}  // namespace Ttx::Concept
