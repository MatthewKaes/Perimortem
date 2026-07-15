// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Project-level folders visible to relative source imports.
//
// The resolver learns source roots from the files a request explicitly loads.
// Each root is widened only when two loaded files share a directory. Imports
// must remain below one of those roots, while compiled packages use the
// separate Package::Cache.
class Roots {
 public:
  auto include(Perimortem::Core::View::Bytes source_path) -> Bool;
  auto contains(Perimortem::Core::View::Bytes source_path) const -> Bool;
  auto reset() -> void;

 private:
  static auto common(
      Perimortem::Core::View::Bytes left,
      Perimortem::Core::View::Bytes right) -> Perimortem::Core::View::Bytes;

  Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes> roots;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
