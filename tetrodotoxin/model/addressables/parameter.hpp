// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model::Addressables {

// Parameter is a real named input or result slot in one Callable Layout. It is
// not runtime storage and does not prove Writable.
class Parameter : public Ttx::Model::Addressable {
 public:
  using ContractOwner = Parameter;
  static constexpr Perimortem::System::Uuid contract_id{
    0x1738260603b24e60,
    0xa3330951197667ab,
  };

  constexpr Parameter(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Documentation::get_empty())
      : name(name), type(type), documentation(documentation) {}

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
    return type.get();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Type> type;
  const Ttx::Concept::Documentation& documentation;
};

}  // namespace Tetrodotoxin::Model::Addressables
