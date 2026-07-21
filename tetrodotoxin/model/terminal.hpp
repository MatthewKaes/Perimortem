// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Tetrodotoxin::Model {

// Terminal is one completed opaque package product. Its logical path is
// relative to the package version directory and its bytes have no semantic
// interpretation at this boundary. The producing compiler, Archiver, restored
// Package, and filesystem materializer share this value without introducing a
// producer Kind or archive-owned replacement model.
class Terminal {
 public:
  Terminal(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes content);

  auto get_path() const -> Perimortem::Core::View::Bytes;
  auto get_content() const -> Perimortem::Core::View::Bytes;

  // Logical terminal paths use `/`, remain below the package version
  // directory, and name an actual file rather than a traversal component.
  static auto is_valid_path(Perimortem::Core::View::Bytes path) -> Bool;

 private:
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes content;
};

}  // namespace Tetrodotoxin::Model
