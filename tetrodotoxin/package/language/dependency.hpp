// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Dependency is the exact external Package request authored in package.ttx.
// Package selection resolves the request and a later assembly transaction
// retains the resulting semantic edge. This value is never itself that
// resolution.
class Dependency {
 public:
  constexpr Dependency(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version)
      : local_name(local_name), package_name(package_name), version(version) {}

  // Consumes one complete Resolve statement and returns its lexical Span
  // separately from the durable request. Failure recovers the Cursor and
  // leaves the supplied Span invalid.
  static auto parse(Ttx::Lexical::Cursor& cursor, Ttx::Lexical::Span& span)
      -> Perimortem::Utility::Option<Dependency>;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
};

}  // namespace Tetrodotoxin::Package::Language
