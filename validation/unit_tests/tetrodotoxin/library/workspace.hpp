// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/record.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Validation {

// Each fixture owns its Toolchain longer than the Workspace graph borrowing
// it. Product hosts exercise the same contract with one shared installation.
inline auto create_library_toolchain() -> Perimortem::Memory::Dynamic::Record<
    Tetrodotoxin::Environment::Toolchain> {
  Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Environment::Toolchain>
      toolchain;
  toolchain->install<Tetrodotoxin::Library::Dialect>("Library"_view);
  return toolchain;
}

inline auto get_library_dialect(Tetrodotoxin::Environment::Toolchain& toolchain)
    -> Tetrodotoxin::Library::Dialect& {
  return static_cast<Tetrodotoxin::Library::Dialect&>(
      *toolchain.find("Library"_view));
}

}  // namespace Validation
