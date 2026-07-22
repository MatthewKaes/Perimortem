// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/target/spir_v/compiler.hpp"

namespace Tetrodotoxin::Target::SpirV {

// Validator is the in-process publication gate for generated modules. It
// checks instruction bounds, logical section order, id bounds/definitions,
// entry/function structure, and the interface facts promised by Metadata.
class Validator {
 public:
  static auto validate(
      Perimortem::Core::View::Bytes words,
      const Metadata& expected) -> Bool;
};

}  // namespace Tetrodotoxin::Target::SpirV
