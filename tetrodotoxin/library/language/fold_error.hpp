// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Language {

// FoldError names the stable recovery category for one rejected Operation
// evaluation. The Operation retains the real inputs so a caller can attach
// source context and render the rejected values without putting provenance in
// this shared decision.
enum class FoldError : Unsigned_8 {
  Unknown = Unsigned_8(-1),
  InvalidReceiverType = 0,
  InvalidOperandType,
  NegativeOperand,
  CountOverflow,
  IndexOutOfBounds,
  RangeStartOutOfBounds,
  RangeSizeOutOfBounds,
  TypeMaterializationFailure,
};

}  // namespace Tetrodotoxin::Library::Language
