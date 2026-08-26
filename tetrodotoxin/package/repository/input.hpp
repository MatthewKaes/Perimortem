// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/repository/artifact.hpp"

namespace Tetrodotoxin::Package::Repository {

// Binds one Package key to the exact source and compiled products supplied by
// a caller. Locations remain outside the key because moving an installation or
// local checkout does not create a different semantic Package.
class Input {
 public:
  constexpr Input(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes archive_location,
      Perimortem::Core::View::Vector<Artifact> artifacts,
      Perimortem::Core::View::Bytes source_location = {})
      : identity(identity),
        version(version),
        archive_location(archive_location),
        artifacts(artifacts),
        source_location(source_location) {}

  // Treating different locations as different Inputs would allow two physical
  // declarations to compete for one Package key. Equality therefore stops at
  // identity and Version so Repository can reject that ambiguity.
  constexpr auto operator==(const Input& rhs) const -> Bool {
    return identity == rhs.identity && version == rhs.version;
  }

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_archive_location() const -> Perimortem::Core::View::Bytes {
    return archive_location;
  }

  constexpr auto get_artifacts() const
      -> Perimortem::Core::View::Vector<Artifact> {
    return artifacts;
  }

  constexpr auto get_source_location() const -> Perimortem::Core::View::Bytes {
    return source_location;
  }

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes archive_location;
  Perimortem::Core::View::Vector<Artifact> artifacts;
  Perimortem::Core::View::Bytes source_location;
};

}  // namespace Tetrodotoxin::Package::Repository
