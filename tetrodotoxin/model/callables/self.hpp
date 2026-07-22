// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/callables/self.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/structured.hpp"

namespace Tetrodotoxin::Model::Callables {

// Self is one concrete receiver Callable. Parameter zero names the real owner
// Type and the remaining parameters retain their authored order.
class Self : public Ttx::Model::Callables::Self {
 public:
  Self(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> parameters,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> results,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameter_layout;
  }
  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return result_layout;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      parameters;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      results;
  Ttx::Model::Layouts::Structured parameter_layout;
  Ttx::Model::Layouts::Fluid result_layout;
};

}  // namespace Tetrodotoxin::Model::Callables
