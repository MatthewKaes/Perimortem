// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Parser {

// Comment consumes the consecutive authored comment prefix at the current
// cursor and materializes one compact TTX documentation block.
//
// A valid Documentation is always returned. If no comment is present then the
// canonical empty Documentation is returned without consuming the cursor.
//
// For required document level comments, every parser that processes the TTX
// file header first can validate that the token stream starts with a `Comment`
// token rather than querying if the document result is `Empty`.
class Comment {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> const Ttx::Concept::Documentation&;
};

}  // namespace Tetrodotoxin::Parser
