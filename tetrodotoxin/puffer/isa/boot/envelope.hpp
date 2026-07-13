// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "ttx/documentation.hpp"

namespace Tetrodotoxin::Puffer::Isa::Boot {

// Envelope is the evaluated source preamble for one canonical TTX source.
// It carries documentation, the selected body ISA, and import requests before
// resolution loads the import graph.
class Envelope {
 public:
  Envelope() = default;
  Envelope(
      Ttx::Documentation documentation,
      Perimortem::Core::View::Bytes isa,
      Perimortem::Core::View::Vector<Import> imports)
      : documentation(documentation), isa(isa), imports(imports) {};

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }

  constexpr auto get_isa() const -> Perimortem::Core::View::Bytes {
    return isa;
  }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<Import> {
    return imports;
  }

 private:
  Ttx::Documentation documentation;
  Perimortem::Core::View::Bytes isa;
  Perimortem::Core::View::Vector<Import> imports;
};

}  // namespace Tetrodotoxin::Puffer::Isa::Boot
