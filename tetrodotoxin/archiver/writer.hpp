// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/environment/workspace.hpp"
#include "tetrodotoxin/model/package/terminal.hpp"

namespace Tetrodotoxin::Archiver {

// Walks a package and encodes it's data along with any required terminals in
// a stable format which can be loaded into a workspace that satisfies it's
// constraints stored in it's manifest.
class Writer {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      const Manifest& manifest,
      const Model::Package::Source& package,
      Perimortem::Core::View::Vector<Model::Package::Terminal> terminals)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Archiver
