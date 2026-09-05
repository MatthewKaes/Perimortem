// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/ffi/cpp/callable.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Export retains one callable and its completed native symbol. The symbol bytes
// belong to either authored source storage or the compilation Arena.
class Export {
 public:
  constexpr Export(
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Bytes symbol)
      : callable(&callable), symbol(symbol) {}

  constexpr auto get_callable() const -> const Ttx::Model::Callable& {
    return *callable;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  const Ttx::Model::Callable* callable;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Terminal::Abi
