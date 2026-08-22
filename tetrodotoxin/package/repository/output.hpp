// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

namespace Tetrodotoxin::Package::Repository {

// Describes the common key and route carried by every Bazel publication.
// Archive and native product kinds remain separate Repository inventories, so
// duplicating their data shape as separate classes would add type differences
// without adding an ownership difference.
class Output {
 public:
  constexpr Output(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes artifact_id,
      Perimortem::Core::View::Bytes route)
      : identity(identity),
        version(version),
        artifact_id(artifact_id),
        route(route) {}

  // The route is mutable build placement rather than product identity. Ignoring
  // it here makes two routes for one product a duplicate instead of allowing
  // the later declaration to hide the conflict.
  constexpr auto operator==(const Output& rhs) const -> Bool {
    return identity == rhs.identity && version == rhs.version &&
           artifact_id == rhs.artifact_id;
  }

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_artifact_id() const -> Perimortem::Core::View::Bytes {
    return artifact_id;
  }

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes artifact_id;
  Perimortem::Core::View::Bytes route;
};

}  // namespace Tetrodotoxin::Package::Repository
