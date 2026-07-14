// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/abi/type.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

auto Abi::Type::create(
    Allocator::Arena& arena,
    View::Bytes public_path,
    const Ttx::Type& type) -> Type {
  Managed::Bytes path(arena, public_path);
  return Type(path.get_view(), type);
}
