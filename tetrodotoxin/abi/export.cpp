// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/abi/export.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/abi/symbol.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

auto Abi::Export::create(
    Allocator::Arena& arena,
    View::Bytes public_path,
    const Ttx::Function& function,
    View::Bytes target_symbol,
    View::Vector<Abi::Type> type_identities) -> const Export* {
  if (public_path.is_empty() || function.is_empty() ||
      target_symbol.is_empty()) {
    return nullptr;
  }

  Managed::Bytes path(arena, public_path);
  View::Bytes symbol =
      Abi::Symbol::exported(arena, path, function, type_identities);
  if (symbol.is_empty()) {
    return nullptr;
  }

  Export output(path.get_view(), function, symbol, target_symbol);
  return &arena.construct<Export>(output);
}

auto Abi::Export::project(
    Allocator::Arena& arena,
    View::Bytes public_path,
    const Export& source) -> const Export* {
  if (public_path.is_empty()) {
    return nullptr;
  }

  Managed::Bytes path(arena, public_path);
  Export output(
      path.get_view(), source.get_function(), source.get_symbol(),
      source.get_target_symbol());
  return &arena.construct<Export>(output);
}
