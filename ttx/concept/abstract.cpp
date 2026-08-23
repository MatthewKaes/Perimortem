// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Ttx::Concept;

auto Abstract::resolve_access(const Abstract&, Perimortem::Core::View::Bytes)
    const -> const Abstract& {
  return Invalid::get_invalid();
}

auto Abstract::resolve_call(const Abstract&, Perimortem::Core::View::Bytes)
    const -> const Abstract& {
  return Invalid::get_invalid();
}
