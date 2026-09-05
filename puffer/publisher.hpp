// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/abi.h"

namespace Puffer {

// Publisher commits the union of every requested product Pack beneath one
// output root. Each request remains alive while its Named routes and immutable
// ArtifactFiles are inspected, allowing the filesystem transition to stay
// independent from every Terminal implementation.
class Publisher {
 public:
  constexpr Publisher(
      Perimortem::Core::View::Bytes root,
      Perimortem::Core::View::Bytes publication_root)
      : root(root), publication_root(publication_root) {}

  auto publish(ttx_pack products) const -> Bool;
  auto publish(Perimortem::Core::View::Vector<ttx_pack> products) const -> Bool;

 private:
  Perimortem::Core::View::Bytes root;
  Perimortem::Core::View::Bytes publication_root;
};

}  // namespace Puffer
