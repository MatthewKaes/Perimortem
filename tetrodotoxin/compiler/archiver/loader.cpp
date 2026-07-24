// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/loader.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;

auto Loader::read_manifest(Allocator::Arena&) const -> Option<const Manifest&> {
  // TODO: Decode a Manifest after the replacement buffer format is defined.
  return none;
}

auto Loader::load_package(
    Allocator::Arena&,
    Tetrodotoxin::Ttx::Model::Environment::Workspace&,
    const Manifest&) const -> const Abstract& {
  // TODO: Restore a Package after its source-free Model owner exists.
  return Invalid::get_invalid();
}
