// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Dependency is the legacy summary retained by Archive Formats 2 and 3. Format
// 4 preserves each exact source-local Package Alias as a GraphImport instead.
class Dependency {
 public:
  constexpr Dependency(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version,
      Ttx::Lexical::Span statement = {})
      : local_name(local_name),
        package_name(package_name),
        version(version),
        statement(statement) {}

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return statement; }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
  Ttx::Lexical::Span statement;
};

}  // namespace Tetrodotoxin::Package::Language
