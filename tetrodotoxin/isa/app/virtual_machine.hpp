// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::App {

// App is the body ISA for process bootstrap sources.
//
// It accepts a single root `main` function. Lowering can later treat that
// function as the foreign executable entry point while the source model remains
// ordinary TTX function facts.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "App"_view;
  }

 private:
  static auto evaluate_function(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation) -> Ttx::Function;
};

}  // namespace Tetrodotoxin::Isa::App
