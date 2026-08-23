// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/language/parser/name.hpp"
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
      Parser::Name local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version,
      Ttx::Lexical::Span statement = {})
      : local_name(local_name),
        package_name(package_name),
        version(version),
        statement(statement) {}

  // Consumes one complete Resolve statement. An authored value retains its
  // statement coordinates for the enclosing import operation. Restored values
  // use the default invalid Span.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Dependency>;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name.get_view();
  }

  constexpr auto get_local_route() const -> const Parser::Name& {
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
  Parser::Name local_name;
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
  Ttx::Lexical::Span statement;
};

}  // namespace Tetrodotoxin::Package::Language
