// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/registry.hpp"

using namespace Tetrodotoxin::Isa;

auto Registry::install(
    Perimortem::Core::View::Bytes name,
    EvaluateFunction evaluator) -> Bool {
  Entry entry(name, evaluator);
  if (!entry.is_valid() || installed_count >= installed.get_size()) {
    return False;
  }

  for (Count i = 0; i < installed_count; i++) {
    if (installed[i].get_name() == name) {
      installed[i] = entry;
      return True;
    }
  }

  installed[installed_count] = entry;
  installed_count++;
  return True;
}

auto Registry::find(Perimortem::Core::View::Bytes name) const -> const Entry* {
  for (Count i = 0; i < installed_count; i++) {
    if (installed[i].get_name() == name) {
      return installed.get_data() + i;
    }
  }

  return nullptr;
}

auto Registry::get_installed() const
    -> Perimortem::Core::View::Vector<Entry> {
  return {installed.get_data(), installed_count};
}
