// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/model/documentations/block.hpp"

namespace Tetrodotoxin::Parser {

// Comment consumes the consecutive authored comment prefix at the current
// cursor and materializes one compact TTX documentation block. Absence is a
// parser control-flow result, so a cursor without a comment returns None and
// remains untouched.
class Comment {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<const Ttx::Model::Documentations::Block&>;
};

}  // namespace Tetrodotoxin::Parser
