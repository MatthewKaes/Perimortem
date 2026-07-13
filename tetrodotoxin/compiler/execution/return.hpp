// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// Terminates a Block with a range into the enclosing Body's operand table.
class Return {
 public:
  explicit constexpr Return(Perimortem::Utility::Range values)
      : values(values) {}

  constexpr auto get_values() const -> Perimortem::Utility::Range {
    return values;
  }

 private:
  Perimortem::Utility::Range values;
};

}  // namespace Tetrodotoxin::Compiler::Execution
