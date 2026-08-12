// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Language {

// Writability is the Library mutation policy retained by declarations that
// create an Addressable. Selection and visibility remain separate facts.
enum class Writability : ::Unsigned_8 {
  Full,
  Internal,
  Init,
};

}  // namespace Tetrodotoxin::Library::Language
