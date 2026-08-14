// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/dialects.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

Environment::Dialects::Installed::Installed(
    Language::Dialect& value,
    Unsigned_64 type_identity)
    : value(value), type_identity(type_identity) {}

auto Environment::Dialects::Installed::get() const -> Language::Dialect& {
  return value;
}

auto Environment::Dialects::Installed::get_type_identity() const
    -> Unsigned_64 {
  return type_identity;
}

Environment::Dialects::Dialects(Allocator::Arena& arena)
    : arena(arena), names(arena), values(arena), installed(arena) {}

Environment::Dialects::~Dialects() {
  // Workspace member order destroys Retention before this inventory. Hosted
  // Monographs are therefore gone while each Dialect can still inspect its
  // graph state during destruction.
  for (Count i = values.get_size(); i > 0; i--) {
    values[i - 1].get().~Dialect();
  }
}

auto Environment::Dialects::contains_instance(
    const Language::Dialect& dialect) const -> Bool {
  return values.get_view().contains(
      [&](const Installed& candidate) { return &candidate.get() == &dialect; });
}

auto Environment::Dialects::contains_type(Unsigned_64 type_identity) const
    -> Bool {
  return values.get_view().contains([&](const Installed& candidate) {
    return candidate.get_type_identity() == type_identity;
  });
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
