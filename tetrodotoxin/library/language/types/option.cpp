// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/option.hpp"

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
