// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/namespace.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model {

// Render is the semantic Type contract for one language-neutral submission
// value. Its ordinary Layout is host visible state. Constants, push values,
// resources, and required Stage Callables remain real queryable owners.
class Render : public Ttx::Model::Type {
 public:
  using ContractOwner = Render;
  static constexpr Perimortem::System::Uuid contract_id{
    0x3807211361c54468,
    0x93bd741fe3480d98,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Ttx::Model::Type::implements(requested);
  }

  virtual constexpr auto get_constant_count() const -> Count = 0;
  virtual auto get_constant(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_push_count() const -> Count = 0;
  virtual auto get_push(Count index) const -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_resource_count() const -> Count = 0;
  virtual auto get_resource(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_stage_count() const -> Count = 0;
  virtual auto get_stage(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_constants_namespace() const
      -> const Model::Namespace& = 0;
  virtual constexpr auto get_pushes_namespace() const
      -> const Model::Namespace& = 0;
  virtual constexpr auto get_resources_namespace() const
      -> const Model::Namespace& = 0;
};

}  // namespace Tetrodotoxin::Model
