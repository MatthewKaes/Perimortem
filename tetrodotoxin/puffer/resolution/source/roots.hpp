// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Project-level folders visible to relative source imports.
//
// Puffer normally needs one project tree. A second root supports generated or
// separately mounted project input without storing search policy on every
// source record. Compiled packages use Package::Cache instead.
class Roots {
 public:
  auto include(Perimortem::Core::View::Bytes source_path) -> Bool;
  auto contains(Perimortem::Core::View::Bytes source_path) const -> Bool;
  auto reset() -> void;

 private:
  static auto common(
      Perimortem::Core::View::Bytes left,
      Perimortem::Core::View::Bytes right) -> Perimortem::Core::View::Bytes;

  Perimortem::Core::Static::Vector<Perimortem::Memory::Dynamic::Bytes, 2> roots;
  Count root_count = 0;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
