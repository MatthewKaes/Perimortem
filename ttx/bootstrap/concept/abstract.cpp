// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/abstract.hpp"

#include <stddef.h>

#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Ttx::Concept;

auto Abstract::from_abi(const ttx_abstract* abstract) -> const Abstract& {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
  const Count offset = __builtin_offsetof(Abstract, abi);
#pragma clang diagnostic pop
  return *reinterpret_cast<const Abstract*>(
      reinterpret_cast<const U8*>(abstract) - offset);
}

auto Abstract::name_abi(const ttx_abstract* abstract) -> perimortem_bytes {
  Perimortem::Core::View::Bytes name = from_abi(abstract).get_name();
  return {
    .data = name.get_data(),
    .size = name.get_size(),
  };
}

auto Abstract::documentation_abi(const ttx_abstract* abstract)
    -> const ttx_documentation* {
  return from_abi(abstract).get_documentation().get_abi();
}

auto Abstract::resolve_abi(const ttx_abstract* abstract)
    -> const ttx_abstract* {
  return from_abi(abstract).resolve().get_abi();
}

auto Abstract::type_abi(const ttx_abstract* abstract) -> const ttx_abstract* {
  return from_abi(abstract).get_type().get_abi();
}

auto Abstract::concept_abi(const ttx_abstract* abstract, perimortem_bytes name)
    -> const ttx_abstract* {
  return from_abi(abstract)
      .resolve_concept(Perimortem::Core::View::Bytes(name.data, name.size))
      .get_abi();
}

auto Abstract::visit_concepts_abi(
    const ttx_abstract* abstract,
    ttx_named_abstract_callable* visitor) -> void {
  from_abi(abstract).visit_concepts(visitor);
}

auto Abstract::interface_abi(
    const ttx_abstract* abstract,
    const ttx_abstract* requirement) -> ttx_interface {
  return from_abi(abstract).negotiate_interface(requirement);
}

const ttx_abstract_operations Abstract::abi_operations = {
  .name = name_abi,
  .documentation = documentation_abi,
  .resolve = resolve_abi,
  .resolve_concept = concept_abi,
  .visit_concepts = visit_concepts_abi,
  .type = type_abi,
  .interface = interface_abi,
};

auto Abstract::get_type() const -> const Abstract& {
  return None::get_none();
}

auto Abstract::resolve_concept(Perimortem::Core::View::Bytes) const
    -> const Abstract& {
  return Unknown::get_unknown();
}

auto Abstract::visit_concepts(ttx_named_abstract_callable*) const -> void {}

auto Abstract::visit_concept(
    ttx_named_abstract_callable* visitor,
    Perimortem::Core::View::Bytes name,
    const Abstract& value) -> void {
  ttx_named_abstract_callable_call(
      visitor,
      perimortem_bytes{
        .data = name.get_data(),
        .size = name.get_size(),
      },
      value.get_abi());
}

auto Abstract::satisfies(const Abstract&) const -> Bool {
  return False;
}

auto Abstract::negotiate_interface(const ttx_abstract* requirement) const
    -> ttx_interface {
  return requirement == ttx_abstract_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), ttx_interface_marker_operations())
             : ttx_interface_rejected(requirement, get_abi());
}
