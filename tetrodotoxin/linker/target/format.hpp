// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"

namespace Tetrodotoxin::Linker::Target {

class Format {
 public:
  virtual ~Format() {}
  virtual auto add_section(Object::Section section) -> void = 0;
  virtual auto add_symbol(Object::Symbol symbol) -> void = 0;
  virtual auto add_relocation(Object::Relocation relocation) -> void = 0;
  virtual auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes = 0;
  virtual auto reset() -> void = 0;
};

}  // namespace Tetrodotoxin::Linker::Target
