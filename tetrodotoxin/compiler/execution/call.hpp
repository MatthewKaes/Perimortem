// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

#include "ttx/function.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// A canonical function call with ranges into the enclosing Body's operand and
// binding tables. The signature remains the owner of parameter and result
// types.
class Call {
 public:
  constexpr Call(
      const Ttx::Function& signature,
      Perimortem::Core::View::Bytes symbol,
      Perimortem::Utility::Range arguments,
      Perimortem::Utility::Range results)
      : signature(signature),
        symbol(symbol),
        arguments(arguments),
        results(results) {}

  constexpr auto get_signature() const -> const Ttx::Function& {
    return signature;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

  constexpr auto get_arguments() const -> Perimortem::Utility::Range {
    return arguments;
  }

  constexpr auto get_results() const -> Perimortem::Utility::Range {
    return results;
  }

 private:
  const Ttx::Function& signature;
  Perimortem::Core::View::Bytes symbol;
  Perimortem::Utility::Range arguments;
  Perimortem::Utility::Range results;
};

}  // namespace Tetrodotoxin::Compiler::Execution
