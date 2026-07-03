// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/lingua_franca.hpp"

using namespace Ttx::Dialect;

static Perimortem::Core::Static::Vector<LinguaFranca, 64> registered;
static Count dialect_count = 0;

auto Ttx::Dialect::LinguaFranca::add(LinguaFranca dialect) -> void {
  // Ran out of dialect space.
  // TODO: We should really figure out how to do this at the compiler layer in
  // C++... TTX allows for compile time aggregation of address but until we self
  // host C++ is a bit of a pain here.
  if (dialect_count >= registered.get_size()) {
    return;
  }

  for (Count i = 0; i < dialect_count; i++) {
    if (registered[i].get_name() == dialect.get_name()) {
      return;
    }
  }

  registered[dialect_count] = dialect;
  dialect_count++;
}

auto Ttx::Dialect::LinguaFranca::find(Perimortem::Core::View::Bytes name)
    -> const LinguaFranca {
  for (Count i = 0; i < dialect_count; i++) {
    if (registered[i].get_name() == name) {
      return registered[i];
    }
  }

  return LinguaFranca();
}

auto Ttx::Dialect::LinguaFranca::get_dialect()
    -> Perimortem::Core::View::Vector<LinguaFranca> {
  return {registered.get_data(), dialect_count};
}
