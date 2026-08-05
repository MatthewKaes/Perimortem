// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// NotEqual owns exact semantic value inequality. Scalar and complete Bytes
// domains keep their own payload contracts while this operation retains only
// operand edges, their selected Type identity, and the canonical Bool result.
class NotEqual : public Operation {
 public:
  using ClassCatagory = NotEqual;
  static constexpr Perimortem::System::Uuid contract_id{
    0xa2c035278af343f7,
    0xa088f67899453ad2,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      const Expression& left) -> Perimortem::Utility::Option<const Expression&>;

  NotEqual(
      Perimortem::Memory::Allocator::Arena& domain,
      const Expression& left,
      const Expression& right);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Operation::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "NotEqual"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;

 protected:
  auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError> override;

 private:
  const Ttx::Concept::Abstract& operand_type;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
