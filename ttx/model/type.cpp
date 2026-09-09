// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/type.hpp"

#include "ttx/concept/unknown.hpp"

using namespace Ttx;

auto Model::Type::bind_interface(Perimortem::System::Uuid requested) const
    -> Perimortem::Utility::
        Result<Concept::Binding, Concept::Binding::Failure> {
  if (requested != Type::contract_id) {
    return Abstract::bind_interface(requested);
  }

  if (resolve().is<Concept::Unknown>()) {
    return Concept::Binding::Failure::Pending;
  }

  static const Operations operations = {
    [](const void* source) -> Concept::Layout::Handle {
      return static_cast<const Type*>(source)->get_layout().get_interface();
    },
  };
  return Concept::Binding::provide<Type>(this, operations);
}
