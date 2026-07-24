// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/environment/workspace.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Archiver {

// Loader reserves the archive entry point while the source-free Package model
// and its durable encoding are still being defined.
class Loader {
 public:
  constexpr Loader(Perimortem::Core::View::Bytes source) : source(source) {}

  auto read_manifest(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Manifest&>;

  auto load_package(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Environment::Workspace& target,
      const Manifest& manifest) const -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Archiver
