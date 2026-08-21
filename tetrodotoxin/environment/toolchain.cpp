// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;

Environment::Toolchain::Toolchain() : dialects(arena) {}

Environment::Toolchain::~Toolchain() {
  for (Count index = dialects.get_size(); index > 0; index--) {
    dialects[index - 1].get().~Dialect();
  }
}

auto Environment::Toolchain::contains_name(View::Bytes name) const -> Bool {
  return dialects.get_view().contains(
      [&](const Reference<Language::Dialect>& dialect) {
        return dialect.get().get_name() == name;
      });
}

auto Environment::Toolchain::contains(const Language::Dialect& dialect) const
    -> Bool {
  return dialects.get_view().contains(
      [&](const Reference<Language::Dialect>& candidate) {
        return &candidate.get() == &dialect;
      });
}

auto Environment::Toolchain::find(View::Bytes name) const
    -> Option<Language::Dialect&> {
  for (const Reference<Language::Dialect>& dialect : dialects.get_view()) {
    if (dialect.get().get_name() == name) {
      return dialect.get();
    }
  }

  return {};
}
