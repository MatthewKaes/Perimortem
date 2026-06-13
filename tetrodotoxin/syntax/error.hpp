// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/lexical/token.hpp"

namespace Tetrodotoxin::Syntax {

// Clang-style colored error printer used by the parser and compiler.
// Currently writes immediately to stderr so that it doesn't need to hold on to
// memory past the report call.
//
// The message passed will be provided wrapped in all of the helper code that
// points to the offending token.
//
// A hint can be provided to propose possible resolutions to the user.
class Error {
 public:
  static auto report(
      Perimortem::Core::View::Bytes path,
      const Lexical::Token& token,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;
};

}  // namespace Tetrodotoxin::Syntax
