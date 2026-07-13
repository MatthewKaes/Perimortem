// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/implementation.hpp"
#include "ttx/lexical/source.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Lowering {

// Resolved facts presented to one selected ISA lowerer. This is a borrowed
// transaction input, not a second source record or a cache. The source record
// remains the owner of the type, implementation, content, and module name.
struct Input {
  Ttx::Lexical::Source source;
  Perimortem::Core::View::Bytes module;
  const Ttx::Type& type;
  const Tetrodotoxin::Isa::Base::Implementation& implementation;
};

}  // namespace Tetrodotoxin::Isa::Lowering
