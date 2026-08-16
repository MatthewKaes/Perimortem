// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package::Repository {

// Keeps the Archive artifact ID separate from its Bazel location. A build may
// relocate the native file without changing the ID stored in semantic data, so
// neither value can be derived from the other.
class Artifact {
 public:
  constexpr Artifact(
      Perimortem::Core::View::Bytes id,
      Perimortem::Core::View::Bytes filesystem_location)
      : id(id), filesystem_location(filesystem_location) {}

  constexpr auto get_id() const -> Perimortem::Core::View::Bytes { return id; }

  constexpr auto get_filesystem_location() const
      -> Perimortem::Core::View::Bytes {
    return filesystem_location;
  }

 private:
  Perimortem::Core::View::Bytes id;
  Perimortem::Core::View::Bytes filesystem_location;
};

}  // namespace Tetrodotoxin::Package::Repository
