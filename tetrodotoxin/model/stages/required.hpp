// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/named.hpp"

namespace Tetrodotoxin::Model::Stages {

// Required is a Render-owned stage Callable contract. Read edges identify the
// exact declared constant/push/resource Addressables available to an
// implementation. Input and result Layouts retain the actual parameters.
class Required : public Ttx::Model::Callable {
 public:
  enum class Execution : Unsigned_8 {
    Vertex,
    Fragment,
  };

  using ContractOwner = Required;
  static constexpr Perimortem::System::Uuid contract_id{
    0x4ca44631e4ee4238,
    0x905bf90463e1921f,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Callable::implements(requested);
  }

  Required(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Execution execution,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> parameters,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> results,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> reads,
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
  constexpr auto get_read_count() const -> Count { return reads.get_size(); }
  auto get_read(Count index) const -> const Ttx::Concept::Abstract&;
  constexpr auto get_execution() const -> Execution { return execution; }

 private:
  Perimortem::Core::View::Bytes name;
  Execution execution;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      parameters;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      results;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      reads;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      parameter_values;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      result_values;
  Ttx::Model::Layouts::Named parameter_layout;
  Ttx::Model::Layouts::Named result_layout;
};

}  // namespace Tetrodotoxin::Model::Stages
