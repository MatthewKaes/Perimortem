// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Environment {

// Retains the real authored textual coordinates used to attach an Environment
// failure to source text. Source free restoration uses an empty Option instead
// of manufacturing an Origin for bytes that never existed.
class Origin {
 public:
  constexpr Origin(
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes body,
      Ttx::Lexical::Span span)
      : path(path), body(body), span(span) {};

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }

  constexpr auto get_body() const -> Perimortem::Core::View::Bytes {
    return body;
  }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

 private:
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes body;
  Ttx::Lexical::Span span;
};

}  // namespace Tetrodotoxin::Environment
