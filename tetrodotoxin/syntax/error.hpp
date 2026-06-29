// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/lexical/token.hpp"

namespace Tetrodotoxin::Syntax {

// Clang-style error renderer used by the parser and compiler. The parser
// buffers rendered diagnostics on Context so callers can decide when to emit
// them, while report() remains as a direct stderr convenience for older call
// sites.
class Error {
 public:
  static constexpr Count max_render_size = 4096;

  static auto render(
      Perimortem::Core::Access::Bytes target,
      Perimortem::Core::View::Bytes path,
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes(),
      Bool use_color = True) -> Count;

  static auto render(
      Perimortem::Core::View::Bytes path,
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes(),
      Bool use_color = True) -> Perimortem::Memory::Dynamic::Bytes;

  static auto report(
      Perimortem::Core::View::Bytes path,
      const Lexical::Token& token,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;

  static auto report(
      Perimortem::Core::View::Bytes path,
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;
};

}  // namespace Tetrodotoxin::Syntax
