// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/option.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Option::create_default(Perimortem::Memory::Allocator::Arena& arena)
    const -> Perimortem::Core::Option<Model::Pack&> {
  return Constants::Option::create_absent(arena, *this);
}

auto Types::Option::accepts(const Model::Pack& source) const -> Bool {
  return source.get_layout().is_empty() || source.fits_into(element);
}

auto Types::Option::create_fitted(
    Perimortem::Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&> {
  auto fitted = Constants::Option::create_fitted(arena, *this, source);
  return fitted ? Perimortem::Core::Option<Model::Pack&>(*fitted)
                : Perimortem::Core::Option<Model::Pack&>();
}

auto Types::Option::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve_option(program, *this);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool element_reserved = element.reserve(program);
  if (!element_reserved) {
    return False;
  }

  return flag.reserve(program);
}

auto Types::Option::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool element_completed = element.complete(program);
  if (!element_completed) {
    return False;
  }

  Bool flag_completed = flag.complete(program);
  if (!flag_completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete_option(program, *this, element, flag);
  return carrier_completed && complete_debug(program);
}
