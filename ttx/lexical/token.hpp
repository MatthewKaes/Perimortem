// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/lexical/code.hpp"

namespace Ttx::Lexical {

// A compact representation of a token that is optimized for TTX's format sizes.
// Can be expanded to 16 bytes if later it turns out we need to support larger
// text values, but given the TTX formating spec all valid TTX should fix inside
// these limits.
class Token {
 public:
  Token() = default;
  Token(U16 offset, U16 line, U16 column, U8 size, Code code)
      : offset(offset), line(line), column(column), size(size), code(code) {}

  constexpr operator bool() const { return bool(is_valid()); }

  constexpr auto caculate_text(Perimortem::Core::View::Bytes source) const
      -> Perimortem::Core::View::Bytes {
    return source.slice(get_offset(), get_size());
  }

  constexpr auto is_valid() const -> Bool {
    return code != Code::Type::Terminal;
  }

  constexpr auto get_offset() const -> U16 { return offset; }
  constexpr auto get_line() const -> U16 { return line; }
  constexpr auto get_column() const -> U16 { return column; }
  constexpr auto get_size() const -> U8 { return size; }
  constexpr auto get_code() const -> Code { return code; }

 private:
  U16 offset = 0;
  U16 line = 0;
  U16 column = 0;
  U8 size = 0;
  Code code = Code::Type::Terminal;
};

static_assert(sizeof(Token) == 8);

}  // namespace Ttx::Lexical
