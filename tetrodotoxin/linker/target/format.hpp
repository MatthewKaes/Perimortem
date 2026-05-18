// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/compiler/context/symbol.hpp"

namespace Tetrodotoxin::Linker::Target {

class Format {
 public:
  struct Section {
    Compiler::Context::Symbol::Location type;
    Perimortem::Core::View::Bytes data;
  };

  virtual ~Format() {};
  virtual auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes = 0;
};

}  // namespace Tetrodotoxin::Linker::Target
