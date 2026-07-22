// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/stages/required.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/body.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/named.hpp"

namespace Tetrodotoxin::Model::Stages {

// Implemented is one Shader Stage paired with the exact required Stage it
// satisfies. Its common Body and interface Layouts are durable semantic facts.
class Implemented final : public Ttx::Model::Callable {
 public:
  using ContractOwner = Implemented;
  static constexpr Perimortem::System::Uuid contract_id{
    0xef43200864944ba6,
    0xa61c9fd5bf546989,
  };

  Implemented(
      Perimortem::Memory::Allocator::Arena& arena,
      const Required& required,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> parameters,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Addressable>> results,
      Ttx::Model::Body body,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Callable::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return required.get().get_name();
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
  constexpr auto get_required() const -> const Required& {
    return required.get();
  }
  constexpr auto get_body() const -> const Ttx::Model::Body& { return body; }

 private:
  Ttx::Concept::Reference<Required> required;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      parameters;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      results;
  Ttx::Model::Layouts::Named parameter_layout;
  Ttx::Model::Layouts::Named result_layout;
  Ttx::Model::Body body;
};

}  // namespace Tetrodotoxin::Model::Stages
