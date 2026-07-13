// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Foreign {

// Foreign is Library's child dialect for host-provided functions.
//
// Library recognizes the declaration boundary and hands the body to Foreign.
// Foreign owns declaration legality and publishes compiler linkage for the
// functions it produces. It does not leak a provider object back through the
// Library type tree.
class Dialect {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Library::Scope& scope,
      const Tetrodotoxin::Isa::Base::Declaration& definition)
      -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "foreign"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Foreign
