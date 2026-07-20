// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Model {

// Definition maps one definition Dialect to the modifier classes accepted by
// its parent grammar. It does not own a context or retain the result it helps
// produce. Definitions owns that transaction and uses this mapping only to
// select the Dialect and validate its authored prefix.
//
// The selected Dialect receives the real modifier tokens because it owns the
// semantic meaning of each accepted combination. No modifier record or runtime
// registry is constructed.
template <typename dialect_type, Ttx::Lexical::Code::Type... modifiers>
class Definition {
  static_assert(
      sizeof...(modifiers) > 0,
      "A Definition must accept at least one modifier.");
  static_assert(
      (Ttx::Lexical::Code(modifiers).is_modifier() && ...),
      "A Definition may accept only lexical modifier classes.");

 public:
  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return dialect_type::name;
  }

  static constexpr auto accepts(
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> prefix) -> Bool {
    if (prefix.is_empty()) {
      return False;
    }

    for (Count i = 0; i < prefix.get_size(); i++) {
      const Ttx::Lexical::Code::Type modifier =
          prefix[i].get_code().get_type();
      if (!((modifier == modifiers) || ...)) {
        return False;
      }
    }

    return True;
  }

  template <typename definitions_type>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> prefix,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract& {
    if constexpr (requires {
                    dialect_type::template evaluate<definitions_type>(
                        cursor, name, documentation, prefix, visible);
                  }) {
      return dialect_type::template evaluate<definitions_type>(
          cursor, name, documentation, prefix, visible);
    } else {
      return dialect_type::evaluate(
          cursor, name, documentation, prefix, visible);
    }
  }
};

}  // namespace Tetrodotoxin::Model
