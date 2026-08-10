// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Constant is an immutable Expression already in normal form. It defines no
// parser, operator set, evaluation engine, or lowering representation. Concrete
// value domains expose their payload through derived contracts without
// extending a central tag.
//
// Constants have no evaluation inputs. Equality includes resolved Type identity
// as well as the derived value, preserving the distinction between equal bits
// interpreted by different Types.
class Constant : public Expression {
 public:
  TTX_CONTRACT(Constant, Expression, 0xba0cda6e761646bc, 0x99c434aed9d840fa);

  // Constant semantic names come from their exact Type. A receiving owner may
  // retain the value through a real Alias or Addressable without renaming this
  // immutable identity.
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return get_type().get_name();
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
  constexpr explicit Constant(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor) {}

  constexpr auto has_same_type(const Constant& rhs) const -> Bool {
    const Ttx::Concept::Abstract& lhs_type = get_type().resolve();
    const Ttx::Concept::Abstract& rhs_type = rhs.get_type().resolve();
    return lhs_type.is<Ttx::Model::Type>() && rhs_type.is<Ttx::Model::Type>() &&
           &lhs_type == &rhs_type;
  }

  auto fold_uncached() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Expression&>, Error> override {
    return Perimortem::Core::Option<Expression&>(*this);
  }

 private:
  static constexpr Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language
