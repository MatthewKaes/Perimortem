// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/lexical/class.hpp"

namespace Ttx::Lexical {

// Wrapper for tokenizing a stream.
class Token {
 public:
  Token() = default;
  Token(
      Perimortem::Core::View::Bytes data,
      Class klass,
      Bits_32 source_line,
      Bits_32 source_column)
      : data(data), klass(klass), line(source_line), column(source_column) {}

  constexpr auto matches(const Perimortem::Core::View::Bytes view) const
      -> Bool {
    return data == view;
  }

  constexpr auto get_class() const -> Class { return klass; }
  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return data;
  }
  constexpr auto get_line() const -> Bits_32 { return line; }
  constexpr auto get_column() const -> Bits_32 { return column; }

 private:
  const Perimortem::Core::View::Bytes data;
  Class klass;
  Bits_32 line = 1;
  Bits_32 column = 1;
};

static_assert(sizeof(Token) == 32);

}  // namespace Ttx::Lexical
