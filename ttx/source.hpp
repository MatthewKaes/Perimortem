// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/parse/cursor.hpp"
#include "ttx/parse/import.hpp"
#include "ttx/parse/symbol_path.hpp"

namespace Ttx {

// Source is the root TTX envelope parse.
//
// It reads only the universal part of a TTX file. That universal envelope is
// source documentation, the required dialect header, and zero or more imports.
// Everything after that is dialect owned body text.
//
// This two stage shape is the reason the source parser stays small. It can
// discover which dialect owns the file and which imports must be loaded without
// knowing Library syntax, Package syntax, Shader syntax, or future dialect
// bodies. Tetrodotoxin resolution then loads the requested packages or files
// before the selected dialect owns the remaining body.
class Source {
 public:
  // Parses the source envelope and leaves `cursor` at the first dialect-owned
  // token.
  //
  // Source records envelope facts only. It does not own errors, token slices,
  // body indexes, or a second cursor.
  static auto parse(
      Parse::Cursor& cursor,
      Perimortem::Core::View::Bytes source_path =
          Perimortem::Core::View::Bytes()) -> Source;

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
    return source;
  }
  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }
  constexpr auto get_dialect_name() const -> const Parse::SymbolPath& {
    return dialect_name;
  }
  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Parse::Import> {
    return imports;
  }

 private:
  Source() = default;

  static auto parse_documentation(Parse::Cursor& cursor) -> Documentation;
  static auto parse_dialect(Parse::Cursor& cursor) -> Parse::SymbolPath;

  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source;
  Documentation documentation;
  Parse::SymbolPath dialect_name;
  Perimortem::Core::View::Vector<Parse::Import> imports;
};

}  // namespace Ttx
