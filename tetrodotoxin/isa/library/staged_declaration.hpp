// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// A Library declaration retained until its type is first referenced.
class StagedDeclaration {
 public:
  enum class State : Unsigned_8 {
    Pending,
    Evaluating,
    Ready,
    Failed,
  };

  StagedDeclaration(
      Tetrodotoxin::Isa::Base::Declaration declaration,
      Perimortem::Utility::Range source)
      : declaration(declaration), source(source) {}

  constexpr auto get_declaration() const
      -> const Tetrodotoxin::Isa::Base::Declaration& {
    return declaration;
  }
  constexpr auto get_source() const -> Perimortem::Utility::Range {
    return source;
  }
  constexpr auto get_state() const -> State { return state; }

  // A recursive declaration may publish its reserved address only while its
  // materializer is active. Failed declarations revoke that provisional edge
  // before any later query can observe it.
  constexpr auto find_type() const -> const Ttx::Type* {
    return state == State::Evaluating || state == State::Ready ? type : nullptr;
  }

  constexpr auto begin_evaluation() -> Bool {
    if (state != State::Pending) {
      return False;
    }

    state = State::Evaluating;
    type = nullptr;
    return True;
  }

  constexpr auto stage_type(const Ttx::Type* reserved_type) -> Bool {
    if (state != State::Evaluating || reserved_type == nullptr) {
      return False;
    }

    type = reserved_type;
    return True;
  }

  constexpr auto complete(const Ttx::Type& materialized_type) -> Bool {
    if (state != State::Evaluating ||
        (type != nullptr && type != &materialized_type)) {
      return False;
    }

    type = &materialized_type;
    state = State::Ready;
    return True;
  }

  constexpr auto fail() -> void {
    type = nullptr;
    state = State::Failed;
  }

 private:
  Tetrodotoxin::Isa::Base::Declaration declaration;
  Perimortem::Utility::Range source;
  const Ttx::Type* type = nullptr;
  State state = State::Pending;
};

}  // namespace Tetrodotoxin::Isa::Library
