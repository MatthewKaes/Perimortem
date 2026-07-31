// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/object/module.hpp"

namespace Tetrodotoxin::Linker::Target {

// Format encodes one supplied Module without retaining its inventory. Concrete
// targets keep descriptors and output storage inside one encoding transaction.
class Format {
 public:
  virtual ~Format() {}
  virtual auto build_library(
      const Object::Module& module,
      Perimortem::Core::View::Bytes object_name) const
      -> Perimortem::Memory::Dynamic::Bytes = 0;
};

}  // namespace Tetrodotoxin::Linker::Target
