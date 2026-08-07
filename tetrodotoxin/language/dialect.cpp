// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/dialect.hpp"

using namespace Tetrodotoxin;

Language::Dialect::~Dialect() {}

auto Language::Dialect::encode(const Monograph&) const
    -> Perimortem::Utility::Option<Perimortem::Memory::Dynamic::Bytes> {
  return {};
}

auto Language::Dialect::restore(
    Perimortem::Memory::Allocator::Arena&,
    Perimortem::Core::View::Bytes) -> Perimortem::Utility::Option<Monograph&> {
  return {};
}
