// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/layouts/addressable.hpp"

using namespace Perimortem;
using namespace Ttx::Model;

auto Layouts::Addressable::create_authored(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    const Type& type) -> Core::Option<Addressable&> {
  BAIL_IF(name.is_empty() || type.get_layout().is_empty());
  return arena.construct_from<Addressable>(
      [&]() -> Addressable { return Addressable(name, type); });
}

auto Layouts::Addressable::create_synthetic(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes name,
    const Type& type) -> Addressable& {
  return arena.construct_from<Addressable>(
      [&]() -> Addressable { return Addressable(name, type); });
}
