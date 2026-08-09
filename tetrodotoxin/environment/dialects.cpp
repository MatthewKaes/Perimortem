// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/dialects.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

Environment::Dialects::Installed::Installed(Language::Dialect& value)
    : value(value) {}

auto Environment::Dialects::Installed::get() const -> Language::Dialect& {
  return value;
}

Environment::Dialects::Dialects(Allocator::Arena& arena)
    : arena(arena), names(arena), values(arena), installed(arena) {}

Environment::Dialects::~Dialects() {
  // Workspace member order destroys Retention before this inventory. Hosted
  // Monographs are therefore gone while each Dialect can still inspect its
  // graph state during destruction.
  for (Count i = 0; i < values.get_size(); i++) {
    values[i].get().~Dialect();
  }
}

auto Environment::Dialects::find(View::Bytes name)
    -> Option<Language::Dialect&> {
  return installed.visit(
      name,
      [](Language::Dialect& dialect) -> Option<Language::Dialect&> {
        return dialect;
      },
      []() -> Option<Language::Dialect&> { return {}; });
}

auto Environment::Dialects::get_names() const -> View::Vector<View::Bytes> {
  return names.get_view();
}
