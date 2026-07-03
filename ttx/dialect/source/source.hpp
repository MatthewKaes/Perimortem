// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/dialect/source/dialect.hpp"
#include "ttx/dialect/source/import.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect::Source {

// Defines the header envelope and is the root TTX parser for canonical source
// files.
//
// Any arbitrary TTX can be parsed if you know the required Dialect, but for the
// source IR format we provide TTX as a "wrapper" Dialect which is used to
// describe the _actual_ Dialect to start with.
//
// It also parses some general concepts that are useful to TTX related
// toolchains such as imports and top level documentation. While nothing in the
// TTX spec requires the use of TTX headers, source lacking them will have a
// hard time being ingested by the Tetrodotoxin toolchain unless the program
// knows exactly what it's doing.
class Source {
 public:
  Source() = default;
  Source(
      Documentation documentation,
      Dialect dialect,
      Perimortem::Core::View::Vector<Import> imports)
      : documentation(documentation), dialect(dialect), imports(imports) {};

  // TODO: Replace with the type erased source blob with stored type info.
  static auto parse(Lexical::Cursor& cursor) -> void*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Source"_view;
  }

  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }
  constexpr auto get_dialect() const -> const Dialect& { return dialect; }
  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<Import> {
    return imports;
  }

 private:
  Documentation documentation;
  Dialect dialect;
  Perimortem::Core::View::Vector<Import> imports;
};

}  // namespace Ttx::Dialect::Source
