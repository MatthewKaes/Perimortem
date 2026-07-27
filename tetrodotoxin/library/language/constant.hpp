// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Constant is an immutable Expression already in normal form. It defines no
// parser, operator set, evaluator, or lowering representation. Concrete value
// domains expose their payload through derived contracts without extending a
// central tag.
//
// Constants have no evaluation inputs. Equality includes resolved Type identity
// as well as the derived value, preserving the distinction between equal bits
// interpreted by different Types.
class Constant : public Expression {
 public:
  using ClassCatagory = Constant;
  static constexpr Perimortem::System::Uuid contract_id{
    0xba0cda6e761646bc,
    0x99c434aed9d840fa,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }

  // A Constant is a value rather than an authored declaration. When it is
  // stored under a documented name, that prose belongs to the Addressable.
  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return inputs;
  }

  virtual constexpr auto get_type() const
      -> const Ttx::Model::Type& override = 0;
  virtual constexpr auto equals(const Constant& rhs) const -> Bool = 0;

  constexpr auto operator==(const Constant& rhs) const -> Bool {
    return equals(rhs);
  }
  constexpr auto operator!=(const Constant& rhs) const -> Bool {
    return !equals(rhs);
  }

 protected:
  constexpr auto has_same_type(const Constant& rhs) const -> Bool {
    const Ttx::Concept::Abstract& lhs_type = get_type().resolve();
    const Ttx::Concept::Abstract& rhs_type = rhs.get_type().resolve();
    return lhs_type.is<Ttx::Model::Type>() && rhs_type.is<Ttx::Model::Type>() &&
           &lhs_type == &rhs_type;
  }

 private:
  static constexpr Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language
