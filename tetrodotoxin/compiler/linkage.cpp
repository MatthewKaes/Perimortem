// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/linkage.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

auto Compiler::Linkage::internal(
    Allocator::Arena& arena,
    const Ttx::Type& owner,
    const Ttx::Function& function,
    View::Bytes module) -> Linkage {
  if (module.is_empty() || function.get_name().is_empty()) {
    return Linkage(owner, function, View::Bytes());
  }

  Managed::Bytes symbol(arena);
  auto append = [](Managed::Bytes& output, View::Bytes value) {
    for (Count i = 0; i < value.get_size(); i++) {
      Bits_8 byte = value[i];
      Bool alpha = (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z');
      Bool digit = byte >= '0' && byte <= '9';
      output.append(alpha || digit || byte == '_' ? byte : Bits_8('_'));
    }
  };

  symbol.concat("TTX_"_view);
  append(symbol, module);
  symbol.append('_');
  append(symbol, function.get_name());
  return Linkage(owner, function, symbol.get_view());
}
