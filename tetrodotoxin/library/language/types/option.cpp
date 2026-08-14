// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/option.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Types::Option::accepts(const Model::Pack& source) const -> Bool {
  return source.get_layout().is_empty() || source.fits_into(element);
}
