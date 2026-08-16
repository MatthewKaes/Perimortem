// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Slice is the safe indexed element or contiguous range access. Element access
// supplies one scalar value. Range access supplies a fixed size Pack whose real
// producer remains this Slice expression. It does not eagerly materialize a
// View or anonymous aggregate Type. Writable reference selection belongs to the
// separate bracket access form.
class Slice : public Expression {
 public:
  TTX_CONTRACT(Slice, Expression);

  // Consumes one complete value postfix for the supplied receiver. Recursive
  // operands use the Expression dispatcher while Slice owns the postfix
  // grammar recovery and construction of one authored Expression.
  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& index,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& start,
      Expression& count,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  TTX_NAME("Slice"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto fits(const Ttx::Model::Type& target) const -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& index,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& start,
      Expression& count,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Expression& receiver;
  Expression& first;
  Perimortem::Core::Option<Ttx::Concept::Reference<Expression>> count;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>> fallback;
  Perimortem::Core::Option<Count> range_count;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> range_layout;
};

}  // namespace Tetrodotoxin::Library::Language::Access
