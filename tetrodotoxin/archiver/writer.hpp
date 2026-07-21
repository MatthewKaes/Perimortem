// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/package.hpp"
#include "tetrodotoxin/model/terminal.hpp"

namespace Tetrodotoxin::Archiver {

// Traverses one completed current-model Package and its opaque terminal
// products into one immutable arena-owned Puffer Buffer.
class Writer {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      const Manifest& manifest,
      const Model::Package& package,
      Perimortem::Core::View::Vector<Model::Terminal> terminals)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Archiver
