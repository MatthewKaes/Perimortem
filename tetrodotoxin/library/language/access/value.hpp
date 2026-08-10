// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Value is the safe indexed element or contiguous range access. It retains two
// inputs for receiver and index, or three inputs for receiver, start, and
// count. A missing element produces its Type default, while a missing range
// produces the default empty View and an oversized range stops at the receiver
// boundary. Writable reference selection belongs to the separate bracket access
// form.
class Value : public Operation {
 public:
  TTX_CONTRACT(Value, Operation, 0x6beea0412c0b4d4e, 0x958a39337c8ced0f);

  // Consumes one complete value postfix for the supplied receiver. Recursive
  // operands use the Expression dispatcher while Value owns the postfix
  // grammar recovery and construction of one authored operation.
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& index,
      Ttx::Lexical::Anchor anchor) -> Value&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& index) -> Value&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& start,
      Expression& count,
      Ttx::Lexical::Anchor anchor) -> Value&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Expression& receiver,
      Expression& start,
      Expression& count) -> Value&;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Value"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error> override;
  auto select_type(Materializations& materializations) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> override;

 private:
  Value(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Access
