// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// AddAssignment owns the explicit `+=` read, modify, and write operator. Its
// target and right operand are the only two graph edges, so lowering never has
// to deduplicate a hidden Add expression before writing the selected address.
class AddAssignment : public Expression {
 public:
  TTX_CONTRACT(AddAssignment, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& target,
      Model::Pack& right,
      Ttx::Lexical::Anchor anchor) -> AddAssignment&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  TTX_NAME("AddAssignment"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Unknown::get_unknown();
  }

  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_target() const -> const Expression& { return target; }

  constexpr auto get_right() const -> const Model::Pack& { return right; }

 private:
  constexpr AddAssignment(
      Expression& target,
      Model::Pack& right,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), target(target), right(right) {}

  Expression& target;
  Model::Pack& right;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
