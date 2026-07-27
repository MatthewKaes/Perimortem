// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Comment consumes the consecutive authored comment prefix at the current
// cursor and materializes one compact TTX documentation block.
//
// None means the current Cursor does not begin with a comment and leaves that
// Cursor unchanged. Callers decide whether absence is legal or should become
// canonical empty Documentation.
class Comment {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> const Ttx::Concept::Documentation&;
};

}  // namespace Tetrodotoxin::Language::Parser
