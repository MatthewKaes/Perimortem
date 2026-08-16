// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Comment consumes the consecutive authored comment prefix at the current
// cursor and materializes one compact TTX documentation block in that Cursor's
// source transaction Arena. Its line views continue to borrow the source bytes.
//
// Empty Documentation means the current Cursor does not begin with a comment
// and leaves that Cursor unchanged. Callers decide whether that absence is
// legal at the selected grammar position.
class Comment {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> const Ttx::Concept::Documentation&;
};

}  // namespace Tetrodotoxin::Language::Parser
