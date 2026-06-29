// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/lexical/class.hpp"

namespace Tetrodotoxin::Format {

constexpr Count soft_line_limit = 100;

// Stateful output object used while formatting syntax nodes. It owns only
// generic text layout state; AST-specific formatting lives with the AST types.
class Formatter {
 public:
  Formatter() = default;

  auto get_output() const -> const Perimortem::Memory::Dynamic::Bytes& {
    return out;
  }

  auto clear() -> void {
    out.clear();
    indent = 0;
    trailing_newlines = 1;
    column = 0;
    in_line = False;
  }

  auto operator<<(Perimortem::Core::View::Bytes text) -> Formatter&;
  auto operator<<(Lexical::Class klass) -> Formatter& {
    return *this << klass.get_source_text();
  }
  auto operator<<(Lexical::Class::Type type) -> Formatter& {
    return *this << Lexical::Class::get_source_text(type);
  }

  // Adds a new line and tracks if it creates a blank line.
  auto emit_newline() -> void;

  // Ensures there is a blank line if there isn't one already.
  auto ensure_blank() -> void;
  auto do_indent() -> void;
  auto emit_padding(Count target, Count current) -> void;
  auto current_line_width() const -> Count;

  Perimortem::Memory::Dynamic::Bytes out;
  Count indent = 0;
  Count trailing_newlines = 1;
  Count column = 0;
  Bool in_line = False;
};

}  // namespace Tetrodotoxin::Format
