// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Slice is the semantic index or contiguous range operation. It retains two
// inputs for receiver and index, or three inputs for receiver, start, and size.
// Operation owns authored ordering and immutable edges. Slice retains the
// result Type chosen from those inputs and evaluates the live Constant Bytes
// payload domain without changing that graph contract during folding.
class Slice : public Operation {
 public:
  using ClassCatagory = Slice;
  static constexpr Perimortem::System::Uuid contract_id{
    0x6beea0412c0b4d4e,
    0x958a39337c8ced0f,
  };

  // Consumes one complete Slice postfix for the supplied receiver. Recursive
  // operands use the Expression dispatcher while Slice owns the postfix
  // grammar recovery and construction of one authored operation.
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& receiver) -> Perimortem::Utility::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& index,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& index) -> Slice&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& start,
      Expression& size,
      Ttx::Lexical::Anchor anchor) -> Slice&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& start,
      Expression& size) -> Slice&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Slice"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  constexpr auto is_range() const -> Bool { return range; }

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations)
      -> Perimortem::Utility::Result<
          Perimortem::Utility::Option<Constant&>,
          Expression::Error> override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override;

 private:
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor);

  Bool range;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
