// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/package/language/parser/name.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Source is the exact authored semantic name to normalized Package path
// binding. It never derives semantic identity from that path.
class Source {
 public:
  constexpr Source(
      Parser::Name local_name,
      Perimortem::Core::View::Bytes source_path,
      Ttx::Lexical::Span statement = {})
      : local_name(local_name),
        source_path(source_path),
        statement(statement) {}

  // Consumes one complete Source statement. An authored value retains its
  // statement coordinates for the enclosing import operation.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Source>;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name.get_view();
  }

  constexpr auto get_local_route() const -> const Parser::Name& {
    return local_name;
  }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return statement; }

 private:
  Parser::Name local_name;
  Perimortem::Core::View::Bytes source_path;
  Ttx::Lexical::Span statement;
};

}  // namespace Tetrodotoxin::Package::Language
