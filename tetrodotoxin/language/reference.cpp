// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Language::Reference::create(
    Memory::Allocator::Arena& domain,
    const Ttx::Concept::Abstract& host,
    Core::View::Bytes name) -> Reference& {
  return domain.construct_from<Reference>(
      [&]() { return Reference(host, name); });
}

auto Language::Reference::resolve() const -> const Ttx::Concept::Abstract& {
  const Ttx::Concept::Abstract& context = host->resolve();
  return context.resolve_concept(name).resolve();
}
