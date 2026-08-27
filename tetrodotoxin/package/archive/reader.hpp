// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"

namespace Tetrodotoxin::Package::Archive {

// Validates and materializes one Package Archive envelope. Format 2 preserves
// the resource free contract, while later formats restore Package Resources
// and the external Type graph before member Dialects consume their routes.
// Reader owns framing, relationships, retained record storage, and low level
// validation logs.
class Reader {
 public:
  Reader() = delete;

  enum class Error {
    InvalidFormat,
    UnsupportedFormat,
  };

  // Reads one complete supported envelope. Result exposes exactly
  // Archive or Error. Rejection logs the exact validation stage and retains
  // nothing.
  // The caller that knows why this Archive was requested decides whether
  // failure becomes a textual source diagnostic. Success borrows the input and
  // retains its record ranges in the caller Arena. The caller keeps the input
  // valid until that Arena is reset or destroyed and keeps the Arena alive
  // while it holds the returned value.
  static auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes input)
      -> Perimortem::Utility::Result<Archive, Error>;
};

}  // namespace Tetrodotoxin::Package::Archive
