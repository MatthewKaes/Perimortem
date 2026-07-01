// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/parse/cursor.hpp"
#include "ttx/parse/import.hpp"

namespace Ttx::Parse {

// Source is the root TTX envelope parse.
//
// It reads only the universal part of a TTX file. That universal envelope is
// source documentation, the required dialect header, and zero or more imports.
// Everything after that is dialect owned body text.
//
// This two stage shape is the reason the source parser stays small. It can
// discover which dialect owns the file and which imports must be loaded without
// knowing Library syntax, Package syntax, Shader syntax, or future dialect
// bodies. Tetrodotoxin resolution then loads the requested packages or files,
// binds imported types to their local names, and calls the dialect registered
// under the parsed dialect name.
//
// Source deliberately does not parse expressions, package graphs, or dialect
// bodies. Adding those here would duplicate the dispatch model and turn the
// envelope into another AST layer.
//
// Dialect body parsing, package graph loading, imported type lookup, and
// expression resolution belong to later owners. Tetrodotoxin resolution can
// prepare the correct imported context before dialect dispatch, and each
// dialect can then create the enriched IR it owns without the source envelope
// becoming another AST layer.
class Source {
 public:
  // Parses the source envelope and leaves `cursor` at the first dialect-owned
  // token.
  //
  // This keeps parsing as one continuous state. Tetrodotoxin can inspect
  // imports, decide whether existing cursor errors are recoverable, resolve the
  // requested packages, then pass the same cursor to the registered dialect.
  // Source records the envelope facts only. It does not own errors, token
  // slices, body indexes, or a second cursor.
  static auto parse(
      Cursor& cursor,
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
  constexpr auto get_dialect_name() const -> const TypePath& {
    return dialect_name;
  }
  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Import> {
    return imports;
  }

 private:
  Source() = default;

  static auto parse_documentation(Cursor& cursor) -> Documentation;
  static auto parse_dialect(Cursor& cursor) -> TypePath;

  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source;
  Documentation documentation;
  TypePath dialect_name;
  Perimortem::Core::View::Vector<Import> imports;
};

}  // namespace Ttx::Parse
