// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/abi/linkage.hpp"

#include "tetrodotoxin/abi/symbol.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

auto Abi::Linkage::type(
    Allocator::Arena& arena,
    const Ttx::Type& owner,
    const Ttx::Function& function,
    View::Bytes unit,
    View::Bytes module,
    View::Bytes owner_path) -> Linkage {
  return Linkage(
      owner, function,
      Abi::Symbol::type(arena, unit, module, owner_path, function));
}

auto Abi::Linkage::addressable(
    Allocator::Arena& arena,
    const Ttx::Type& owner,
    const Ttx::Function& function,
    View::Bytes unit,
    View::Bytes module,
    View::Bytes owner_path) -> Linkage {
  return Linkage(
      owner, function,
      Abi::Symbol::addressable(arena, unit, module, owner_path, function));
}
