// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// A block of consecutive comment lines. Each element in lines is a View::Bytes
// pointing directly into the tokenizer source — no copying occurs.
// The tokenizer strips the leading "//" and one optional space before emitting
// the comment text, so lines contains only the payload text.
class Comment {
 public:
  static auto parse(Context& ctx) -> Comment;

  auto get_lines() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return lines;
  }

  auto is_empty() const -> Bool { return lines.get_size() == 0; }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Tetrodotoxin::Syntax::Ast
