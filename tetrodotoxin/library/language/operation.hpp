// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"
#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/fold_error.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Operation is the Expression base for executable value operations. It owns
// the replaceable ordered input edges and recursively folds child Operations.
// A concrete operation supplies only its evaluation once every retained input
// is a Constant.
//
// Successful partial folds replace real child edges and keep this Operation.
// A completed evaluation is retained so repeated attempts return the same
// Expression identity without constructing duplicate values.
class Operation : public Expression {
 public:
  using ClassCatagory = Operation;
  static constexpr Perimortem::System::Uuid contract_id{
    0x7b99d8819ace4f54,
    0x84d44c33d0c23202,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }

  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return input_layout;
  }

  auto attempt_fold(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError>;

 protected:
  Operation(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs);

  virtual auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) const
      -> Perimortem::Utility::Result<const Expression&, FoldError> = 0;

 private:
  mutable Perimortem::Core::Access::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      inputs;
  Ttx::Model::Layouts::Fluid input_layout;
  mutable Perimortem::Utility::Option<Ttx::Concept::Reference<Expression>>
      folded;
};

}  // namespace Tetrodotoxin::Library::Language
