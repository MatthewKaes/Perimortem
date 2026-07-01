// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/parse/error.hpp"
#include "ttx/parse/import.hpp"

namespace Ttx::Parse {

class Source {
 public:
  static auto parse(
      const Lexical::Tokenizer& tokenizer,
      Perimortem::Core::View::Bytes source_path =
          Perimortem::Core::View::Bytes()) -> Source;

  auto get_source_path() const -> Perimortem::Core::View::Bytes;
  auto get_source() const -> Perimortem::Core::View::Bytes;
  auto get_documentation() const -> Documentation;
  auto get_dialect_name() const -> const TypePath&;
  auto get_imports() const -> Perimortem::Core::View::Vector<Import>;
  auto get_body_tokens() const -> Perimortem::Core::View::Vector<Lexical::Token>;
  auto get_body_token_index() const -> Count;

  auto is_valid() const -> Bool;
  auto has_error() const -> Bool;
  auto get_errors() const -> Perimortem::Core::View::Vector<Error>;
  auto get_error_count() const -> Count;

 private:
  static auto parse_documentation(Cursor& cursor) -> Documentation;
  static auto parse_dialect(Cursor& cursor) -> TypePath;

  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source;
  Documentation documentation;
  TypePath dialect_name;
  Perimortem::Core::View::Vector<Import> imports;
  Perimortem::Core::View::Vector<Lexical::Token> body_tokens;
  Count body_token_index = 0;
  Perimortem::Core::View::Vector<Error> errors;
};

}  // namespace Ttx::Parse
