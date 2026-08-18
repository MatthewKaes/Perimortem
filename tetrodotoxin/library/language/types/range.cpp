// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/range.hpp"

#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Range::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Range::create_synthetic(arena, *this);
}

auto Types::Range::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve(program, *this, Llvm::Carriers::Kind::Range);
  if (!reserved) {
    return False;
  }

  return !*reserved || element.reserve(program);
}

auto Types::Range::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool completed = element.complete(program);
  if (!completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Range);
  return carrier_completed && complete_debug(program);
}
