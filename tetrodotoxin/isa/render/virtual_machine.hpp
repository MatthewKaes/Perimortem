// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Terminal {

class Context;
class Input;

}  // namespace Tetrodotoxin::Terminal

namespace Tetrodotoxin::Isa::Render {

// Render is the installed body ISA for render pipeline source files.
//
// Render-specific validation will eventually live here. For now the evaluator
// publishes the first authored Type as a shallow source export so package and
// Library code can resolve `ImportName::RenderType` without a side IR.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Ttx::Type*;
  static auto lower(
      Tetrodotoxin::Terminal::Context& context,
      const Tetrodotoxin::Terminal::Input& input) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Render"_view;
  }

 private:
  static auto evaluate_member(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      const Tetrodotoxin::Isa::Base::Declaration& definition,
      Perimortem::Core::View::Bytes initializer_error,
      Perimortem::Core::View::Bytes unresolved_error) -> const Ttx::Member*;
};

}  // namespace Tetrodotoxin::Isa::Render
