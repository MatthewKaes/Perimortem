// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// One executable body dialect installed in a toolchain.
//
// Boot resolves authored ISA text to this semantic value once. Source records
// retain the value so evaluation and terminal lowering do not pass names
// between consumers and repeat registry lookup.
class Dialect {
 public:
  using Evaluator = Ttx::Type* (*)(Ttx::Lexical::Cursor & cursor,
                                   Base::Context& context);
  using Lowerer = Bool (*)(
      Tetrodotoxin::Isa::Lowering::Context& context,
      const Tetrodotoxin::Isa::Lowering::Input& input);

  Dialect() = default;
  Dialect(
      Perimortem::Core::View::Bytes name,
      Evaluator evaluator,
      Lowerer lowerer = nullptr,
      Bool package_ready = False)
      : name(name),
        evaluator(evaluator),
        lowerer(lowerer),
        package_ready(package_ready) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_evaluator() const -> Evaluator { return evaluator; }
  constexpr auto get_lowerer() const -> Lowerer { return lowerer; }
  constexpr auto can_lower() const -> Bool { return lowerer != nullptr; }
  constexpr auto can_lower_package() const -> Bool {
    return lowerer != nullptr && package_ready;
  }

  constexpr auto is_empty() const -> Bool {
    return name.is_empty() || evaluator == nullptr;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Evaluator evaluator = nullptr;
  Lowerer lowerer = nullptr;
  Bool package_ready = False;
};

}  // namespace Tetrodotoxin::Isa
