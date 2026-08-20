// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/record.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/errors.hpp"

namespace Puffer::Lsp {

// SemanticWorkspace owns one immutable analysis snapshot. A standalone request
// publishes one source, while a Package request imports the complete fixed
// Source table through one shared filesystem snapshot owner.
class SemanticWorkspace {
 public:
  enum class Mode : Unsigned_8 {
    Standalone,
    Package,
  };

  SemanticWorkspace(
      Mode mode,
      Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Package::Snapshots>
          snapshots,
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes package_root = {},
      Perimortem::Core::View::Bytes packages_root = {});

  auto find(Perimortem::Core::View::Bytes source_name, Count byte_offset) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  constexpr auto get_errors() const -> const Ttx::Lexical::Errors& {
    return errors;
  }

 private:
  Ttx::Lexical::Errors errors;
  Tetrodotoxin::Environment::Workspace workspace;
};

}  // namespace Puffer::Lsp
