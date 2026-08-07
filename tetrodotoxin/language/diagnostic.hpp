// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/anchor.hpp"

namespace Tetrodotoxin::Language {

// Diagnostic is one source independent semantic failure reported while a
// Monograph links or finalizes. The owning Monograph keeps both text views
// stable in its Arena so Environment can attach the authored source later.
// Anchor retains source context independently from the Token Errors may mark.
class Diagnostic {
 public:
  constexpr Diagnostic(
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint)
      : anchor(anchor), message(message), hint(hint) {
    if (this->anchor && !this->anchor->get_span()) {
      this->anchor = {};
    }
  }

  constexpr auto get_anchor() const
      -> const Perimortem::Utility::Option<Ttx::Lexical::Anchor>& {
    return anchor;
  }

  constexpr auto get_message() const -> Perimortem::Core::View::Bytes {
    return message;
  }

  constexpr auto get_hint() const -> Perimortem::Core::View::Bytes {
    return hint;
  }

 private:
  Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor;
  Perimortem::Core::View::Bytes message;
  Perimortem::Core::View::Bytes hint;
};

}  // namespace Tetrodotoxin::Language
