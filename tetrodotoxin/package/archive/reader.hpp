// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Package::Archive {

// Validates and materializes one Package Archive Format 1 envelope. Reader owns
// byte framing, semantic relationships, retained record storage, and rejection
// diagnostics. Archive receives only the completed stable views.
class Reader {
 public:
  Reader() = delete;

  // Reads one complete Format 1 envelope. Rejection publishes one scoped
  // diagnostic and retains nothing. Success borrows the input and retains its
  // record ranges in the caller Arena. The caller keeps the input valid until
  // that Arena is reset or destroyed and keeps the Arena alive while it holds
  // the returned Archive value.
  static auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes diagnostic_identity,
      Perimortem::Core::View::Bytes input)
      -> Perimortem::Utility::Option<Archive>;
};

}  // namespace Tetrodotoxin::Package::Archive
