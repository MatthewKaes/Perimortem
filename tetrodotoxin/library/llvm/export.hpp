// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Export retains one callable and its completed native symbol. The symbol bytes
// belong to either authored source storage or the compilation Arena.
class Export {
 public:
  constexpr Export(
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Bytes symbol)
      : callable(callable), symbol(symbol) {}

  constexpr auto get_callable() const -> const Ttx::Model::Callable& {
    return callable.get();
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  Ttx::Concept::Reference<const Ttx::Model::Callable> callable;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Library::Llvm
