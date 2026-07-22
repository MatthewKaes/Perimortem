// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Model::Expressions {

// Call is a compile-time/runtime initializer value that invokes one real
// Callable. Executable function bodies use Body Call operations instead. This
// Expression exists only where a durable value initializer owns the call.
class Call final : public Ttx::Model::Expression {
 public:
  using ContractOwner = Call;
  static constexpr Perimortem::System::Uuid contract_id{
    0x6c360823714e40b2,
    0x97f0c9e3b14cedfc,
  };

  Call(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Model::Callable& callable,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Expression>> arguments);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Expression::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return {};
  }
  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return type.get();
  }
  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return inputs;
  }
  constexpr auto get_callable() const -> const Ttx::Model::Callable& {
    return callable.get();
  }

 private:
  Ttx::Concept::Reference<Ttx::Model::Callable> callable;
  Ttx::Concept::Reference<Ttx::Model::Type> type;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      arguments;
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Model::Expressions
