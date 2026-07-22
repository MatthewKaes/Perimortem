// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Parser {

// Type restores the original Abstract-era progressive reference parser. It
// consumes authored Type segments and returns a borrow of the real resolved
// semantic Type. Parse failure is Utility::None because parser control flow is
// not a semantic Abstract query and therefore must not leak Invalid into the
// graph.
class Type {
 public:
  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context)
      -> Perimortem::Utility::Option<const Ttx::Model::Type&>;
};

}  // namespace Tetrodotoxin::Parser
