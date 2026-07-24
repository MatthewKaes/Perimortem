// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/environment/workspace.hpp"

namespace Tetrodotoxin::Archiver {

// Reads an archived Tetrodotoxin package and loads in into a workspace.
// If it's dependencies aren't resolved then intalling the package will fail.
class Loader {
 public:
  constexpr Loader(Perimortem::Core::View::Bytes source) : source(source) {}

  auto read_manifest(Perimortem::Memory::Allocator::Arena& arena) const
      -> const Manifest*;

  // TODO: Needs to take the workspace it's loading into
  auto load_package(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Environment::Workspace& target,
      const Manifest& manifest) const -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Archiver
