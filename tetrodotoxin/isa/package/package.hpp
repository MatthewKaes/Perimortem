// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/package/export.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Package owns package metadata and its public type surface.
//
// Boot leaves the cursor at the package body. Package reads `@package_name` and
// export syntax, then publishes a root Ttx::Type with nested types for the
// package's public surface.
class Package {
 public:
  static auto evaluate(Context& context, Ttx::Lexical::Cursor& cursor) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Package"_view;
  }
};

}  // namespace Tetrodotoxin::Isa
