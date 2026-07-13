// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/package/export.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Package {

// Package owns package metadata and its public type surface.
//
// Boot leaves the cursor at the package body. Puffer provides the package name
// as compile configuration, and Package reads the export syntax into a root
// Ttx::Type that represents the package's public surface.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Package"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Package
