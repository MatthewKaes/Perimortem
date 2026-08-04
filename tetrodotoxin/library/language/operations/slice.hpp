// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Slice is the semantic index or contiguous range operation. It retains two
// inputs for receiver and index, or three inputs for receiver, start, and size.
// Operation owns replacement and authored ordering, leaving Slice to derive
// the result Type and evaluate the live Constant Bytes payload domain.
class Slice : public Operation {
 public:
  using ClassCatagory = Slice;
  static constexpr Perimortem::System::Uuid contract_id{
    0x6beea0412c0b4d4e,
    0x958a39337c8ced0f,
  };

  // Consumes one complete Slice postfix for the supplied receiver. Recursive
  // operands use the Expression dispatcher while Slice keeps its own recovery,
  // diagnostics, construction, and eager folding transaction.
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      const Expression& receiver)
      -> Perimortem::Utility::Option<const Expression&>;

  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      const Expression& receiver,
      const Expression& index);
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      const Expression& receiver,
      const Expression& start,
      const Expression& size);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Slice"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;

  constexpr auto is_range() const -> Bool { return range; }

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError> override;

 private:
  auto get_expression(Count index) const -> const Expression&;

  Materializations& materializations;
  Bool range;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
