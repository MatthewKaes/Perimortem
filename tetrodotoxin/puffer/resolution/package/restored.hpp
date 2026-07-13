// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/object.hpp"

#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Package {

// One restored package and the source-graph record that exposes its Type.
//
// The Record retains the restoration arena, so the Package pointer remains
// valid for exactly the same lifetime as the graph identity published from it.
class Restored {
 public:
  Restored(
      Perimortem::Memory::Dynamic::Object<Source::Record> record,
      const Tetrodotoxin::Archiver::Package& package)
      : record(record), package(&package) {}
  Restored(const Restored&) = default;
  auto operator=(const Restored&) -> Restored& = default;

  constexpr auto get_record()
      -> Perimortem::Memory::Dynamic::Object<Source::Record>& {
    return record;
  }

  constexpr auto get_package() const -> const Tetrodotoxin::Archiver::Package& {
    return *package;
  }

 private:
  Perimortem::Memory::Dynamic::Object<Source::Record> record;
  const Tetrodotoxin::Archiver::Package* package;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Package
