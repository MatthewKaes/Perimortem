// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/record.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"

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

inline auto retains_library_source(
    const Tetrodotoxin::Environment::Workspace& workspace,
    Perimortem::Core::View::Bytes semantic_name) -> Bool {
  return workspace.resolve_context(semantic_name)
      .is<Tetrodotoxin::Library::Language::Monograph>();
}

}  // namespace Validation
