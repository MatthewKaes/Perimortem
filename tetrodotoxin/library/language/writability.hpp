// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Language {

// Writability is the Library mutation policy retained by declarations that
// create an Addressable. Constant marks compile-time value identity rather
// than an initialization-time write window.
enum class Writability : ::Unsigned_8 {
  Full,
  Internal,
  Constant,
};

}  // namespace Tetrodotoxin::Library::Language
