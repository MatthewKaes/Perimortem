// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Package::Language {

// Dependency is the exact external Package request authored in package.ttx.
// Package selection resolves the request and a later assembly transaction
// retains the resulting semantic edge. This value is never itself that
// resolution.
class Source {
 public:
  constexpr Source(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_path)
      : local_name(local_name), source_path(source_path) {}

  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<Source>;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_path;
};

}  // namespace Tetrodotoxin::Package::Language
