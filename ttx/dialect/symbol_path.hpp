// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect {

// A package/type style symbol reference written as Type(::Type)*.
//
// SymbolPath is intentionally only a source view right now. The dialect that
// consumes it decides whether the text names a package, alias target, namespace
// member, or some later type-system concept.
class SymbolPath {
 public:
  SymbolPath() = default;
  SymbolPath(Perimortem::Core::View::Bytes text) : text(text) {}

  static auto parse(Lexical::Cursor& cursor) -> SymbolPath;

  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return text;
  }
  constexpr auto is_valid() const -> Bool { return !text.is_empty(); }

 private:
  Perimortem::Core::View::Bytes text;
};

}  // namespace Ttx::Dialect
