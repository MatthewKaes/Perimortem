// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/uuid.hpp"

namespace Tetrodotoxin::Archiver {

// Dependency is a package import edge stored in a Puffer Buffer manifest.
//
// Imports preserve the authored local name so a restored package can rebuild
// its source-visible Boot preamble. Archive readers and writers derive the
// compact type-ref restore table by walking these imports recursively.
class Dependency {
 public:
  constexpr Dependency(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_name,
      Perimortem::System::Uuid version)
      : local_name(local_name), source_name(source_name), version(version) {}

  constexpr auto operator==(const Dependency& rhs) const -> Bool {
    return version == rhs.version && local_name == rhs.local_name &&
           source_name == rhs.source_name;
  }

  constexpr auto operator!=(const Dependency& rhs) const -> Bool {
    return !(*this == rhs);
  }

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return source_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Uuid {
    return version;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_name;
  Perimortem::System::Uuid version;
};

}  // namespace Tetrodotoxin::Archiver
