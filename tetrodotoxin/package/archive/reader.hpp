// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/archive/read_error.hpp"

namespace Tetrodotoxin::Package::Archive {

// Validates and materializes one Package Archive Format 1 envelope. Reader owns
// byte framing, semantic relationships, retained record storage, and low level
// validation logs. Archive receives only the completed stable views.
class Reader {
 public:
  Reader() = delete;

  // Reads one complete Format 1 envelope. Result exposes exactly Archive or
  // ReadError. Rejection logs the exact validation stage and retains nothing.
  // The caller that knows why this Archive was requested decides whether
  // failure becomes a textual source diagnostic. Success borrows the input and
  // retains its record ranges in the caller Arena. The caller keeps the input
  // valid until that Arena is reset or destroyed and keeps the Arena alive
  // while it holds the returned value.
  static auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes input)
      -> Perimortem::Utility::Result<Archive, ReadError>;
};

}  // namespace Tetrodotoxin::Package::Archive
