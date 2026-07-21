// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/packages/compiled.hpp"

namespace Tetrodotoxin::Puffer::Package {

// Materializer publishes one compiled terminal collection beneath the exact
// authored package identity. Filesystem paths remain Puffer policy and never
// become fields on the semantic Package.
class Materializer {
 public:
  static auto materialize(
      Perimortem::Core::View::Bytes packages_root,
      const Tetrodotoxin::Archiver::Manifest& manifest,
      const Model::Packages::Compiled& package) -> Bool;
};

}  // namespace Tetrodotoxin::Puffer::Package
