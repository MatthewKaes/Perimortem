// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/compiler/execution/body.hpp"
#include "ttx/function.hpp"
#include "ttx/lexical/source.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// Joins one TTX signature and diagnostic source to its compiled Body.
class Function {
 public:
  constexpr Function(
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes symbol,
      const Ttx::Function& signature,
      const Body& body)
      : source(source), symbol(symbol), signature(signature), body(body) {}

  constexpr auto get_source() const -> Ttx::Lexical::Source { return source; }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

  constexpr auto get_signature() const -> const Ttx::Function& {
    return signature;
  }

  constexpr auto get_body() const -> const Body& { return body; }

 private:
  Ttx::Lexical::Source source;
  Perimortem::Core::View::Bytes symbol;
  const Ttx::Function& signature;
  const Body& body;
};

}  // namespace Tetrodotoxin::Compiler::Execution
