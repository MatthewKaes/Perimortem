// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "ttx/function.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Abi {

// Symbol produces the machine names used at TTX ABI boundaries.
//
// Internal names identify one implementation inside a compilation unit. Export
// names identify an authored public path and remain independent of source file
// placement. Both names include the complete callable signature and dispatch
// surface, so overloads and identically named Type and Addressable functions do
// not share a linker symbol.
//
// Generated language bindings carry human-facing names. Linker names stay
// opaque so punctuation and sanitization cannot make two paths look alike.
// Two independently seeded 64-bit FNV-1a streams hash length-framed fields and
// produce the stable suffix. The hash is deterministic across targets and
// compiler processes, but it is not a security primitive. Compilations still
// reject duplicate generated symbols because a 128-bit result cannot prove that
// collisions are impossible.
class Symbol {
 public:
  // Callers select the Type or Addressable surface through the matching entry
  // point. Symbol never searches the owner to rediscover the table that the
  // caller was already enumerating.
  static auto type(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes unit,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes owner_path,
      const Ttx::Function& function) -> Perimortem::Core::View::Bytes;
  static auto addressable(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes unit,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes owner_path,
      const Ttx::Function& function) -> Perimortem::Core::View::Bytes;
  static auto exported(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes public_path,
      const Ttx::Function& function,
      Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Type> type_identities)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Abi
