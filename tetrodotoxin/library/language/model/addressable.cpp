// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/addressable.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Model::Addressable::persist(Archive::Writer&) const -> Bool {
  return False;
}
