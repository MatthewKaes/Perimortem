// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Ttx::Model {

// Constant is an immutable Expression already in normal form. It defines no
// parser, operator set, evaluator, or lowering representation. Concrete value
// domains expose their payload through derived contracts, allowing tools and
// ISAs to add constants without extending a central tag.
//
// Constants have no evaluation inputs. Equality includes resolved Type identity
// as well as the derived value, preserving the distinction between equal bits
// interpreted by different Types.
class Constant : public Expression {
 public:
  using ContractOwner = Constant;
  static constexpr Perimortem::System::Uuid contract_id{
    0xba0cda6e761646bc,
    0x99c434aed9d840fa,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }

  auto get_inputs() const -> const Layout& final { return inputs; }

  virtual auto equals(const Constant& rhs) const -> Bool = 0;

  auto operator==(const Constant& rhs) const -> Bool { return equals(rhs); }
  auto operator!=(const Constant& rhs) const -> Bool { return !equals(rhs); }

 protected:
  auto has_same_type(const Constant& rhs) const -> Bool {
    const Abstraction::Abstract& lhs_type = get_type().resolve();
    const Abstraction::Abstract& rhs_type = rhs.get_type().resolve();
    return lhs_type.is<Type>() && rhs_type.is<Type>() && &lhs_type == &rhs_type;
  }

 private:
  inline static const Layouts::Fluid inputs;
};

}  // namespace Ttx::Model
