// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;

Environment::Toolchain::Toolchain() : dialects(arena) {}

Environment::Toolchain::~Toolchain() {
  for (Count index = dialects.get_size(); index > 0; index--) {
    dialects[index - 1]->~Dialect();
  }
}

auto Environment::Toolchain::contains_name(View::Bytes name) const -> Bool {
  return dialects.get_view().contains([&](const Language::Dialect* dialect) {
    return dialect->get_name() == name;
  });
}

auto Environment::Toolchain::contains(const Language::Dialect& dialect) const
    -> Bool {
  return dialects.get_view().contains([&](const Language::Dialect* candidate) {
    return candidate == &dialect;
  });
}

auto Environment::Toolchain::find(View::Bytes name) const
    -> Option<Language::Dialect&> {
  for (Language::Dialect* dialect : dialects.get_view()) {
    if (dialect->get_name() == name) {
      return *dialect;
    }
  }

  return {};
}
