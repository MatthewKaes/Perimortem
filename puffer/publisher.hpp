// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/concept/pack.h"

namespace Puffer {

// Publisher commits one complete named Product Pack beneath a single output
// root. It owns filesystem validation and atomic replacement, never semantic
// traversal or product naming.
class Publisher {
 public:
  constexpr explicit Publisher(Perimortem::Core::View::Bytes root)
      : root(root) {}

  auto publish(const ttx_pack* products) const -> Bool;

 private:
  Perimortem::Core::View::Bytes root;
};

}  // namespace Puffer
