// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package::Repository {

// Keeps the Archive artifact ID separate from its native and ABI Manifest
// locations. A build may relocate either file without changing the ID stored in
// semantic data, so none of those values can be derived from another.
class Artifact {
 public:
  constexpr Artifact(
      Perimortem::Core::View::Bytes id,
      Perimortem::Core::View::Bytes filesystem_location,
      Perimortem::Core::View::Bytes abi_manifest_location)
      : id(id),
        filesystem_location(filesystem_location),
        abi_manifest_location(abi_manifest_location) {}

  constexpr auto get_id() const -> Perimortem::Core::View::Bytes { return id; }

  constexpr auto get_filesystem_location() const
      -> Perimortem::Core::View::Bytes {
    return filesystem_location;
  }

  constexpr auto get_abi_manifest_location() const
      -> Perimortem::Core::View::Bytes {
    return abi_manifest_location;
  }

 private:
  Perimortem::Core::View::Bytes id;
  Perimortem::Core::View::Bytes filesystem_location;
  Perimortem::Core::View::Bytes abi_manifest_location;
};

}  // namespace Tetrodotoxin::Package::Repository
