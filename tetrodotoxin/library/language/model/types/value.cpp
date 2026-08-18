// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/types/value.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Model::Types::Value::reserve(Llvm::Program& program) const
    -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve_value(
      program, *this, get_width(), is<Real>(), is<Signed>(), is<Flag>());
  return reserved ? True : False;
}

auto Language::Model::Types::Value::complete(Llvm::Program& program) const
    -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  Bool completed = !*began || carriers.complete_value(program, *this);
  return completed && complete_debug(program);
}
