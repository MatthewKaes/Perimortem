// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/package/source.hpp"
#include "tetrodotoxin/model/package/terminal.hpp"

namespace Tetrodotoxin::Archiver {

// Writer reserves the archive entry point while a finalized Package graph and
// its durable encoding are still being defined.
class Writer {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      const Manifest& manifest,
      const Model::Package::Source& package,
      Perimortem::Core::View::Vector<Model::Package::Terminal> terminals)
      -> Perimortem::Utility::Option<Perimortem::Core::View::Bytes>;
};

}  // namespace Tetrodotoxin::Archiver
