// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/archiver/package.hpp"

namespace Tetrodotoxin::Archiver {

// Random-access Puffer Buffer reader.
//
// The fixed table directory makes each read independent. Reader owns no
// decoded archive state. The caller owns every returned Manifest or Package.
class Reader {
 public:
  explicit constexpr Reader(Perimortem::Core::View::Bytes source)
      : source(source) {}

  auto read_manifest(Perimortem::Memory::Allocator::Arena& arena) const
      -> const Manifest*;
  auto read_package(
      Perimortem::Memory::Allocator::Arena& arena,
      const Manifest& manifest,
      Perimortem::Core::View::Vector<const Package*> references) const
      -> Package*;

 private:
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Archiver
