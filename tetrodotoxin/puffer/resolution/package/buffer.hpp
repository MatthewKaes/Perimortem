// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/archiver/package.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Package {

// Registered immutable Puffer Buffer input.
//
// Registration keeps the raw buffer alive. Manifest and package tables are
// independently readable because the archive directory stores their offsets.
class Buffer {
 public:
  explicit Buffer(Perimortem::Core::View::Bytes content);

  auto read_manifest(Perimortem::Memory::Allocator::Arena& target) const
      -> const Tetrodotoxin::Archiver::Manifest*;
  auto read_package(
      Perimortem::Memory::Allocator::Arena& target,
      const Tetrodotoxin::Archiver::Manifest& manifest,
      Perimortem::Core::View::Vector<const Tetrodotoxin::Archiver::Package*>
          references) const -> Tetrodotoxin::Archiver::Package*;

 private:
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Bytes source;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Package
