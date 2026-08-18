// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/types/value.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Model::Types::Value::reserve(Llvm::Program& program) const
    -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve(program, *this, Llvm::Carriers::Kind::Value);
  return reserved ? True : False;
}

auto Language::Model::Types::Value::complete(Llvm::Program& program) const
    -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  Bool completed =
      !*began || carriers.complete(program, *this, Llvm::Carriers::Kind::Value);
  return completed && complete_debug(program);
}
