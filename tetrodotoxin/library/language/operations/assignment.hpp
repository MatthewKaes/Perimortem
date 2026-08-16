// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Assignment is the lowest precedence `=` Expression operator. It retains one
// writable target and the complete Pack written to it. The empty output Layout
// records an effect without fabricating a result value.
class Assignment : public Expression {
 public:
  TTX_CONTRACT(Assignment, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& left,
      Ttx::Lexical::Span left_span) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  TTX_NAME("Assignment"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_target() const -> const Expression& { return target; }

  constexpr auto get_source() const -> const Model::Pack& { return source; }

 private:
  constexpr Assignment(
      Expression& target,
      Model::Pack& source,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), target(target), source(source) {}

  Expression& target;
  Model::Pack& source;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
