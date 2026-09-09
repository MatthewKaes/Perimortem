// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/callable.hpp"

#include "ttx/concept/unknown.hpp"

using namespace Ttx;

auto Model::Callable::bind_interface(U64 requested) const
    -> Perimortem::Utility::Result<Concept::Binding,
                                   Concept::Binding::Failure> {
  if (requested != Concept::get_type_identity<Callable>()) {
    return Abstract::bind_interface(requested);
  }

  if (resolve().is<Concept::Unknown>()) {
    return Concept::Binding::Failure::Pending;
  }

  static const Operations operations = {
    [](const void* source) -> Concept::Layout::Handle {
      return static_cast<const Callable*>(source)
          ->get_parameters()
          .get_interface();
    },
    [](const void* source) -> Concept::Layout::Handle {
      return static_cast<const Callable*>(source)
          ->get_results()
          .get_interface();
    },
  };
  return Concept::Binding::provide<Callable>(this, operations);
}
