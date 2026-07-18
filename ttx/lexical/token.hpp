// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/lexical/class.hpp"

namespace Ttx::Lexical {

// A compact representation of a token that is optimized for TTX's format sizes.
// Can be expanded to 16 bytes if later it turns out we need to support larger
// text values, but given the TTX formating spec all valid TTX should fix inside
// these limits.
class Token {
 public:
  Token() = default;
  Token(
      Unsigned_16 offset,
      Unsigned_16 line,
      Unsigned_16 column,
      Unsigned_8 size,
      Class klass)
      : offset(offset), line(line), column(column), size(size), klass(klass) {}

  constexpr auto caculate_text(Perimortem::Core::View::Bytes source) const
      -> Perimortem::Core::View::Bytes {
    return source.slice(get_offset(), get_size());
  }

  constexpr auto is_valid() const -> Bool {
    return klass != Class::Type::EndOfStream;
  }

  constexpr auto get_offset() const -> Unsigned_16 { return offset; }
  constexpr auto get_line() const -> Unsigned_16 { return line; }
  constexpr auto get_column() const -> Unsigned_16 { return column; }
  constexpr auto get_size() const -> Unsigned_8 { return size; }
  constexpr auto get_class() const -> Class { return klass; }

 private:
  Unsigned_16 offset;
  Unsigned_16 line;
  Unsigned_16 column;
  Unsigned_8 size;
  Class klass = Class::Type::EndOfStream;
};

static_assert(sizeof(Token) == 8);

}  // namespace Ttx::Lexical
