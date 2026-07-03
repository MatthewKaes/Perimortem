// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/boot/import.hpp"
#include "tetrodotoxin/isa/boot/selection.hpp"
#include "tetrodotoxin/isa/registry.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Boot evaluates the universal source envelope for canonical TTX files.
// It reads documentation, the `dialect : Name;` ISA selection instruction, and
// import requests before resolution loads the import graph.
class Boot {
 public:
  Boot() = default;
  Boot(
      Ttx::Documentation documentation,
      Selection isa,
      Perimortem::Core::View::Vector<Import> imports)
      : documentation(documentation), isa(isa), imports(imports) {};

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Registry& registry) -> Boot*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Boot"_view;
  }

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }
  constexpr auto get_isa() const -> const Selection& { return isa; }
  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<Import> {
    return imports;
  }

 private:
  Ttx::Documentation documentation;
  Selection isa;
  Perimortem::Core::View::Vector<Import> imports;
};

}  // namespace Tetrodotoxin::Isa
