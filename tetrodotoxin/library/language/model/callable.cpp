// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/callable.hpp"

using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;

auto Language::Model::Callable::persist(Archive::Writer&) const -> Bool {
  return False;
}
