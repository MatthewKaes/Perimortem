// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Concept {

// Pack carries one produced semantic flow. Its Layout names the real
// Abstracts supplying that flow, so the Pack needs no identity of its own.
// Discovery visits named answers directly and does not require a Pack.
class Pack {
 public:
  constexpr virtual ~Pack() = default;
  virtual constexpr auto get_layout() const -> const Layout& = 0;
};

}  // namespace Ttx::Concept
