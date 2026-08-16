// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Propagate observes one Option. Presence continues with the exact element
// value while absence returns one empty Pack from the enclosing Function.
class Propagate : public Expression {
 public:
  TTX_CONTRACT(Propagate, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Propagate"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_empty_return() const -> const Model::Pack& {
    return empty_return;
  }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  constexpr Propagate(
      Expression& receiver,
      Model::Pack& empty_return,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), empty_return(empty_return) {}

  Expression& receiver;
  Model::Pack& empty_return;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
