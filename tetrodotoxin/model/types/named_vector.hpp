// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/addressables/field.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/types/vector.hpp"

namespace Tetrodotoxin::Model::Types {

// NamedVector is a declared vector Type whose lanes have stable semantic names
// such as x, y, z, and w. It is distinct from the positional Vec[T, N] Generic
// result even when both values use the same element Type and extent.
class NamedVector final : public Ttx::Model::Types::Vector {
 public:
  using ContractOwner = NamedVector;
  static constexpr Perimortem::System::Uuid contract_id{
    0x41490c1e806e4c55,
    0x9a1209cc35272eb4,
  };

  NamedVector(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element_type,
      Count element_count);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Types::Vector::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_element_type() const -> const Ttx::Model::Type& override {
    return element_type.get();
  }
  constexpr auto get_element_count() const -> Count override {
    return fields.get_size();
  }
  constexpr auto get_member_count() const -> Count {
    return members.get_root_count();
  }
  auto get_member(Count index) const -> const Ttx::Concept::Abstract&;

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "A named vector is one inline vector value with stable addressable component names. Use it when lane names and vector operations are both semantic facts."_view,
  };
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Type> element_type;
  Members members;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      fields;
  Ttx::Model::Layouts::Structured layout;
};

}  // namespace Tetrodotoxin::Model::Types
