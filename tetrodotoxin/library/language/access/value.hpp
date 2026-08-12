// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Value is the safe indexed element or contiguous range access. Element access
// supplies one scalar value. Range access supplies a fixed-size Pack whose real
// producer remains this Value expression; it does not eagerly materialize a
// View or anonymous aggregate Type. Writable reference selection belongs to the
// separate bracket access form.
class Value : public Operation {
 public:
  TTX_CONTRACT(Value, Operation, 0x6beea0412c0b4d4e, 0x958a39337c8ced0f);

  // Consumes one complete value postfix for the supplied receiver. Recursive
  // operands use the Expression dispatcher while Value owns the postfix
  // grammar recovery and construction of one authored operation.
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& index,
      Ttx::Lexical::Anchor anchor) -> Value&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& index) -> Value&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& start,
      Expression& count,
      Ttx::Lexical::Anchor anchor) -> Value&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Expression& start,
      Expression& count) -> Value&;

  TTX_NAME("Value"_view);

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto fits(const Ttx::Model::Type& target) const -> Bool override;

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Constant&>,
          Expression::Error> override;
  auto select_type(Tetrodotoxin::Language::Monograph& source) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Value(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Bool range;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      range_element;
  Perimortem::Core::Option<Count> range_count;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> range_layout;
};

}  // namespace Tetrodotoxin::Library::Language::Access
