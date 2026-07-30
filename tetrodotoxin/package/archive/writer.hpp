// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"

namespace Tetrodotoxin::Package::Archive {

// Emits the canonical Package Archive Format 1 representation from a validated
// Archive value. Writer measures the complete envelope before allocation, then
// preserves every supplied list order through the shared section vocabulary.
class Writer {
 public:
  Writer() = delete;

  // Writes all six required sections in canonical order. A body that exceeds
  // the Format 1 limit logs a warning. Failure to reach the measured boundary
  // logs an error.
  static auto write(const Archive& archive)
      -> Perimortem::Utility::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::Package::Archive
