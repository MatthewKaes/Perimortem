// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/body.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/structured.hpp"

namespace Tetrodotoxin::Model::Apps {

// Lifecycle is an ordinary Callable which retains the common host Body
// produced by App evaluation. Its role is assigned by Program's direct edge.
// neither its name nor a runtime search determines start/frame/stop policy.
class Lifecycle final : public Ttx::Model::Callable {
 public:
  using ContractOwner = Lifecycle;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe2f46646b1da4aad,
    0x96cdfd8a6c0ba674,
  };

  Lifecycle(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> parameters,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> results,
      Ttx::Model::Body body,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Callable::implements(requested);
  }
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
  constexpr auto get_body() const -> const Ttx::Model::Body& { return body; }

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
  Ttx::Model::Body body;
};

}  // namespace Tetrodotoxin::Model::Apps
