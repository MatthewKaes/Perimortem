// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Library is the baseline body ISA for general TTX source files.
//
// For now it accepts the body while package and type binding are still being
// built. That still gives `dialect : Library;` a real installed evaluator
// instead of letting the resolver tolerate unknown source bodies.
class Library {
 public:
  static auto evaluate(Context& context, Ttx::Lexical::Cursor& cursor) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Library"_view;
  }
};

}  // namespace Tetrodotoxin::Isa
