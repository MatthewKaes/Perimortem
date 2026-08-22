// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

namespace Tetrodotoxin::Package::Archive {

// Emits the canonical Package Archive Format 2 representation from a validated
// Archive value. Writer measures the complete envelope before allocation, then
// preserves every supplied list order through the shared section vocabulary.
class Writer {
 public:
  Writer() = delete;

  // Writes all seven required sections in canonical order. A body that exceeds
  // the Format 2 limit logs a warning. Failure to reach the measured boundary
  // logs an error.
  static auto write(const Archive& archive)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  static auto write(
      const Package::Language::Monograph& package,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Tetrodotoxin::Language::Persistence::Profile profile,
      Perimortem::Core::View::Vector<Artifact> artifacts = {},
      Perimortem::Core::View::Vector<Export> exports = {})
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::Package::Archive
