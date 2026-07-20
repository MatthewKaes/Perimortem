// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Interpreter::Dialects {

// Alias is the grammar handler for an authored Abstract redirection.
// Definitions selects it as a compile time Definition, then Alias resolves the
// target and constructs the durable TTX Alias directly from the passed facts.
class Alias final {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "alias"_view;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract&;

 private:
  static auto unresolved_target_error(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& target) -> void;

  static auto resolve_target(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract&;
};

}  // namespace Tetrodotoxin::Interpreter::Dialects
