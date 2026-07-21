// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/package.hpp"

namespace Tetrodotoxin::Archiver {

// Random-access Puffer Buffer reader. Manifest inspection is independent from
// semantic restoration. A Package read validates and constructs the complete
// graph in the target arena before returning any queryable root.
class Reader {
 public:
  explicit constexpr Reader(Perimortem::Core::View::Bytes source)
      : source(source) {}

  auto read_manifest(Perimortem::Memory::Allocator::Arena& arena) const
      -> const Manifest*;
  auto read_package(
      Perimortem::Memory::Allocator::Arena& arena,
      const Manifest& manifest,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Package>>
          dependencies) const -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Archiver
