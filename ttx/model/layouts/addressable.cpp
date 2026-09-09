// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/addressable.hpp"

using namespace Perimortem;
using namespace Ttx::Model;

auto Layouts::Addressable::create_authored(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    const Type& type,
    Core::Option<Ttx::Concept::Abstract::Handle> source)
    -> Core::Option<Addressable&> {
  BAIL_IF(name.is_empty() || type.get_layout().is_empty());
  auto edge = source ? *source : type.get_interface();
  BAIL_IF(edge.resolve().get_identity() != &type);
  return arena.construct_from<Addressable>(
      [&]() -> Addressable { return Addressable(name, type, edge); });
}

auto Layouts::Addressable::create_synthetic(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    const Type& type) -> Addressable& {
  return arena.construct_from<Addressable>([&]() -> Addressable {
    return Addressable(name, type, type.get_interface());
  });
}
