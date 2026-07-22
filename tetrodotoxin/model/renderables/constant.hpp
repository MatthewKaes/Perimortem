// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/constant.hpp"

namespace Tetrodotoxin::Model::Renderables {

// Constant assigns the Render constant role to one real semantic Constant.
class Constant final : public Ttx::Model::Addressable {
 public:
  using ContractOwner = Constant;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdbf68ac6ff844823,
    0xb73293457d26b968,
  };

  constexpr Constant(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Constant& value,
      const Ttx::Concept::Documentation& documentation)
      : name(name), value(value), documentation(documentation) {}
  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressable::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return value.get().get_type();
  }
  constexpr auto get_value() const -> const Ttx::Model::Constant& {
    return value.get();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Constant> value;
  const Ttx::Concept::Documentation& documentation;
};

}  // namespace Tetrodotoxin::Model::Renderables
