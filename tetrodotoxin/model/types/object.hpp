// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/addressables/field.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/types/managed.hpp"

namespace Tetrodotoxin::Model::Types {

// Object is the Library aggregate whose values prove the shared Managed Type
// contract. Its fields remain semantic tracing inputs. Collector headers and
// target pointer representation do not enter this Type or its Layout.
class Object final : public Ttx::Model::Types::Managed {
 public:
  using ContractOwner = Object;
  static constexpr Perimortem::System::Uuid contract_id{
    0x787840d67fd94545,
    0x918983ad9107af83,
  };

  Object(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Types::Managed::implements(requested);
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
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto add_field(const Addressables::Field& field, Bool publish) -> Bool;
  auto add_member(const Ttx::Concept::Abstract& member, Bool publish) -> Bool;
  auto complete() -> Bool;

  constexpr auto get_member_count() const -> Count {
    return members.get_root_count();
  }
  auto get_member(Count index) const -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Members members;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      fields;
  Ttx::Model::Layouts::Structured layout;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Model::Types
