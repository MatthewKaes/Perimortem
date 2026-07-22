// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/addressables/parameter.hpp"

namespace Tetrodotoxin::Model::Interfaces {

// Builtin is a Stage interface slot with a source-authored platform-neutral
// shader role. The SPIR-V planner maps the closed role contract to a target
// decoration without inspecting the slot's name.
class Builtin final : public Addressables::Parameter {
 public:
  enum class Role : Unsigned_8 {
    Position,
    VertexIndex,
  };

  using ContractOwner = Builtin;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb0cbb91603964f56,
    0x94c670cff9ac2cef,
  };

  constexpr Builtin(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      Role role)
      : Parameter(name, type), role(role) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Parameter::implements(requested);
  }
  constexpr auto get_role() const -> Role { return role; }

 private:
  Role role;
};

}  // namespace Tetrodotoxin::Model::Interfaces
