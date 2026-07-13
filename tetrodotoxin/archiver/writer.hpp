// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/reference.hpp"

namespace Tetrodotoxin::Archiver {

// Writes a Package into one dense Puffer Buffer.
class Writer {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      const Package& package,
      Perimortem::Core::View::Vector<Reference> references)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Archiver
