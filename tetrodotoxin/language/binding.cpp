// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/binding.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem::Core;

Binding::Binding(
    View::Bytes name,
    ttx_abstract target,
    const Ttx::Concept::Documentation& documentation)
    : name(name), documentation(documentation), target(target) {}

Binding::Binding(
    View::Bytes name,
    const Ttx::Concept::Documentation& documentation)
    : Binding(name, ttx_unknown(), documentation) {}

auto Binding::get_name() const -> View::Bytes {
  return name;
}

auto Binding::get_documentation() const -> const Ttx::Concept::Documentation& {
  return documentation;
}

auto Binding::bind_target(ttx_abstract selected) -> bool {
  if (selected == nullptr || ttx_abstract_same(selected, get_abi())) {
    return false;
  }
  if (!ttx_abstract_same(target, ttx_unknown())) {
    return ttx_abstract_same(target, selected);
  }
  target = selected;
  return true;
}

auto Binding::resolve(ttx_abstract) const -> ttx_abstract {
  return Ttx::resolve(target);
}

auto Binding::resolve_concept(ttx_borrowed_bytes) const -> ttx_abstract {
  return ttx_abstract_same(target, ttx_unknown()) ? ttx_unknown() : ttx_none();
}
