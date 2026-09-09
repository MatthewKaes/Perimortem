// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/concept/scope.hpp"

using namespace Ttx::Concept;

auto Abstract::bind_interface(U64 requested) const
    -> Perimortem::Utility::Result<Binding, Binding::Failure> {
  if (requested != get_type_identity<Scope>()) {
    return Binding::Failure::Unsupported;
  }

  static const Scope::Operations operations = {
    [](const void* source,
       Perimortem::Core::View::Bytes name) -> Abstract::Handle {
      return static_cast<const Abstract*>(source)
          ->resolve_concept(name)
          .get_interface();
    },
    [](const void* source, Scope::Visitor visitor) -> void {
      auto receive = [&](Perimortem::Core::View::Bytes name,
                         const Abstract& value) {
        visitor(name, value.get_interface());
      };
      static_cast<const Abstract*>(source)->visit_concepts(Visitor(receive));
    },
  };
  return Binding::provide<Scope>(this, operations);
}

auto Abstract::get_type() const -> const Abstract& {
  return None::get_none();
}

auto Abstract::resolve_concept(Perimortem::Core::View::Bytes) const
    -> const Abstract& {
  return None::get_none();
}

auto Abstract::visit_concepts(Visitor) const -> void {}

auto Abstract::satisfies(const Abstract&) const -> Bool {
  return False;
}
